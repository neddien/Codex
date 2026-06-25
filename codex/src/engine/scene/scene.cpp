#include "public/scene.h"

#include <engine/audio/audio_manager.h>
#include <engine/core/engine.h>
#include <engine/core/json_archive.h>
#include <engine/core/public/binary_archive.h>
#include <engine/core/public/common_third_party_libs.h>
#include <engine/core/public/serialization_manager.h>
#include <engine/debug/public/profiler.h>
#include <engine/debug/public/time_scope.h>
#include <engine/graphics/renderer.h>
#include <engine/native_behaviour/public/native_behaviour.h>
#include <engine/scene/component_factory.h>
#include <engine/scene/editor_camera.h>
#include <engine/scene/public/components.h>
#include <engine/scene/public/entity.inl>
#include <engine/scene/public/prefab.h>
#include <engine/system/dynamic_library.h>
#include <engine/utils/box2d_utils.h>
#include <engine/utils/public/math.h>

#include <box2d/box2d.h>

namespace codex {
    void B2WorldDeleter::operator()(b2World* world) noexcept
    {
        delete world;
    }

    void Scene::ImportSettings::archive(Archive& ar)
    {
        ar("serdes_type", ar_type_);
    }

    using EntityMap = std::unordered_map<UUID, entt::entity>;

    template <typename... Components>
    static void copy_components(const entt::registry& from, entt::registry& to, const EntityMap& map) noexcept
    {
        (
            [&]()
            {
                const auto view = from.view<Components>();
                for (const auto& e : view) {
                    const auto  uuid      = from.get<IDComponent>(e).uuid;
                    const auto& component = from.get<Components>(e);
                    const auto  entity    = map.at(uuid);
                    to.emplace_or_replace<Components>(entity, component);
                }
            }(),
            ...);
    }

    template <typename... Components>
    static void copy_components(ComponentGroup<Components...>, const entt::registry& from, entt::registry& to,
                                const EntityMap& map) noexcept
    {
        copy_components<Components...>(from, to, map);
    }

    Scene::Scene(Scene&& other) noexcept
    {
        registry_ = std::move(other.registry_);
        name_     = std::move(other.name_);
    }

    Scene& Scene::operator=(Scene&& other) noexcept
    {
        Scene{ std::move(other) }.swap(*this);
        return *this;
    }

    Scene::~Scene() noexcept
    {
        // Attached scripts need to be detached (and freed) first before NBMan is unloaded.
        // The reason why I'm not directly invoking the destructor of entt::basic_registry<> is
        // because it causes a crash on OSX.
        auto view = registry_->view<NativeBehaviourComponent>();
        for (auto& e : view)
            view.get<NativeBehaviourComponent>(e).dispose_behaviours();

        state_.store(State::Edit);
        if (fixed_update_thread_.joinable())
            fixed_update_thread_.join();
    }

    void Scene::copy_to(Scene& other) const noexcept
    {
        /*
        {
            EntityMap entity_map;

            const auto src_reg = registry_.lock();
            const auto id_view = src_reg->view<IDComponent, TagComponent>();

            for (const auto& e : id_view) {
                const auto& tc        = id_view.get<TagComponent>(e);
                const auto& idc       = id_view.get<IDComponent>(e);
                auto        cx_entity = other.create_entity(tc.tag, idc.uuid);
                entity_map[idc.uuid]  = entt::entity{ static_cast<Entity::handle_type>(cx_entity) };
            }

            auto dst_reg = other.registry_.lock();
            copy_components(AllComponents{}, *src_reg, *dst_reg, entity_map);
        }

        other.name_               = name_;
        other.physics_properties_ = physics_properties_;
        */
    }

    void Scene::clone_via_serialization(Scene& other) const noexcept
    {
        const std::vector<u8> bytes = SerializationManager::to_binary(*this);
        SerializationManager::from_binary(other, bytes);
    }

    u32 Scene::entity_count() const noexcept
    {
        return registry_->view<entt::entity>().size_hint();
    }

    void Scene::swap(Scene& other) noexcept
    {
        {
            auto reg       = registry_.lock();
            auto other_reg = other.registry_.lock();
            std::swap(*reg, *other_reg);
        }

        std::swap(name_, other.name_);
        std::swap(fixed_update_thread_, other.fixed_update_thread_);
        std::swap(physics_world_, other.physics_world_);
    }

    bool Scene::is_valid(const Entity entity) const noexcept
    {
        return registry_->valid(entity.handle);
    }

