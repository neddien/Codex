#include "public/components.inl"

#include <engine/core/engine.h>
#include <engine/core/public/serialization_manager.h>
#include <engine/core/window.h>
#include <engine/graphics/debug_draw.h>
#include <engine/native_behaviour/public/native_behaviour_manager.h>
#include <engine/utils/box2d_utils.h>

#include "public/entity.inl"

namespace codex {
    using namespace codex::gfx;

    TagComponent::TagComponent()
        : tag{ "default tag" }
    {
    }

    TagComponent::TagComponent(const std::string_view tag)
        : tag{ tag }
    {
    }

    void TagComponent::archive_impl(Archive& ar)
    { ar("tag", tag); }

    TransformComponent::TransformComponent(const math::transform& transform)
        : math::transform{ transform }
    {
    }

    TransformComponent& TransformComponent::operator=(const math::transform& transform) noexcept
    {
        *this = TransformComponent{ transform };
        return *this;
    }

    void TransformComponent::archive_impl(Archive& ar)
    {
        ar("position", position);
        ar("rotation", rotation);
        ar("scale", scale);
    }

    SpriteRendererComponent::SpriteRendererComponent(Sprite sprite)
        : sprite_{ std::move(sprite) }
    {
    }

    void SpriteRendererComponent::archive_impl(Archive& ar)
    { sprite_.archive(ar); }

    NativeBehaviourComponent::NativeBehaviourComponent() noexcept = default;

    NativeBehaviourComponent::NativeBehaviourComponent(NativeBehaviourComponent&& other) noexcept
        : handle_{ std::exchange(other.handle_, Scene::BagHandle::invalid_id()) }
        , pending_{ std::move(other.pending_) }
    {
    }

    NativeBehaviourComponent& NativeBehaviourComponent::operator=(NativeBehaviourComponent&& other) noexcept
    {
        if (this != &other) {
            handle_       = other.handle_;
            other.handle_ = Scene::BagHandle::invalid_id();
            pending_      = std::move(other.pending_);
        }
        return *this;
    }

    // NativeBehaviourComponent& NativeBehaviourComponent::operator=(
    //     const NativeBehaviourComponent& other) noexcept = default;

    NativeBehaviourComponent::~NativeBehaviourComponent() noexcept
    {
        // NOTE: Reason why we don't dispose our behaviours here is because NBC's state is entirely owned by
        // NBMan & Scene, when Scene is disposing it will call the Ctor for NBC which will in turn call into
        // a dying scene with half-freed state which is bad!
        // TLDR: This will bite you in the ass later just like it did to me so I'm making the cleanup for the
        // behaviours explicit with ::dispose().
    }

    void NativeBehaviourComponent::on_init()
    {
        assert(parent_.scene());
        handle_ = scene()->create_behaviour_bag(parent_);
        attach_pending();
    }

    NativeBehaviour* NativeBehaviourComponent::attach(const std::string_view type_name) noexcept
    {
        if (!NBMan::is_type_registered(type_name)) {
            pending_.emplace(std::string{ type_name }, std::string{});
            log(Warn, "Behaviour {} not found in the reigstry, adding it to the pending list", type_name);
            return nullptr;
        }

        auto handle = scene()->create_behaviour(handle_, type_name);
        assert(handle != Scene::NBHandle::invalid_id());
        NativeBehaviour* bh = scene()->behaviour(handle);
        assert(bh);
        log(Info, "Attached behaviour: {}", type_name);
        return bh;
    }

    void NativeBehaviourComponent::detach(const std::string_view type_name) noexcept
    {
        const auto handles = scene()->behaviours(handle_);
        for (const auto bhhandle : handles) {
            NativeBehaviour* bh = scene()->behaviour(bhhandle);
            assert(bh);

            if (bh->type_info().name() == type_name)
                scene()->dispose_behaviour(handle_, bhhandle);
        }
    }

    void NativeBehaviourComponent::dispose_behaviours() noexcept
    {
        const auto handles = scene()->behaviours(handle_);
        for (const auto handle : handles)
            scene()->dispose_behaviour(handle_, handle);
    }

    void NativeBehaviourComponent::dispose_and_save_attached_to_pending() noexcept
    {
        const auto handles = scene()->behaviours(handle_);
        for (const auto handle : handles) {
            NativeBehaviour* bh = scene()->behaviour(handle);
            // Capture the behaviour's state so a script hot-reload restores its
            // property values, not just the attachment.
            pending_.insert_or_assign(std::string{ bh->type_info().name() }, SerializationManager::to_json(*bh));
            scene()->dispose_behaviour(handle_, handle);
        }
    }

