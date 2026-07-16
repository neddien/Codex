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
    { delete world; }

    void Scene::ImportSettings::archive(Archive& ar)
    { ar("serdes_type", ar_type_); }

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
    { copy_components<Components...>(from, to, map); }

    template <typename... Components>
    [[nodiscard]] static std::vector<Component*> collect_components(ComponentGroup<Components...>,
                                                                    Entity entity) noexcept
    {
        std::vector<Component*> components;
        (
            [&]()
            {
                if (entity.has_component<Components>()) {
                    components.push_back(&entity.get_component<Components>());
                }
            }(),
            ...);

        return components;
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
        // Stop the fixed-update thread first: it iterates the behaviours we are
        // about to destroy.
        state_.store(State::Edit);
        if (fixed_update_thread_.joinable())
            fixed_update_thread_.join();

        // Attached scripts need to be detached (and freed) first before NBMan is unloaded.
        // The reason why I'm not directly invoking the destructor of entt::basic_registry<> is
        // because it causes a crash on OSX.
        auto view = registry_->view<NativeBehaviourComponent>();
        for (auto& e : view)
            view.get<NativeBehaviourComponent>(e).dispose_behaviours();
    }

    void Scene::clone_via_serialization(Scene& other) const
    {
        const std::vector<u8> bytes = SerializationManager::to_binary(*this);
        SerializationManager::from_binary(other, bytes);
    }

    u32 Scene::entity_count() const noexcept
    { return registry_->view<entt::entity>().size_hint(); }

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
    { return registry_->valid(entity.handle_); }

    Entity Scene::create_entity(std::string_view tag) noexcept
    {
        Entity cx_entity;
        {
            auto registry                 = registry_.lock();
            auto entity                   = registry->create();
            cx_entity                     = Entity{ entity, this };
            auto& id_comp                 = registry->emplace<IDComponent>(entity, UUID{});
            uuid_to_entity_[id_comp.uuid] = cx_entity;
            id_comp.parent_               = cx_entity;
        }

        cx_entity.add_component<TagComponent>(tag);
        return cx_entity;
    }

    Entity Scene::create_entity(const std::optional<math::transform>& transform, std::string_view tag,
                                UUID uuid) noexcept
    {
        Entity cx_entity;
        {
            auto registry                 = registry_.lock();
            auto entity                   = registry->create();
            cx_entity                     = Entity{ entity, this };
            auto& id_comp                 = registry->emplace<IDComponent>(entity, uuid);
            uuid_to_entity_[id_comp.uuid] = cx_entity;
            id_comp.parent_               = cx_entity;
        }

        cx_entity.add_component<TransformComponent>(transform.value_or(math::transform{}));
        cx_entity.add_component<TagComponent>(tag);
        return cx_entity;
    }

    void Scene::remove_entity(Entity entity)
    {
        cxassert(registry_->valid(entity.handle_), "Entity does not exists in registry.");

        std::scoped_lock guard{ mutex_ };
        auto             registry = registry_.lock();

        // Detach in case there's a hierarchy
        if (auto* hc = registry->try_get<HierarchyComponent>(entity.handle_); hc) {
            HierarchyComponent* p_hc = nullptr;

            if (registry->valid(hc->parent.handle_)) {
                p_hc = registry->try_get<HierarchyComponent>(hc->parent.handle_);

                if (p_hc) {
                    if (auto it = std::find(p_hc->children.begin(), p_hc->children.end(), entity);
                        it != p_hc->children.end())
                        p_hc->children.erase(it);
                }
            }

            for (Entity& e : hc->children) {
                auto& c_hc  = registry->get<HierarchyComponent>(e.handle_);
                c_hc.parent = hc->parent; // If we don't have a parent, neither will our children when we'll die so its
                                          // okay to assign parent here blindly

                if (p_hc) {
                    p_hc->children.push_back(e);
                }
            }
        }

        auto& idc = registry->get<IDComponent>(entity.handle_);
        if (auto it = uuid_to_entity_.find(idc.uuid); it != uuid_to_entity_.end())
            uuid_to_entity_.erase(it);

        // Release the entity's behaviours (and its bag) before the registry entry gets deleted
        if (auto* nbc = registry->try_get<NativeBehaviourComponent>(entity.handle_))
            nbc->dispose();

        registry->destroy(entity.handle_);
    }

    void Scene::remove_entity(const u32 entity)
    { remove_entity(Entity{ static_cast<entt::entity>(entity), this }); }

    Entity Scene::instantiate_prefab(const scene::Prefab& prefab, const std::optional<math::transform>& transform,
                                     std::string_view tag, UUID uuid) noexcept
    {
        Entity entity = prefab.instantiate(*this);

        if (transform)
            static_cast<math::transform&>(entity.get_component<TransformComponent>()) = *transform;

        auto& idc = entity.get_component<IDComponent>();
        auto& tc  = entity.get_component<TagComponent>();

        // Updated the UUID to Entity map
        if (auto it = uuid_to_entity_.find(idc.uuid); it != uuid_to_entity_.end())
            uuid_to_entity_.erase(it);
        uuid_to_entity_[uuid] = entity;

        idc.uuid = uuid;
        tc.tag   = tag;

        // Need to construct physics body and call on_init() in case this prefab is
        // being spawned from runtime because the prefab might have NB and RB2D
        // components which need to be properly configured and initialized.
        if (state() != State::Edit && physics_world_) {
            std::scoped_lock guard{ mutex_ };
            {
                auto registry = registry_.lock();
                construct_physics_body(*registry, entity.handle_);
            }

            std::vector<NativeBehaviour*> spawned;
            {
                auto registry = registry_.lock();
                if (auto* nbc = registry->try_get<NativeBehaviourComponent>(entity.handle_))
                    spawned = nbc->behaviours();
            }
            for (NativeBehaviour* bh : spawned) {
                try {
                    bh->on_init();
                }
                catch (const std::exception& ex) {
                    log(Error, "A behaviour exception occured while spawning a prefab: {}", ex.what());
                }
            }
        }

        return entity;
    }

    void Scene::enqueue_for_disposal(Entity entity) noexcept
    {
        std::scoped_lock guard{ mutex_ };
        entities_to_be_disposed_.push_back(entity);
    }

    Entity Scene::entity_by_uuid(UUID uuid) noexcept
    {
        std::scoped_lock guard{ mutex_ };
        if (auto it = uuid_to_entity_.find(uuid); it != uuid_to_entity_.end())
            return it->second;
        return Entity{};
    }

    bool Scene::attach_parent(Entity parent, Entity child) noexcept
    {
        // If parent or child are nil, or parent is child
        if ((!parent || !child) || parent == child)
            return false;

        if (!parent.has_component<HierarchyComponent>())
            parent.add_component<HierarchyComponent>();

        if (!child.has_component<HierarchyComponent>())
            child.add_component<HierarchyComponent>();

        auto& c_hc = child.get_component<HierarchyComponent>();
        auto& p_hc = parent.get_component<HierarchyComponent>();

        Entity cur = parent;
        while (cur) {
            if (cur == child)
                return false;
            if (!cur.has_component<HierarchyComponent>())
                break;
            cur = cur.get_component<HierarchyComponent>().parent;
        }

        // If our parent already has this child
        if (std::find(p_hc.children.begin(), p_hc.children.end(), child) != p_hc.children.end())
            return false;

        // If our child already has a parent
        if (c_hc.parent) {
            auto& cp_hc = c_hc.parent.get_component<HierarchyComponent>();
            cp_hc.children.erase(std::find(cp_hc.children.begin(), cp_hc.children.end(), child));
        }

        mat4 w_child = child.transform().world_mat();

        c_hc.parent = parent;
        p_hc.children.push_back(child);

        // Calculate local_new and decompose to TRS
        mat4       local_new   = glm::inverse(parent.transform().world_mat()) * w_child;
        transform& local_child = child.transform();
        vec3       rot_in_rad;
        math::transform_decompose(local_new, local_child.position, rot_in_rad, local_child.scale);
        local_child.rotation = glm::degrees(rot_in_rad);

        return true;
    }

    bool Scene::detach_parent(Entity parent, Entity child) noexcept
    {
        std::scoped_lock guard{ mutex_ };
        auto             registry = registry_.lock();

        // If parent or child are nil, or parent is child
        if (parent.handle_ == entt::null || child.handle_ == entt::null || parent == child ||
            !registry->valid(parent.handle_) || !registry->valid(child.handle_))
            return false;

        auto* p_hc = registry->try_get<HierarchyComponent>(parent.handle_);
        auto* c_hc = registry->try_get<HierarchyComponent>(child.handle_);

        if (!p_hc || !c_hc)
            return false;

        // If our parent already has this child
        if (auto it = std::find(p_hc->children.begin(), p_hc->children.end(), child); it != p_hc->children.end()) {
            auto& c_trans = registry->get<TransformComponent>(child.handle_);
            mat4  w_child = c_trans.world_mat();

            p_hc->children.erase(it);
            c_hc->parent = Entity{};

            vec3 rot_in_rad;
            math::transform_decompose(w_child, c_trans.position, rot_in_rad, c_trans.scale);
            c_trans.rotation = glm::degrees(rot_in_rad);

            return true;
        }

        return false;
    }

    Scene::BagHandle Scene::create_behaviour_bag(Entity owner) noexcept
    {
        /* clang-format: no inline */
        auto handle = bag_.emplace_back(NBCRecord{
            .owner      = owner,
            .behaviours = {},
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
                NBHandle handle = behaviours_.emplace_back(std::move(bh));
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
    { return (primary_camera_entity_) ? *primary_camera_entity_ : Entity::none(); }

    void Scene::transform_pass()
    {
        std::scoped_lock guard{ mutex_ };
        auto             registry = registry_.lock();

        auto calc_local = [](const vec3& translation, const vec3& rotation, const vec3& scale) -> mat4

        {
            mat4 trs_mat =
                glm::eulerAngleXYZ(glm::radians(rotation.x), glm::radians(rotation.y), glm::radians(rotation.z));
            trs_mat[3] = vec4(translation, 1.0f);
            return glm::scale(trs_mat, scale);
        };

        std::vector<entt::entity> parent_entities;
        auto                      view = registry->view<TransformComponent>();
        for (const auto& [e, tc] : view.each()) {
            if (auto* hc = registry->try_get<HierarchyComponent>(e); hc) {
                if (registry->valid(hc->parent.handle_))
                    continue;
            }

            parent_entities.emplace_back(e);
        }

        for (entt::entity p : parent_entities) {
            std::queue<std::pair<entt::entity, mat4>> equeue;

            equeue.push({ p, mat4{ 1.0f } });

            while (!equeue.empty()) {
                auto [e, world] = equeue.front();
                equeue.pop();

                auto& tc  = registry->get<TransformComponent>(e);
                tc.local_ = calc_local(tc.position, tc.rotation, tc.scale);
                tc.world_ = world * tc.local_;

                if (auto* hc = registry->try_get<HierarchyComponent>(e); hc) {
                    for (Entity c : hc->children)
                        equeue.push({ c.handle_, tc.world_ });
                }
            }
        }
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
                    const auto transform =
                        transform_component.world_mat() * glm::scale(mat4{ 1.0f }, { size.x, size.y, 1.0f });
                    gfx::BatchRenderer2D::render_sprite(renderer_component.sprite(), transform, static_cast<i32>(e));
                }
            }
        }

        // Tiles
        {
            auto registry = registry_.lock();
            auto view     = registry->view<TilemapComponent, TransformComponent>();
            for (const auto& [e, tmc, tc] : view.each()) {
                for (auto& tile : tmc.tiles) {
                    Sprite& sprite = tmc.sprite;
                    sprite.set_size(tmc.grid_size);
                    sprite.set_texture_coords(util::to_rect(tile.atlas, tmc.tile_size.x, tmc.tile_size.y));
                    sprite.set_z_index(tile.layer);

                    auto transform = mat4{ 1.0f };
                    transform      = glm::translate(transform, tile.pos);
                    transform      = glm::scale(transform, vec3{ sprite.size().x, sprite.size().y, 1.0f });
                    gfx::BatchRenderer2D::render_sprite(sprite, transform, static_cast<i32>(e));
                }
            }
        }

        // Tileset Animation
        {
            auto      registry = registry_.lock();
            auto      view     = registry->view<TilesetAnimationComponent, TransformComponent>();
            const f32 dt       = Engine::delta();
            for (auto& e : view) {
                auto&       tac = view.get<TilesetAnimationComponent>(e);
                const auto& tc  = view.get<TransformComponent>(e);

                if (!tac.sprite)
                    continue;
                auto* anim = tac.active_anim();
                if (!anim)
                    continue;

                // Advance playback at the animation's own frame rate.
                if (anim->frame_count > 0 && anim->frame_rate > 0.0f) {
                    const f32 frame_time = 1.0f / anim->frame_rate;
                    anim->accumulator += dt;
                    while (anim->accumulator >= frame_time) {
                        anim->accumulator -= frame_time;

                        switch (anim->playback_mode) {
                            using enum TilesetAnimationComponent::Animation::PlaybackMode;
                            case kNormal: {
                                anim->current_frame = (anim->current_frame + 1) % anim->frame_count;
                            } break;
                            case kReverse: {
                                anim->current_frame = (anim->current_frame + anim->frame_count - 1) % anim->frame_count;
                            } break;
                            case kPingPong: {
                                if (anim->reverse) {
                                    anim->current_frame =
                                        (anim->current_frame + anim->frame_count - 1) % anim->frame_count;

                                    if (anim->current_frame == 0)
                                        anim->reverse = false;
                                } else {
                                    anim->current_frame = (anim->current_frame + 1) % anim->frame_count;

                                    if (anim->current_frame == anim->frame_count - 1)
                                        anim->reverse = true;
                                }
                            } break;
                            case kOneShot: {
                                if (anim->current_frame != anim->frame_count - 1) {
                                    anim->current_frame = (anim->current_frame + 1) % anim->frame_count;
                                }
                            } break;
                            case kOneShotReverse: {
                                if (anim->current_frame != 0) {
                                    anim->current_frame =
                                        (anim->current_frame + anim->frame_count - 1) % anim->frame_count;
                                }
                            } break;
                            case kPlaybackModeSize: break; // sentinel, not a real mode
                        }
                    }

                    auto sprite = tac.sprite;
                    sprite.set_texture_coords(
                        rect{ (anim->starting_tile.x + static_cast<f32>(anim->current_frame)) * tac.grid_size.x,
                              anim->starting_tile.y * tac.grid_size.y, tac.grid_size.x, tac.grid_size.y });

                    vec2 size = sprite.size();
                    if (size.x <= 0.0f || size.y <= 0.0f)
                        size = tac.grid_size;

                    const auto transform = tc.world_mat() * glm::scale(glm::identity<mat4>(), { size.x, size.y, 1.0f });
                    gfx::BatchRenderer2D::render_sprite(sprite, transform, static_cast<i32>(e));
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
                const auto& tc    = primary_camera.get_component<TransformComponent>();
                mat4        trans = tc.world_mat();

                ax::SpatialAttributes attr;
                attr.position = vec3(trans[3]);
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
                    mat4                  trans = view.get<TransformComponent>(e).world_mat();
                    attr.position               = vec3(trans[3]);
                    asc.handle->set_spatial_attributes(attr);
                }
            }
        }
    }

    void Scene::construct_physics_bodies()
    {
        auto registry = registry_.lock();

        const auto rb2d_view = registry->view<TransformComponent, RigidBody2DComponent>();
        for (const auto& e : rb2d_view) {
            construct_physics_body(*registry, e);
            registry->on_destroy<RigidBody2DComponent>().connect<&Scene::destroy_physics_body>(*this);
        }
    }

    void Scene::construct_physics_body(entt::registry& registry, const entt::entity entity)
    {
        auto* rb2d = registry.try_get<RigidBody2DComponent>(entity);
        if (!rb2d || rb2d->runtime_body)
            return;

        const auto& trans = registry.get<TransformComponent>(entity);

        const mat4 world = trans.world_mat();

        // In a TRS our translation is always on the last column (OpenGL is column-major)
        const vec2 world_translation = vec2(world[3]);

        // Because Scale gets applied to our rotation matrix and the values of the rotation matricies are always
        // normalized [-1,1], we can just calculate the length of the rotation coordinates to restore the scale back
        const vec2 world_scale = vec2(glm::length(world[0]), glm::length(world[1]));

        // This is basically the inverse operation of a rotation matrix but we use atan2() (tan shows us the relation
        // between sin and cos) because we lose rotation information and atan2 takes in a horiztonal and a vertical
        // coordinate and depending on the signs TLDR: of these coordinates returns the correct angle.
        // TLDR: cos(45) = 0.7 as well as cos(-45) = 0.7; Now was it -45deg or 45deg? Atan2 asnwers that.
        const f32 world_rotation_z = glm::atan2(world[0].y, world[0].x); // In radians because Box2D wants radians, we
                                                                         // should also switch to radians only show
                                                                         // degress in the editor.
        // Then we'll switch to quats when 3D support gets added eventually.

        b2BodyDef body_def;
        body_def.position.Set(world_translation.x * physics_properties_.scaling_factor,
                              world_translation.y * physics_properties_.scaling_factor);
        body_def.type           = util::to_b2_type(rb2d->body_type);
        body_def.angularDamping = rb2d->angular_damping;
        body_def.linearDamping  = rb2d->linear_damping;
        body_def.bullet         = rb2d->high_velocity;
        body_def.fixedRotation  = rb2d->fixed_rotation;
        body_def.gravityScale   = rb2d->gravity_scale;
        body_def.enabled        = rb2d->enabled;
        body_def.angle          = world_rotation_z;
        auto* b2_body           = physics_world_->CreateBody(&body_def);

        rb2d->runtime_body = b2_body;

        if (const auto* collider = registry.try_get<BoxCollider2DComponent>(entity)) {
            b2PolygonShape shape;
            shape.SetAsBox(collider->size.x * world_scale.x * physics_properties_.scaling_factor,
                           collider->size.y * world_scale.y * physics_properties_.scaling_factor,
                           util::to_b2_vec2(collider->offset * physics_properties_.scaling_factor), 0.0f);

            b2FixtureDef fixture_def;
            fixture_def.shape                = &shape;
            fixture_def.density              = collider->physics_material.density_;
            fixture_def.friction             = collider->physics_material.friction_;
            fixture_def.restitution          = collider->physics_material.restitution_;
            fixture_def.restitutionThreshold = collider->physics_material.restitution_threshold_;

            b2_body->CreateFixture(&fixture_def);
        }

        if (const auto* collider = registry.try_get<CircleCollider2DComponent>(entity)) {
            b2CircleShape shape;
            shape.m_p.Set(collider->offset.x * physics_properties_.scaling_factor,
                          collider->offset.y * physics_properties_.scaling_factor);
            shape.m_radius = collider->radius * physics_properties_.scaling_factor;

            b2FixtureDef fixture_def;
            fixture_def.shape                = &shape;
            fixture_def.density              = collider->physics_material.density_;
            fixture_def.friction             = collider->physics_material.friction_;
            fixture_def.restitution          = collider->physics_material.restitution_;
            fixture_def.restitutionThreshold = collider->physics_material.restitution_threshold_;

            b2_body->CreateFixture(&fixture_def);
        }
    }

    void Scene::destroy_physics_body(entt::registry& registry, entt::entity entity) noexcept
    {
        auto& rb2d = registry.get<RigidBody2DComponent>(entity);
        if (rb2d.runtime_body && physics_world_) {
            physics_world_->DestroyBody(reinterpret_cast<b2Body*>(rb2d.runtime_body));
            rb2d.runtime_body = nullptr;
        }
    }

    void Scene::on_editor_init([[maybe_unused]] scene::EditorCamera& camera)
    {
    }

    void Scene::on_runtime_start()
    {
        // Transform pass
        transform_pass();

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

        attach_pending_behaviours();

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
                            mat4                  trans = asc_view.get<TransformComponent>(e).world_mat();
                            attr.position               = vec3(trans[3]);
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
        // Transform pass
        transform_pass();

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
        // Transform pass
        transform_pass();

        // Render
        {
            gfx::BatchRenderer2D::begin(camera);
            render_sprites();
            gfx::BatchRenderer2D::end(end_shader);
        }
    }

    void Scene::on_simulation_update([[maybe_unused]] const f32 dt, scene::EditorCamera& camera)
    {
        // Transform pass
        transform_pass();

        // Render
        {
            gfx::BatchRenderer2D::begin(camera);
            render_sprites();
            gfx::BatchRenderer2D::end();
        }
    }

    void Scene::on_runtime_update(const f32 dt)
    {
        // Transform pass
        transform_pass();

        // Render
        {
            // Grab primary camera
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

            // Render our sprites if there's a camera
            // FIXME: Sprites should always get rendered despite the abscence of a camera
            if (primary_camera_entity_) {
                const auto& cc = primary_camera_entity_->get_component<CameraComponent>();
                const auto& tc = primary_camera_entity_->get_component<TransformComponent>();

                gfx::BatchRenderer2D::begin(cc.camera, tc);
                render_sprites();
                gfx::BatchRenderer2D::end();
            }
        }

        // Native behaviour update
        {
            bool stop = false;
            {
                std::scoped_lock guard{ mutex_ };

                // NOTE: Temporary Fix: Need to create a snapshot of behaviours here because
                // behaviours can spawn prefabs with their own behaviour(s) which will
                // cause our behaviours_ dense vector to be invalidated
                // TODO: This is not acceptable for a tight game loop, in the future we'll need to
                // have separate to_be_attached_ and to_be_disposed_ lists holding their respective
                // behaviours as the names suggest.
                std::vector<NativeBehaviour*> behaviours;
                for (Box<NativeBehaviour>& bh : behaviours_)
                    behaviours.push_back(bh.get());

                for (NativeBehaviour* bh : behaviours) {
                    assert(bh);

                    try {
                        bh->on_update(dt);
                    }
                    catch (const std::exception& ex) {
                        log(Error, "A behaviour exception occured: {}", ex.what());
                        stop = true;
                    }
                }

                for (Entity entity : entities_to_be_disposed_)
                    remove_entity(entity);
                entities_to_be_disposed_.clear();
            }

            if (stop) {
                if (state() == Scene::State::Play) {
                    set_state(Scene::State::Edit);
                    on_runtime_stop();
                } else if (state() == Scene::State::Simulate) {
                    set_state(Scene::State::Edit);
                    on_simulation_stop();
                }
            }
        }

        render_audio();
    }

    void Scene::on_fixed_update(Scene& self) noexcept
    {
        CX_DEBUG_PROFILE_SCOPE("Scene::on_fixed_update")

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
                    std::scoped_lock guard{ self.mutex_ };

                    // Snapshot for the same reason as on_runtime_update: callbacks may
                    // spawn prefabs and grow behaviours_ mid-iteration.
                    // TODO: Overhaul here too
                    std::vector<NativeBehaviour*> behaviours;
                    for (Box<NativeBehaviour>& bh : self.behaviours_)
                        behaviours.push_back(bh.get());

                    for (NativeBehaviour* bh : behaviours) {
                        assert(bh);

                        try {
                            bh->on_fixed_update(frame_interval);
                        }
                        catch (const std::exception& ex) {
                            self.log(Error, "A behaviour exception occured: {}", ex.what());

                            // Cannot call on_runtime_stop() from this thread (it joins this
                            // very thread); drop to Edit so the loop exits and the editor
                            // performs the actual cleanup.
                            self.set_state(Scene::State::Edit);
                        }
                    }
                }

                // Physics
                if (self.state_.load() != State::Edit) {
                    std::scoped_lock guard{ self.mutex_ };
                    auto             registry = self.registry_.lock();
                    auto             view     = registry->view<TransformComponent, RigidBody2DComponent>();
                    self.physics_world_->Step(frame_interval, self.physics_properties_.velocity_iterations,
                                              self.physics_properties_.position_iterations);
                    for (auto& e : view) {
                        auto& trans = view.get<TransformComponent>(e);
                        auto& rb2d  = view.get<RigidBody2DComponent>(e);

                        // Bodies added at runtime outside the prefab path are built here
                        if (!rb2d.runtime_body)
                            self.construct_physics_body(*registry, e);

                        auto* b2_body = reinterpret_cast<b2Body*>(rb2d.runtime_body);
                        if (!b2_body)
                            continue;
                        const auto& b2_pos = b2_body->GetPosition();

                        const vec2 b2_world{ b2_pos.x / self.physics_properties_.scaling_factor,
                                             b2_pos.y / self.physics_properties_.scaling_factor };
                        const f32  b2_angle_world = math::to_degf(b2_body->GetAngle());

                        if (auto* hc = registry->try_get<HierarchyComponent>(e); hc && hc->parent) {
                            const mat4 parent_world = registry->get<TransformComponent>(hc->parent.handle_).world_mat();
                            const vec3 local_world =
                                glm::inverse(parent_world) * vec4(b2_world, trans.position.z, 1.0f);
                            const f32 parent_angle_world_rad = glm::atan2(parent_world[0].y, parent_world[0].x);

                            trans.position.x = local_world.x;
                            trans.position.y = local_world.y;
                            trans.rotation.z = b2_angle_world - glm::degrees(parent_angle_world_rad);
                        } else {
                            trans.position.x = b2_world.x;
                            trans.position.y = b2_world.y;
                            trans.rotation.z = b2_angle_world;
                        }
                    }
                }

                lag -= frame_interval;
            }

            // Upper bound for the outer loop.
            if (frame_time < max_frame_interval)
                std::this_thread::sleep_for(std::chrono::duration<f32>(max_frame_interval - frame_time));
        }
    }

    void Scene::attach_pending_behaviours() noexcept
    {
        std::scoped_lock guard{ mutex_ };
        auto             registry = registry_.lock();
        auto             view     = registry->view<NativeBehaviourComponent>();
        for (auto& e : view)
            view.get<NativeBehaviourComponent>(e).attach_pending();
    }

    void Scene::archive(Archive& ar)
    {
        ar("name", name_);

        IArchiveBackend& b = ar.backend();

        if (ar.saving()) {
            std::vector<entt::entity> entities;
            {
                auto registry = registry_.lock();
                for (const auto& e : registry->view<entt::entity>())
                    entities.push_back(e);
            }

            usize entity_total = entities.size();
            b.begin_array("entities", entity_total);
            for (const auto& entity : entities) {
                Entity cx_entity{ entity, this };
                serialize(ar, cx_entity);
            }
            b.end_array();
        } else {
            usize entity_total = 0;
            b.begin_array("entities", entity_total);
            for (usize i = 0; i < entity_total; ++i) {
                Entity entity = create_entity();
                serialize(ar, entity);
            }
            b.end_array();

            // Need to update uuid_to_entity_ map now that all entities are present with their real UUIDs
            {
                uuid_to_entity_.clear();

                auto        registry = registry_.lock();
                const auto& view     = registry->view<const IDComponent>();

                for (const auto& [e, v] : view.each()) {
                    uuid_to_entity_[v.uuid] = Entity{ e, this };
                }
            }

            // We need to resolve HierarchyComponents now that all the entities are complete
            {
                auto        registry = registry_.lock();
                const auto& view     = registry->view<HierarchyComponent>();

                for (const auto& [e, hc] : view.each()) {
                    hc.resolve_pending();
                }
            }
        }
    }

    void serialize(Archive& ar, Entity& entity)
    {
        if (!entity)
            return;

        IArchiveBackend& b = ar.backend();

        if (ar.saving()) {
            std::vector<Component*> components       = collect_components(AllComponents{}, entity);
            usize                   total_components = components.size();
            b.begin_array("components", total_components);
            for (Component* c : components) {
                b.begin_object({});
                c->archive(ar);
                b.end_object();
            }
            b.end_array();
        } else {
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
        }
    }
} // namespace codex