    Entity Scene::create_entity(const std::string_view defaultTag, UUID uuid) noexcept
    {
        Entity cx_entity;
        {
            auto registry   = registry_.lock();
            auto entity     = registry->create();
            cx_entity       = Entity{ entity, this };
            auto& id_comp   = registry->emplace<IDComponent>(entity, uuid);
            id_comp.parent_ = cx_entity;
        }

        cx_entity.add_component<TransformComponent>();
        cx_entity.add_component<TagComponent>(defaultTag);
        return cx_entity;
    }

    void Scene::remove_entity(const Entity entity)
    {
        CX_ASSERT(registry_->valid(entity.handle), "Entity does not exists in registry.");
        registry_->destroy(entity.handle);
    }

    void Scene::remove_entity(const u32 entity)
    {
        registry_->destroy(static_cast<entt::entity>(entity));
    }

    Entity Scene::instantiate_prefab(const scene::Prefab& prefab) noexcept
    {
        return prefab.instantiate(*this);
    }

    Scene::BagHandle Scene::create_behaviour_bag(Entity owner) noexcept
    {
        /* clang-format: no inline */
        auto handle = bag_.emplace_back(NBCRecord{
            .owner = owner,
        });

        log(Info, "Created bag for: ({}, {})", handle.gen(), handle.index());
        return handle;
    }

    void Scene::dispose_behaviour_bag(BagHandle handle) noexcept
    {
        /* clang-format: no inline */
        bag_.erase(handle);
        log(Info, "Bag disposed for: ({}, {})", handle.gen(), handle.index());
    }

    NativeBehaviour* Scene::behaviour(NBHandle bhhandle) noexcept
    {
        Box<NativeBehaviour>* bh = behaviours_.try_at(bhhandle);
        return (bh) ? bh->get() : nullptr;
    }

    std::vector<Scene::NBHandle> Scene::behaviours(BagHandle handle) noexcept
    {
        NBCRecord* record = bag_.try_at(handle);
        return (record) ? std::vector<NBHandle>(record->behaviours.begin(), record->behaviours.end())
                        : std::vector<NBHandle>{};
    }

    Scene::NBHandle Scene::create_behaviour(BagHandle handle, const std::string_view type_name)
    {
        if (const NBMan::BHRecord* type_rec = NBMan::type_record(type_name); type_rec) {
            if (NBCRecord* rec = bag_.try_at(handle); rec) {
                Box<NativeBehaviour> bh = type_rec->factory();
                bh->set_owner(rec->owner);
                NativeBehaviour* bhptr  = bh.get();
                NBHandle         handle = behaviours_.emplace_back(std::move(bh));
                rec->behaviours.emplace(handle);
                return handle;
            }
        }
        return NBHandle::invalid_id();
    }

    void Scene::dispose_behaviour(BagHandle handle, NBHandle bhhandle) noexcept
    {
        if (NBCRecord* record = bag_.try_at(handle); record) {
            record->behaviours.erase(bhhandle);
            behaviours_.erase(bhhandle);
        }
    }

    [[nodiscard]] std::vector<Entity> Scene::entities_with_tag(const std::string_view tag)
    {
        auto                view = registry_->view<TagComponent>();
        std::vector<Entity> entities;
        entities.reserve(view.size());
        for (auto& e : view) {
            if (view.get<TagComponent>(e).tag == tag)
                entities.emplace_back(e, this);
        }
        return entities;
    }

    [[nodiscard]] std::vector<Entity> Scene::entities()
    {
        std::vector<Entity> entities;
        entities.reserve(entity_count());
        for (auto entities_view = registry_->view<entt::entity>(); const auto& e : entities_view) {
            const auto entity = Entity(e, this);
            if (!entity)
                break;
            entities.push_back(entity);
        }
        return entities;
    }

    [[nodiscard]] Entity Scene::primary_camera_entity() noexcept
    {
        return (primary_camera_entity_) ? *primary_camera_entity_ : Entity::none();
    }