    NativeBehaviour* NativeBehaviourComponent::behaviour(const std::string_view type_name) noexcept
    {
        const auto handles = scene()->behaviours(handle_);
        for (const auto handle : handles) {
            NativeBehaviour* bh = scene()->behaviour(handle);
            assert(bh);

            if (bh->type_info().name() == type_name)
                return bh;
        }
        return nullptr;
    }

    std::vector<NativeBehaviour*> NativeBehaviourComponent::behaviours() noexcept
    {
        const auto                    handles = scene()->behaviours(handle_);
        std::vector<NativeBehaviour*> vec;
        vec.reserve(handles.size());
        for (const auto handle : handles) {
            NativeBehaviour* bh = scene()->behaviour(handle);
            assert(bh);

            vec.push_back(bh);
        }

        return vec;
    }

    void NativeBehaviourComponent::dispose() noexcept
    {
        if (handle_ != Scene::BagHandle::invalid_id()) {
            dispose_behaviours();
            scene()->dispose_behaviour_bag(handle_);
        }
    }

    void NativeBehaviourComponent::archive_impl(Archive& ar)
    {
        // Scripts are stored as name -> serialized JSON state so property values
        // (including asset references) survive scene saves and the play clone.
        // Both `optional` slots are written on save so the binary stream stays
        // symmetric; JSON simply omits absent keys.
        IArchiveBackend& b = ar.backend();

        if (ar.saving()) {
            std::unordered_map<std::string, std::string> scripts;
            for (auto handle : scene()->behaviours(handle_)) {
                NativeBehaviour* bh = scene()->behaviour(handle);
                scripts.emplace(std::string{ bh->type_info().name() }, SerializationManager::to_json(*bh));
            }
            for (const auto& [type_name, state] : pending_)
                scripts.emplace(type_name, state);

            if (b.optional("scripts", true))
                ar("scripts", scripts);
            (void)b.optional("attached_scripts", false); // legacy slot, no longer written
        } else {
            if (b.optional("scripts", false)) {
                std::unordered_map<std::string, std::string> scripts;
                ar("scripts", scripts);
                for (auto& [name, state] : scripts) {
                    // By default pend all behaviours, we cannot be sure that NBMan has been loaded yet.
                    log(Info, "Deser: Pending behaviour: {}", name);
                    pending_.emplace(name, std::move(state));
                }
            }
            // Scenes saved before script state was serialized carry names only.
            if (b.optional("attached_scripts", false)) {
                std::vector<std::string> names;
                ar("attached_scripts", names);
                for (auto& name : names) {
                    log(Info, "Deser: Pending behaviour (legacy): {}", name);
                    pending_.emplace(std::move(name), std::string{});
                }
            }
        }
    }

    void NativeBehaviourComponent::attach_pending() noexcept
    {
        log(Info, "Attaching pending behaviours: {}", pending_.size());
        auto pending = std::exchange(pending_, {});
        for (auto& [type_name, state] : pending) {
            if (!NBMan::is_type_registered(type_name)) {
                // Re-pend this behaviour.
                pending_.emplace(type_name, std::move(state));
                log(Warn, "Pending behaviour still hasn't been found: {}", type_name);
                continue;
            }

            const auto       handle = scene()->create_behaviour(handle_, type_name);
            NativeBehaviour* bh     = scene()->behaviour(handle);
            if (bh && !state.empty()) {
                try {
                    SerializationManager::from_json(*bh, state);
                }
                catch (const std::exception& ex) {
                    // The script's fields likely changed since the state was saved; keep defaults.
                    log(Warn, "Could not restore state of behaviour {}: {}", type_name, ex.what());
                }
            }
        }
    }

    void CameraComponent::archive_impl([[maybe_unused]] Archive& ar)
    {
    }

    void RigidBody2DComponent::apply_force(const vec2& force, const std::optional<vec2> point) noexcept
    {
        auto* body = reinterpret_cast<b2Body*>(runtime_body);
        body->ApplyForce(util::to_b2_vec2(force), (point) ? util::to_b2_vec2(*point) : body->GetWorldCenter(), true);
    }

    void RigidBody2DComponent::archive_impl(Archive& ar)
    {
        ar("body_type", body_type);
        ar("fixed_rotation", fixed_rotation);
        ar("linear_damping", linear_damping);
        ar("angular_damping", angular_damping);
        ar("high_velocity", high_velocity);
        ar("enabled", enabled);
        ar("gravity_scale", gravity_scale);
    }

    void RigidBody2DComponent::apply_torque(const f32 torque) noexcept
    {
        auto* body = reinterpret_cast<b2Body*>(runtime_body);
        body->ApplyTorque(torque, true);
    }

    void RigidBody2DComponent::apply_linear_impulse(const vec2& impulse, const std::optional<vec2> point)
    {
        auto* body = reinterpret_cast<b2Body*>(runtime_body);
        body->ApplyLinearImpulse(util::to_b2_vec2(impulse), (point) ? util::to_b2_vec2(*point) : body->GetWorldCenter(),
                                 true);
    }

