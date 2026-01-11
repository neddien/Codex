#include "Public/Scene.h"

#include <box2d/box2d.h>
#include <entt.hpp>

#include <Debug/Public/Profiler.h>
#include <Debug/Public/TimeScope.h>
#include <Engine/Core/Application.h>
#include <Engine/Graphics/Renderer.h>
#include <Engine/NativeBehaviour/Public/NativeBehaviour.h>
#include <Engine/Reflection/Reflector.h>
#include <Engine/Scene/ComponentFactory.h>
#include <Engine/Utils/Box2DUtils.h>
#include <Engine/Utils/Public/Math.h>
#include <Engine/Scene/EditorCamera.h>
#include <Engine/System/DynamicLibrary.h>

namespace codex {
    using EntityMap = std::unordered_map<UUID, entt::entity>;

    template <typename... Components>
    static void CopyComponents(const entt::registry& from, entt::registry& to, const EntityMap& map) noexcept
    {
        (
            [&]()
            {
                const auto view = from.view<Components>();
                for (const auto& e : view)
                {
                    const auto  uuid      = from.get<IDComponent>(e).uuid;
                    const auto& component = from.get<Components>(e);
                    const auto  entity    = map.at(uuid);
                    to.emplace_or_replace<Components>(entity, component);
                }
            }(),
            ...);
    }

    template <typename... Components>
    static void CopyComponents(ComponentGroup<Components...>, const entt::registry& from, entt::registry& to,
                               const EntityMap& map) noexcept
    {
        CopyComponents<Components...>(from, to, map);
    }

    Scene::Scene(Scene&& other) noexcept
    {
        m_Registry     = std::move(other.m_Registry);
        m_Name         = std::move(other.m_Name);
    }

    Scene& Scene::operator=(Scene&& other) noexcept
    {
        Scene{ std::move(other) }.Swap(*this);
        return *this;
    }

    Scene::~Scene() noexcept
    {
        // Attached scripts need to be detached (and freed) first before NBMan is unloaded.
        // The reason why I'm not directly invoking the destructor of entt::basic_registry<> is
        // because it causes a crash on OSX.
        auto view = m_Registry->view<NativeBehaviourComponent>();
        for (auto& e : view)
            view.get<NativeBehaviourComponent>(e).DisposeBehaviours();

        m_State.store(State::Edit);
        if (m_FixedUpdateThread.joinable())
            m_FixedUpdateThread.join();
    }

    void Scene::CopyTo(Scene& other) const noexcept
    {
        {
            EntityMap entity_map;

            const auto src_reg = m_Registry.Lock();
            const auto id_view = src_reg->view<IDComponent, TagComponent>();

            for (const auto& e : id_view)
            {
                const auto& tc        = id_view.get<TagComponent>(e);
                const auto& idc       = id_view.get<IDComponent>(e);
                auto        cx_entity = other.CreateEntity(tc.tag, idc.uuid);
                entity_map[idc.uuid]  = entt::entity{ static_cast<Entity::HandleType>(cx_entity) };
            }

            auto dst_reg = other.m_Registry.Lock();
            CopyComponents(AllComponents{}, *src_reg, *dst_reg, entity_map);
        }

        other.m_Name              = m_Name;
        other.m_PhysicsProperties = m_PhysicsProperties;
    }

    u32 Scene::GetEntityCount() const noexcept
    {
        return m_Registry->view<entt::entity>().size_hint();
    }

    void Scene::Swap(Scene& other) noexcept
    {
        {
            auto reg       = m_Registry.Lock();
            auto other_reg = other.m_Registry.Lock();
            std::swap(*reg, *other_reg);
        }

        std::swap(m_Name, other.m_Name);
        std::swap(m_FixedUpdateThread, other.m_FixedUpdateThread);
        std::swap(m_PhysicsWorld, other.m_PhysicsWorld);
    }

    bool Scene::IsValid(const Entity entity) const noexcept
    {
        return m_Registry->valid(entity.m_Handle);
    }

    Entity Scene::CreateEntity(const std::string_view defaultTag, UUID uuid) noexcept
    {
        Entity cx_entity;
        {
            auto registry = m_Registry.Lock();
            auto entity   = registry->create();
            cx_entity     = Entity{ entity, this };
            auto& id_comp = registry->emplace<IDComponent>(entity, uuid);
            id_comp.m_Parent = cx_entity;
        }

        cx_entity.AddComponent<TransformComponent>();
        cx_entity.AddComponent<TagComponent>(defaultTag);
        return cx_entity;
    }