    void Scene::render_sprites()
    {
        CX_DEBUG_PROFILE_SCOPE("render_sprites")

        // Sprites
        {
            const auto registry = registry_.lock();
            const auto view     = registry->view<TransformComponent, SpriteRendererComponent>();
            for (auto& e : view) {
                const auto& transform_component = view.get<TransformComponent>(e);
                const auto& renderer_component  = view.get<SpriteRendererComponent>(e);
                if (const auto& s = renderer_component.sprite(); s) {
                    const auto size = s.size();
                    // The scaling we do here is the Sprite's size.

                    // TODO: Get rid of this and optimize this?
                    const auto transform = transform_component.to_matrix() *
                                           glm::scale(glm::identity<Matrix4f>(), { size.x, size.y, 1.0f });
                    gfx::BatchRenderer2D::render_sprite(renderer_component.sprite(), transform, static_cast<i32>(e));
                }
            }
        }

        // Tiles
        {
            const auto registry = registry_.lock();
            const auto view     = registry->view<TilemapComponent, TransformComponent>();
            for (const auto& e : view) {
                auto& tilemap_component = view.get<TilemapComponent>(e);
                for (const auto& tile : tilemap_component.tiles) {
                    auto sprite = tilemap_component.sprite;
                    sprite.set_size(tilemap_component.grid_size);
                    sprite.set_texture_coords(
                        util::to_rectf(tile.atlas, tilemap_component.tile_size.x, tilemap_component.tile_size.y));
                    sprite.set_z_index(tile.layer);

                    auto transform = Matrix4f{ 1.0f };
                    transform      = glm::translate(transform, tile.pos);
                    transform      = glm::scale(transform, Vector3f{ sprite.size().x, sprite.size().y, 1.0f });
                    gfx::BatchRenderer2D::render_sprite(sprite, transform, static_cast<i32>(e));
                }
            }
        }

        // Tileset Animation
        {
            auto registry = registry_.lock();
            auto view     = registry->view<TilesetAnimationComponent, TransformComponent>();
            for (auto& e : view) {
                auto&       tac = view.get<TilesetAnimationComponent>(e);
                const auto& tc  = view.get<TransformComponent>(e);
                for (auto& anim : tac.animations) {
                    // render here
                    gfx::BatchRenderer2D::render_sprite(tac.sprite, tc.to_matrix(), (i32)e);
                    anim.current_frame = (anim.current_frame < anim.frame_count) ? ++anim.current_frame : 0;
                }
            }
        }
    }

    void Scene::render_audio()
    {
        {
            CX_DEBUG_PROFILE_SCOPE("audio_listener_update")
            const auto primary_camera = primary_camera_entity();
            if (primary_camera.has_component<AudioListenerComponent>()) {
                const auto& tc = primary_camera.get_component<TransformComponent>();

                ax::SpatialAttributes attr;
                attr.position = tc.position;
                // attr.velocity = ?
                ax::AudioSystem::set_listener_attributes(attr);
            }
        }

        {
            CX_DEBUG_PROFILE_SCOPE("audio_source_update");
            auto registry = registry_.lock();
            auto view     = registry->view<AudioSourceComponent, TransformComponent>();
            for (auto& e : view) {
                auto& asc = view.get<AudioSourceComponent>(e);
                if (!asc.handle || !asc.handle->is_playing())
                    continue;

                // Sync parameters every frame.
                for (const auto& [name, value] : asc.parameters)
                    asc.handle->set_parameter(name, value);

                if (asc.is_3d) {
                    ax::SpatialAttributes attr;
                    attr.position = view.get<TransformComponent>(e).position;
                    asc.handle->set_spatial_attributes(attr);
                }
            }
        }
    }