    void RigidBody2DComponent::apply_angular_impulse(const f32 torque)
    {
        auto* body = reinterpret_cast<b2Body*>(runtime_body);
        body->ApplyAngularImpulse(torque, true);
    }

    void TilemapComponent::add_tile([[maybe_unused]] const vec3 pos, [[maybe_unused]] const i32 tileId)
    {
    }

    void TilemapComponent::add_tile(const vec3 pos, const vec2 atlas)
    {
        if (auto it = std::find_if(tiles.begin(), tiles.end(),
                                   [&](auto& tile) { return tile.pos == pos && tile.layer == current_layer; });
            it != tiles.end()) {
            it->atlas = atlas;
            it->pos   = pos;
        } else
            tiles.push_back({ .pos = pos, .atlas = atlas, .layer = current_layer });
    }

    void TilemapComponent::remove_tile(const vec3 pos)
    {
        if (auto it = std::find_if(tiles.begin(), tiles.end(),
                                   [&](auto& tile) { return tile.pos == pos && tile.layer == current_layer; });
            it != tiles.end()) {
            tiles.erase(it);
        }
    }

    void TilemapComponent::archive_impl(Archive& ar)
    {
        ar("sprite", sprite);
        ar("tiles", tiles);
        ar("grid_size", grid_size);
        ar("tile_size", tile_size);
        ar("current_tile", current_tile);
        ar("current_state", current_state);
        ar("current_layer", current_layer);
    }

    IDComponent::IDComponent(UUID uuid) noexcept
        : uuid{ uuid }
    {
    }

    void IDComponent::archive_impl(Archive& ar)
    { uuid.archive(ar); }

    void BoxCollider2DComponent::archive_impl(Archive& ar)
    {
        ar("offset", offset);
        ar("size", size);
        ar("physics_material", physics_material);
    }

    void CircleCollider2DComponent::archive_impl(Archive& ar)
    {
        ar("offset", offset);
        ar("radius", radius);
        ar("physics_material", physics_material);
    }

    void GridRendererComponent::archive_impl(Archive& ar)
    {
        ar("cell_size", cell_size);
        ar("colour", colour);
    }

    void TilesetAnimationComponent::archive_impl(Archive& ar)
    {
        ar("sprite", sprite);
        ar("grid_size", grid_size);
        ar("animations", animations);
        ar.optional("active_animation", active_animation);
    }

    void AudioSourceComponent::archive_impl(Archive& ar)
    {
        ar("event_path", event_path);

        std::string sound_path_str = ar.saving() ? sound_path.string() : std::string{};
        ar("sound_path", sound_path_str);
        if (ar.loading())
            sound_path = sound_path_str;

        ar("volume", volume);
        ar("pitch", pitch);
        ar("loop", loop);
        ar("play_on_start", play_on_start);
        ar("is_3d", is_3d);
        ar("min_distance", min_distance);
        ar("max_distance", max_distance);
        ar.optional("parameters", parameters);
    }

    void HierarchyComponent::resolve_pending() noexcept
    {
        cxassert(parent_, "invalid component: invalid parent entity");
        Scene* scene = parent_.scene();

        children.clear();

        if (pending_parent_) {
            parent          = scene->entity_by_uuid(pending_parent_.value());
            pending_parent_ = std::nullopt;
        }

        for (UUID uuid : pending_children_) {
            Entity ent = scene->entity_by_uuid(uuid);
            if (ent)
                children.push_back(ent);
        }

        pending_children_.clear();
    }

    void HierarchyComponent::archive_impl(Archive& ar)
    {
        cxassert(parent_, "parent of an entity needs to be valid for archiving");

        if (ar.saving()) {
            UUID uuid = UUID{ 0 };
            if (parent)
                uuid = parent.get_component<IDComponent>().uuid;

            ar("parent", uuid);

            usize count = children.size();
            ar.backend().begin_array("children", count);
            for (const Entity& e : children) {
                cxassert(e.has_component<IDComponent>(), "invalid entity: does not have an IDComponent");

                UUID uuid = e.get_component<IDComponent>().uuid;
                ar("uuid", uuid);
            }
            ar.backend().end_array();
        } else {
            UUID uuid;
            ar("parent", uuid);

            if (uuid != UUID{ 0 }) {
                pending_parent_ = uuid;
            }

            pending_children_.clear();

            usize count = 0;
            ar.backend().begin_array("children", count);
            for (usize i = 0; i < count; ++i) {
                UUID uuid;
                ar("uuid", uuid);
                pending_children_.push_back(uuid);
            }
            ar.backend().end_array();
        }
    }
} // namespace codex