    void Scene::RemoveEntity(const Entity entity)
    {
        CX_ASSERT(m_Registry->valid(entity.m_Handle), "Entity does not exists in registry.");
        m_Registry->destroy(entity.m_Handle);
    }

    void Scene::RemoveEntity(const u32 entity)
    {
        m_Registry->destroy(static_cast<entt::entity>(entity));
    }

    [[nodiscard]] std::vector<Entity> Scene::GetAllEntitesWithTag(const std::string_view tag)
    {
        auto                view = m_Registry->view<TagComponent>();
        std::vector<Entity> entities;
        entities.reserve(view.size());
        for (auto& e : view)
        {
            if (view.get<TagComponent>(e).tag == tag)
                entities.emplace_back(e, this);
        }
        return entities;
    }

    [[nodiscard]] std::vector<Entity> Scene::GetAllEntities()
    {
        std::vector<Entity> entities;
        entities.reserve(GetEntityCount());
        for (auto entities_view = m_Registry->view<entt::entity>(); const auto& e : entities_view)
        {
            const auto entity = Entity(e, this);
            if (!entity)
                break;
            entities.push_back(entity);
        }
        return entities;
    }

    [[nodiscard]] Entity Scene::GetPrimaryCameraEntity() noexcept
    {
        return (m_PrimaryCameraEntity) ? *m_PrimaryCameraEntity : Entity::None();
    }

    void Scene::RenderSprites()
    {
        // Sprites
        {
            CX_DEBUG_PROFILE_SCOPE("SpriteRender")

            const auto registry = m_Registry.Lock();
            const auto view     = registry->view<TransformComponent, SpriteRendererComponent>();
            for (auto& e : view)
            {
                const auto& transform_component = view.get<TransformComponent>(e);
                const auto& renderer_component  = view.get<SpriteRendererComponent>(e);
                if (const auto& s = renderer_component.GetSprite(); s)
                {
                    const auto size = s.GetSize();
                    // The scaling we do here is the Sprite's size.

                    // TODO: Get rid of this and optimize this?
                    const auto transform = transform_component.ToMatrix() *
                                           glm::scale(glm::identity<Matrix4f>(), { size.x, size.y, 1.0f });
                    gfx::BatchRenderer2D::RenderSprite(renderer_component.GetSprite(), transform, static_cast<i32>(e));
                }
            }
        }

        // Tiles
        {
            CX_DEBUG_PROFILE_SCOPE("TileRender")

            const auto registry = m_Registry.Lock();
            const auto view     = registry->view<TilemapComponent, TransformComponent>();
            for (const auto& e : view)
            {
                auto& tilemap_component = view.get<TilemapComponent>(e);
                for (const auto& tile : tilemap_component.tiles)
                {
                    auto sprite = tilemap_component.sprite;
                    sprite.SetSize(tilemap_component.gridSize);
                    sprite.SetTextureCoords(
                        utils::ToRectf(tile.atlas, tilemap_component.tileSize.x, tilemap_component.tileSize.y));
                    sprite.SetZIndex(tile.layer);

                    auto transform = Matrix4f{ 1.0f };
                    transform      = glm::translate(transform, tile.pos);
                    transform      = glm::scale(transform, Vector3f{ sprite.GetSize().x, sprite.GetSize().y, 1.0f });
                    gfx::BatchRenderer2D::RenderSprite(sprite, transform, static_cast<i32>(e));
                }
            }
        }
    }