    void Scene::construct_physics_bodies()
    {
        // Rigid body 2d construction.
        {
            auto registry = registry_.lock();

            const auto rb2d_view = registry->view<TransformComponent, RigidBody2DComponent>();
            for (const auto& e : rb2d_view) {
                const auto& trans = rb2d_view.get<TransformComponent>(e);
                auto&       rb2d  = rb2d_view.get<RigidBody2DComponent>(e);

                b2BodyDef body_def;
                body_def.position.Set(trans.position.x * physics_properties_.scaling_factor,
                                      trans.position.y * physics_properties_.scaling_factor);
                body_def.type           = util::to_b2_type(rb2d.body_type);
                body_def.angle          = trans.rotation.z;
                body_def.angularDamping = rb2d.angular_damping;
                body_def.linearDamping  = rb2d.linear_damping;
                body_def.bullet         = rb2d.high_velocity;
                body_def.fixedRotation  = rb2d.fixed_rotation;
                body_def.gravityScale   = rb2d.gravity_scale;
                body_def.enabled        = rb2d.enabled;
                body_def.angle          = math::to_radf(trans.rotation.z);
                auto* b2_body           = physics_world_->CreateBody(&body_def);

                rb2d.runtime_body = b2_body;
            }
        }

        // Box collider 2d construction.
        {
            auto registry = registry_.lock();

            const auto box_collider_view =
                registry->view<TransformComponent, RigidBody2DComponent, BoxCollider2DComponent>();
            for (const auto& e : box_collider_view) {
                const auto& body     = box_collider_view.get<RigidBody2DComponent>(e);
                const auto& collider = box_collider_view.get<BoxCollider2DComponent>(e);
                const auto& trans    = box_collider_view.get<TransformComponent>(e);

                b2PolygonShape shape;
                shape.SetAsBox(collider.size.x * trans.scale.x * physics_properties_.scaling_factor,
                               collider.size.y * trans.scale.y * physics_properties_.scaling_factor,
                               util::to_b2_vec2(collider.offset * physics_properties_.scaling_factor), 0.0f);

                b2FixtureDef fixture_def;
                fixture_def.shape                = &shape;
                fixture_def.density              = collider.physics_material.density_;
                fixture_def.friction             = collider.physics_material.friction_;
                fixture_def.restitution          = collider.physics_material.restitution_;
                fixture_def.restitutionThreshold = collider.physics_material.restitution_threshold_;

                reinterpret_cast<b2Body*>(body.runtime_body)->CreateFixture(&fixture_def);
            }
        }

        // Cirlce collider 2d
        {
            auto registry = registry_.lock();

            const auto circle_collider_view =
                registry->view<TransformComponent, RigidBody2DComponent, CircleCollider2DComponent>();
            for (const auto& e : circle_collider_view) {
                const auto& body     = circle_collider_view.get<RigidBody2DComponent>(e);
                const auto& collider = circle_collider_view.get<CircleCollider2DComponent>(e);

                b2CircleShape shape;
                shape.m_p.Set(collider.offset.x * physics_properties_.scaling_factor,
                              collider.offset.y * physics_properties_.scaling_factor);
                shape.m_radius = collider.radius * physics_properties_.scaling_factor;

                b2FixtureDef fixture_def;
                fixture_def.shape                = &shape;
                fixture_def.density              = collider.physics_material.density_;
                fixture_def.friction             = collider.physics_material.friction_;
                fixture_def.restitution          = collider.physics_material.restitution_;
                fixture_def.restitutionThreshold = collider.physics_material.restitution_threshold_;

                reinterpret_cast<b2Body*>(body.runtime_body)->CreateFixture(&fixture_def);
            }
        }
    }

    void Scene::on_editor_init([[maybe_unused]] scene::EditorCamera& camera)
    {
    }

    void Scene::on_runtime_start()
    {
        physics_world_ = Box<b2World, B2WorldDeleter>::make(util::to_b2_vec2(physics_properties_.gravity));
        physics_world_->SetAllowSleeping(true);

        state_.store(State::Play);

        // Grab primary camera.
        {
            auto registry = registry_.lock();

            // Grab the primary camera entity.
            {
                auto view = registry->view<TransformComponent, CameraComponent>();
                for (auto& e : view) {
                    auto& c = view.get<CameraComponent>(e);
                    if (c.primary) {
                        primary_camera_entity_.reset(new Entity(e, this));
                        break;
                    }
                }
            }
        }

        construct_physics_bodies();

        // Native behaviour instantiation
        // TODO: Thread safety
        {
            for (Box<NativeBehaviour>& bh : behaviours_) {
                assert(bh);

                try {
                    bh->on_init();
                }
                catch (const std::exception& ex) {
                    log(Error, "A behaviour exception occured: {}", ex.what());

                    // FIXME: This doesn't work as intended!
                    if (state() == Scene::State::Play) {
                        // TODO: Edit? What if we're on runtime?
                        set_state(Scene::State::Edit);
                        on_runtime_stop();
                    } else if (state() == Scene::State::Simulate) {
                        // TODO: Edit? What if we're on runtime?
                        set_state(Scene::State::Edit);
                        on_simulation_stop();
                    }
                }
            }
        }

        fixed_update_thread_ = std::thread(Scene::on_fixed_update, std::ref(*this));

        // Begin audio events or playback from AudioSourceComponents
        {
            auto asc_view = registry_->view<AudioSourceComponent, TransformComponent>();
            for (auto& e : asc_view) {
                auto& asc = asc_view.get<AudioSourceComponent>(e);
                if (asc.play_on_start) {
                    try {
                        auto event = Shared<ax::EventHandle>::make(ax::AudioManager::load_event(asc.event_path));
                        asc.handle = event;

                        event->set_volume(asc.volume);
                        event->set_pitch(asc.pitch);

                        for (const auto& [name, value] : asc.parameters)
                            event->set_parameter(name, value);

                        if (asc.is_3d) {
                            event->set_min_max_distance(asc.min_distance, asc.max_distance);

                            ax::SpatialAttributes attr;
                            attr.position = asc_view.get<TransformComponent>(e).position;
                            event->set_spatial_attributes(attr);
                        }

                        event->play();
                    }
                    catch (const CodexException& ex) {
                        log(Error, "{}", ex.what());
                    }
                }
            }
        }
    }