    void Scene::ConstructPhysicsBodies()
    {
        // Rigid body 2d construction.
        {
            auto registry = m_Registry.Lock();

            const auto rb2d_view = registry->view<TransformComponent, RigidBody2DComponent>();
            for (const auto& e : rb2d_view)
            {
                const auto& trans = rb2d_view.get<TransformComponent>(e);
                auto&       rb2d  = rb2d_view.get<RigidBody2DComponent>(e);

                b2BodyDef body_def;
                body_def.position.Set(trans.position.x * m_PhysicsProperties.scalingFactor,
                                      trans.position.y * m_PhysicsProperties.scalingFactor);
                body_def.type           = utils::ToB2Type(rb2d.bodyType);
                body_def.angle          = trans.rotation.z;
                body_def.angularDamping = rb2d.angularDamping;
                body_def.linearDamping  = rb2d.linearDamping;
                body_def.bullet         = rb2d.highVelocity;
                body_def.fixedRotation  = rb2d.fixedRotation;
                body_def.gravityScale   = rb2d.gravityScale;
                body_def.enabled        = rb2d.enabled;
                body_def.angle          = math::ToRadf(trans.rotation.z);
                auto* b2_body           = m_PhysicsWorld->CreateBody(&body_def);

                rb2d.runtimeBody = b2_body;
            }
        }

        // Box collider 2d construction.
        {
            auto registry = m_Registry.Lock();

            const auto box_collider_view =
                registry->view<TransformComponent, RigidBody2DComponent, BoxCollider2DComponent>();
            for (const auto& e : box_collider_view)
            {
                const auto& body     = box_collider_view.get<RigidBody2DComponent>(e);
                const auto& collider = box_collider_view.get<BoxCollider2DComponent>(e);
                const auto& trans    = box_collider_view.get<TransformComponent>(e);

                b2PolygonShape shape;
                shape.SetAsBox(collider.size.x * trans.scale.x * m_PhysicsProperties.scalingFactor,
                               collider.size.y * trans.scale.y * m_PhysicsProperties.scalingFactor,
                               utils::ToB2Vec2(collider.offset * m_PhysicsProperties.scalingFactor), 0.0f);

                b2FixtureDef fixture_def;
                fixture_def.shape                = &shape;
                fixture_def.density              = collider.physicsMaterial.density;
                fixture_def.friction             = collider.physicsMaterial.friction;
                fixture_def.restitution          = collider.physicsMaterial.restitution;
                fixture_def.restitutionThreshold = collider.physicsMaterial.restitutionThreshold;

                reinterpret_cast<b2Body*>(body.runtimeBody)->CreateFixture(&fixture_def);
            }
        }

        // Cirlce collider 2d.
        {
            auto registry = m_Registry.Lock();

            const auto circle_collider_view =
                registry->view<TransformComponent, RigidBody2DComponent, CircleCollider2DComponent>();
            for (const auto& e : circle_collider_view)
            {
                const auto& body     = circle_collider_view.get<RigidBody2DComponent>(e);
                const auto& collider = circle_collider_view.get<CircleCollider2DComponent>(e);

                b2CircleShape shape;
                shape.m_p.Set(collider.offset.x * m_PhysicsProperties.scalingFactor,
                              collider.offset.y * m_PhysicsProperties.scalingFactor);
                shape.m_radius = collider.radius * m_PhysicsProperties.scalingFactor;

                b2FixtureDef fixture_def;
                fixture_def.shape                = &shape;
                fixture_def.density              = collider.physicsMaterial.density;
                fixture_def.friction             = collider.physicsMaterial.friction;
                fixture_def.restitution          = collider.physicsMaterial.restitution;
                fixture_def.restitutionThreshold = collider.physicsMaterial.restitutionThreshold;

                reinterpret_cast<b2Body*>(body.runtimeBody)->CreateFixture(&fixture_def);
            }
        }
    }

    void Scene::OnEditorInit([[maybe_unused]] scene::EditorCamera& camera)
    {
    }

    void Scene::OnRuntimeStart()
    {
        m_PhysicsWorld = mem::Box<b2World>::New(utils::ToB2Vec2(m_PhysicsProperties.gravity));
        m_PhysicsWorld->SetAllowSleeping(true);

        m_State.store(State::Play);

        // Grab primary camera.
        {
            auto registry = m_Registry.Lock();

            // Grab the primary camera entity.
            {
                auto view = registry->view<TransformComponent, CameraComponent>();
                for (auto& e : view)
                {
                    auto& c = view.get<CameraComponent>(e);
                    if (c.primary)
                    {
                        m_PrimaryCameraEntity.Reset(new Entity(e, this));
                        break;
                    }
                }
            }
        }

        ConstructPhysicsBodies();

        // Native behaviour instantiation.
        {
            // Lock the registry only for one statement because user scripts can also possibly lock the registry
            // for interactions (such as calls to GetComponent<T>, HasComponent<T> etc...) instead of
            // locking for the entire scope.
            auto nbc_view = m_Registry->view<NativeBehaviourComponent>();
            for (auto& e : nbc_view)
            {
                auto& nbc = nbc_view.get<NativeBehaviourComponent>(e);
                nbc.SetParent(Entity{ e, this });

                try
                {
                    nbc.OnInit();
                }
                catch (const NativeBehaviourException& ex)
                {
                    // If NBC fails to initialise all components then most likely one of the Behaviours threw an error
                    // occured; revert state back to State::Edit and halt physics simulation.
                    OnRuntimeStop();

                    // TODO: We should stop calling lgx::Get for every log.
                    // TODO: Review this inner exception thing for later.
                    auto& exi = ex.InnerException();
                    lgx::Get("engine").Error("A Behaviour threw an Error: {}", exi.to_string());
                    return;
                }
            }
        }

        m_FixedUpdateThread = std::thread(Scene::OnFixedUpdate, std::ref(*this));
    }