    void Scene::on_simulation_start()
    {
        physics_world_ = Box<b2World, B2WorldDeleter>::make(util::to_b2_vec2(physics_properties_.gravity));
        physics_world_->SetAllowSleeping(true);

        state_.store(State::Simulate);

        construct_physics_bodies();

        // Native behaviour instantiation
        // TODO: Thread safety
        {
            for (Box<NativeBehaviour>& bh : behaviours_) {
                assert(bh);

                try {
                    bh->on_init();
                }
                catch (const std::exception& ex) {
                    log(Error, "A behaviour exception occured: {}", ex.what());

                    // FIXME: This doesn't work as intended!
                    if (state() == Scene::State::Play) {
                        // TODO: Edit? What if we're on runtime?
                        set_state(Scene::State::Edit);
                        on_runtime_stop();
                    } else if (state() == Scene::State::Simulate) {
                        // TODO: Edit? What if we're on runtime?
                        set_state(Scene::State::Edit);
                        on_simulation_stop();
                    }
                }
            }
        }

        fixed_update_thread_ = std::thread(Scene::on_fixed_update, std::ref(*this));
    }

    void Scene::on_simulation_stop()
    {
        state_.store(State::Edit);

        if (fixed_update_thread_.joinable())
            fixed_update_thread_.join();

        physics_world_.reset();
    }

    void Scene::on_runtime_stop()
    {
        state_.store(State::Edit);

        if (fixed_update_thread_.joinable())
            fixed_update_thread_.join();

        physics_world_.reset();

        ax::AudioManager::stop_all();
    }

    void Scene::on_editor_update([[maybe_unused]] const f32 dt, scene::EditorCamera& camera, gfx::Shader* end_shader)
    {
        // Render
        {
            gfx::BatchRenderer2D::begin(camera);
            render_sprites();
            gfx::BatchRenderer2D::end(end_shader);
        }
    }

    void Scene::on_simulation_update([[maybe_unused]] const f32 dt, scene::EditorCamera& camera)
    {
        // Render
        {
            gfx::BatchRenderer2D::begin(camera);
            render_sprites();
            gfx::BatchRenderer2D::end();
        }
    }

    void Scene::on_runtime_update(const f32 dt)
    {
        // Render.
        {
            // Grab primary camera.
            {
                auto registry = registry_.lock();

                // Grab the primary camera entity.
                {
                    auto view = registry->view<TransformComponent, CameraComponent>();
                    for (auto& e : view) {
                        auto& c = view.get<CameraComponent>(e);
                        if (c.primary) {
                            *primary_camera_entity_ = Entity{ e, this };
                            break;
                        }
                    }
                }
            }

            // Render our sprites if there's a camera.
            if (primary_camera_entity_) {
                const auto& cc = primary_camera_entity_->get_component<CameraComponent>();
                const auto& tc = primary_camera_entity_->get_component<TransformComponent>();

                gfx::BatchRenderer2D::begin(cc.camera, tc);
                render_sprites();
                gfx::BatchRenderer2D::end();
            }
        }

        // Native behaviour instantiation
        // TODO: Thread safety
        {
            for (Box<NativeBehaviour>& bh : behaviours_) {
                assert(bh);

                try {
                    bh->on_update(dt);
                }
                catch (const std::exception& ex) {
                    log(Error, "A behaviour exception occured: {}", ex.what());

                    // FIXME: This doesn't work as intended!
                    if (state() == Scene::State::Play) {
                        // TODO: Edit? What if we're on runtime?
                        set_state(Scene::State::Edit);
                        on_runtime_stop();
                    } else if (state() == Scene::State::Simulate) {
                        // TODO: Edit? What if we're on runtime?
                        set_state(Scene::State::Edit);
                        on_simulation_stop();
                    }
                }
            }
        }

        render_audio();
    }