    void Scene::OnSimulationStart()
    {
        m_PhysicsWorld = mem::Box<b2World>::New(utils::ToB2Vec2(m_PhysicsProperties.gravity));
        m_PhysicsWorld->SetAllowSleeping(true);

        m_State.store(State::Simulate);

        ConstructPhysicsBodies();

        // Native behaviour instantiation.
        {
            // Lock the registry only for one statement because user scripts can also possibly lock the registry
            // for interactions (such as calls to GetComponent<T>, HasComponent<T> etc...) instead of
            // locking for the entire scope.
            auto nbc_view = m_Registry->view<NativeBehaviourComponent>();
            for (auto& e : nbc_view)
            {
                auto& nbc = nbc_view.get<NativeBehaviourComponent>(e);
                nbc.SetParent(Entity{ e, this });

                try
                {
                    nbc.OnInit();
                }
                catch (const NativeBehaviourException& ex)
                {
                    // If NBC fails to initialise all components then most likely one of the Behaviours threw an error
                    // occured; revert state back to State::Edit and halt physics simulation.
                    OnRuntimeStop();

                    // TODO: We should stop calling lgx::Get for every log.
                    // TODO: Review this inner exception thing for later.
                    auto& exi = ex.InnerException();
                    lgx::Get("engine").Error("A Behaviour threw an Error: {}", exi.to_string());
                    return;
                }
            }
        }

        m_FixedUpdateThread = std::thread(Scene::OnFixedUpdate, std::ref(*this));
    }

    void Scene::OnSimulationStop()
    {
        m_State.store(State::Edit);

        if (m_FixedUpdateThread.joinable())
            m_FixedUpdateThread.join();

        m_PhysicsWorld.Reset();
    }

    void Scene::OnRuntimeStop()
    {
        m_State.store(State::Edit);

        if (m_FixedUpdateThread.joinable())
            m_FixedUpdateThread.join();

        m_PhysicsWorld.Reset();
    }

    void Scene::OnEditorUpdate([[maybe_unused]] const f32 deltaTime, scene::EditorCamera& camera)
    {
        // Render
        {
            gfx::BatchRenderer2D::Begin(camera);
            RenderSprites();
            gfx::BatchRenderer2D::End();
        }
    }

    void Scene::OnSimulationUpdate([[maybe_unused]] const f32 deltaTime, scene::EditorCamera& camera)
    {
        // Render
        {
            gfx::BatchRenderer2D::Begin(camera);
            RenderSprites();
            gfx::BatchRenderer2D::End();
        }
    }

    void Scene::OnRuntimeUpdate(const f32 deltaTime)
    {
        // Render.
        {
            // Grab primary camera.
            {
                auto registry = m_Registry.Lock();

                // Grab the primary camera entity.
                {
                    auto view = registry->view<TransformComponent, CameraComponent>();
                    for (auto& e : view)
                    {
                        auto& c = view.get<CameraComponent>(e);
                        if (c.primary)
                        {
                            *m_PrimaryCameraEntity = Entity{ e, this };
                            break;
                        }
                    }
                }
            }

            // Render our sprites if there's a camera.
            if (m_PrimaryCameraEntity)
            {
                const auto& cc = m_PrimaryCameraEntity->GetComponent<CameraComponent>();
                const auto& tc = m_PrimaryCameraEntity->GetComponent<TransformComponent>();

                gfx::BatchRenderer2D::Begin(cc.camera, tc);
                RenderSprites();
                gfx::BatchRenderer2D::End();
            }
        }

        // Native scripts.
        {
            // Lock the registry only for one statement because user scripts can also possibly lock the registry
            // for interactions (such as calls to GetComponent<T>, HasComponent<T> etc...) instead of
            // locking for the entire scope.
            auto view = m_Registry->view<NativeBehaviourComponent>();
            for (auto& e : view)
            {
                auto& nbc = view.get<NativeBehaviourComponent>(e);
                try
                {
                    nbc.OnUpdate(deltaTime);
                }
                catch (const NativeBehaviourException& ex)
                {
                    // If NBC fails to update all components then most likely one of the Behaviours threw an error
                    // occured; revert state back to State::Edit and halt physics simulation.
                    OnRuntimeStop();

                    // TODO: We should stop calling lgx::Get for every log.
                    // TODO: Review this inner exception thing for later.
                    auto& exi = ex.InnerException();
                    lgx::Get("engine").Error("A Behaviour threw an Error: {}", exi.to_string());
                    return;
                }
            }
        }
    }

    void Scene::OnFixedUpdate(Scene& self) noexcept
    {
        using clock = std::chrono::high_resolution_clock;

        const auto frame_interval     = 1.0f / self.m_PhysicsProperties.tickRate;
        const auto max_frame_interval = 1.0f / Application::GetFps();

        auto lag         = 0.0f;
        auto frame_start = clock::now();
        while (self.m_State.load() != State::Edit)
        {
            const auto frame_now  = clock::now();
            const auto frame_time = std::chrono::duration<f32>(clock::now() - frame_start).count();
            frame_start           = frame_now;

            lag += frame_time;

            while (lag >= frame_interval)
            {
                // Native behaviours.
                if (self.m_State.load() == State::Play)
                {
                    // Lock the registry only for one statement because user scripts can also possibly lock the registry
                    // for interactions (such as calls to GetComponent<T>, HasComponent<T> etc...) instead of
                    // locking for the entire scope.
                    auto view = (*self.m_Registry)->view<NativeBehaviourComponent>();
                    for (auto& e : view)
                    {
                        auto& nbc = view.get<NativeBehaviourComponent>(e);
                        nbc.OnFixedUpdate(frame_interval);
                    }
                }

                // Physics.
                {
                    auto registry = self.m_Registry.Lock();
                    auto view     = registry->view<TransformComponent, RigidBody2DComponent>();
                    self.m_PhysicsWorld->Step(frame_interval, self.m_PhysicsProperties.velocityIterations,
                                              self.m_PhysicsProperties.positionIterations);
                    for (auto& e : view)
                    {
                        auto& trans = view.get<TransformComponent>(e);
                        auto& rb2d  = view.get<RigidBody2DComponent>(e);

                        auto        b2_body = reinterpret_cast<b2Body*>(rb2d.runtimeBody);
                        const auto& b2_pos  = b2_body->GetPosition();

                        trans.position.x = b2_pos.x / self.m_PhysicsProperties.scalingFactor + 1.0f;
                        trans.position.y = b2_pos.y / self.m_PhysicsProperties.scalingFactor + 1.0f;
                        trans.rotation.z = math::ToDegf(b2_body->GetAngle());
                    }
                }

                lag -= frame_interval;
            }

            // Upper bound for the outer loop.
            if (frame_time < max_frame_interval)
                std::this_thread::sleep_for(std::chrono::duration<f32>(max_frame_interval - frame_time));
        }
    }

    void Scene::Serialize(ISerializationNode& node) const
    {
        node.Write("name", m_Name);
        auto& entities = node.BeginArray("entities");

        for (auto entities_view = m_Registry->view<entt::entity>(); const auto e : entities_view)
        {
            const auto entity = Entity(e, const_cast<Scene*>(this));
            if (!entity)
                break;

            auto&            node   = entities.AddArrayElement();
            auto&            idcomp = entity.GetComponent<IDComponent>();
            const Component* comp   = &idcomp;

            auto& components = node.BeginArray("components");

            while (comp)
            {
                auto& node = components.AddArrayElement();
                comp->Serialize(node);
                comp = comp->m_Next;
            }

            components.EndArray();
        }

        entities.EndArray();
    }

    void Scene::Deserialize(const ISerializationNode& node)
    {
        node.Read("name", m_Name);

        auto& entities = node.GetArray("entities");
        entities.ForEachArrayElement(
            [this](const ISerializationNode& inode)
            {
                auto entity = CreateEntity();

                auto& components = inode.GetArray("components");
                components.ForEachArrayElement(
                    [&entity](const ISerializationNode& jnode)
                    {
                        std::string type_name;
                        if (jnode.Read("type", type_name))
                        {
                            ComponentFactory::Get().DeserializeComponent(type_name, jnode, entity);
                        }
                    });
            });
    }
} // namespace codex