    void Scene::on_fixed_update(Scene& self) noexcept
    {
        using clock = std::chrono::high_resolution_clock;

        const auto frame_interval     = 1.0f / self.physics_properties_.tick_rate;
        const auto max_frame_interval = 1.0f / Engine::fps();

        auto lag         = 0.0f;
        auto frame_start = clock::now();
        while (self.state_.load() != State::Edit) {
            const auto frame_now  = clock::now();
            const auto frame_time = std::chrono::duration<f32>(clock::now() - frame_start).count();
            frame_start           = frame_now;

            lag += frame_time;

            while (lag >= frame_interval) {
                // Native behaviours.
                if (self.state_.load() == State::Play || self.state_.load() == State::Simulate) {
                    // Native behaviour instantiation
                    // TODO: Thread safety
                    for (Box<NativeBehaviour>& bh : self.behaviours_) {
                        assert(bh);

                        try {
                            bh->on_fixed_update(frame_interval);
                        }
                        catch (const std::exception& ex) {
                            self.log(Error, "A behaviour exception occured: {}", ex.what());

                            // FIXME: This doesn't work as intended!
                            if (self.state() == Scene::State::Play) {
                                // TODO: Edit? What if we're on runtime?
                                self.set_state(Scene::State::Edit);
                                self.on_runtime_stop();
                            } else if (self.state() == Scene::State::Simulate) {
                                // TODO: Edit? What if we're on runtime?
                                self.set_state(Scene::State::Edit);
                                self.on_simulation_stop();
                            }
                        }
                    }
                }

                // Physics.
                {
                    auto registry = self.registry_.lock();
                    auto view     = registry->view<TransformComponent, RigidBody2DComponent>();
                    self.physics_world_->Step(frame_interval, self.physics_properties_.velocity_iterations,
                                              self.physics_properties_.position_iterations);
                    for (auto& e : view) {
                        auto& trans = view.get<TransformComponent>(e);
                        auto& rb2d  = view.get<RigidBody2DComponent>(e);

                        auto        b2_body = reinterpret_cast<b2Body*>(rb2d.runtime_body);
                        const auto& b2_pos  = b2_body->GetPosition();

                        trans.position.x = b2_pos.x / self.physics_properties_.scaling_factor + 1.0f;
                        trans.position.y = b2_pos.y / self.physics_properties_.scaling_factor + 1.0f;
                        trans.rotation.z = math::to_degf(b2_body->GetAngle());
                    }
                }

                lag -= frame_interval;
            }

            // Upper bound for the outer loop.
            if (frame_time < max_frame_interval)
                std::this_thread::sleep_for(std::chrono::duration<f32>(max_frame_interval - frame_time));
        }
    }

    void Scene::archive(Archive& ar)
    {
        ar("name", name_);

        // Polymorphic component dispatch needs explicit control over the array/type-tag
        // traversal, so this talks to the backend directly rather than through ar(key, vec).
        IArchiveBackend& b = ar.backend();

        if (ar.saving()) {
            // The array length must precede its elements (binary), so count first.
            usize entity_total = 0;
            for (auto view = registry_->view<entt::entity>(); const auto e : view) {
                if (!Entity(e, this))
                    break;
                ++entity_total;
            }

            b.begin_array("entities", entity_total);
            for (auto view = registry_->view<entt::entity>(); const auto e : view) {
                auto entity = Entity(e, this);
                if (!entity)
                    break;

                b.begin_object({});

                Component* head  = &entity.get_component<IDComponent>();
                usize      count = 0;
                for (const Component* c = head; c; c = c->next_)
                    ++count;

                b.begin_array("components", count);
                for (Component* c = head; c; c = c->next_) {
                    b.begin_object({});
                    c->archive(ar); // writes "type" + fields
                    b.end_object();
                }
                b.end_array();

                b.end_object();
            }
            b.end_array();
        } else {
            usize entity_total = 0;
            b.begin_array("entities", entity_total);
            for (usize i = 0; i < entity_total; ++i) {
                b.begin_object({});
                auto entity = create_entity();

                usize count = 0;
                b.begin_array("components", count);
                for (usize c = 0; c < count; ++c) {
                    b.begin_object({});
                    std::string type_name;
                    ar("type", type_name); // consume the tag, then dispatch the rest
                    if (!type_name.empty())
                        ComponentFactory::get().deserialize_component(type_name, ar, entity);
                    b.end_object();
                }
                b.end_array();
                b.end_object();
            }
            b.end_array();
        }
    }
} // namespace codex
