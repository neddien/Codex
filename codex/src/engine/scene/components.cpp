#include "public/components.inl"

#include <engine/core/engine.h>
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

    void TagComponent::serialize_impl(ISerializationNode& node) const
    {
        node.write("tag", tag);
    }

    void TagComponent::deserialize_impl(const ISerializationNode& node)
    {
        node.read("tag", tag);
    }

    TransformComponent::TransformComponent(const Vector3f position, const Vector3f rotation, const Vector3f scale)
        : position{ position }
        , rotation{ rotation }
        , scale{ scale }
    {
    }

    void TransformComponent::serialize_impl(ISerializationNode& node) const
    {
        node.write("position", position);
        node.write("rotation", rotation);
        node.write("scale", scale);
    }

    void TransformComponent::deserialize_impl(const ISerializationNode& node)
    {
        node.read("position", position);
        node.read("rotation", rotation);
        node.read("scale", scale);
    }

    SpriteRendererComponent::SpriteRendererComponent(Sprite sprite)
        : sprite_{ std::move(sprite) }
    {
    }

    void SpriteRendererComponent::serialize_impl(ISerializationNode& node) const
    {
        sprite_.serialize(node);
    }

    void SpriteRendererComponent::deserialize_impl(const ISerializationNode& node)
    {
        sprite_.deserialize(node);
    }

    NativeBehaviourComponent::NativeBehaviourComponent() noexcept = default;

    NativeBehaviourComponent::NativeBehaviourComponent(NativeBehaviourComponent&& other) noexcept
        : handle_{ std::exchange(other.handle_, Scene::BagHandle::invalid_id()) }
        , pending_{ std::move(other.pending_) }
    {
    }

    NativeBehaviourComponent& NativeBehaviourComponent::operator=(NativeBehaviourComponent&& other) noexcept
    {
        if (this != &other) {
            handle_        = other.handle_;
            other.handle_  = Scene::BagHandle::invalid_id();
            other.pending_ = std::move(other.pending_);
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
            pending_.emplace(type_name);
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

    void NativeBehaviourComponent::serialize_impl(ISerializationNode& node) const
    {
        auto& scripts = node.begin_array("attached_scripts");
        for (auto handle : scene()->behaviours(handle_)) {
            const NativeBehaviour* bh = scene()->behaviour(handle);
            scripts.add_array_element().write("name", bh->type_info().name());
        }

        auto pending = std::exchange(pending_, {});
        for (auto type_name : pending)
            scripts.add_array_element().write("name", type_name);

        node.end_array();
    }

    void NativeBehaviourComponent::deserialize_impl(const ISerializationNode& node)
    {
        auto& scripts = node.array("attached_scripts");
        scripts.for_each_array_element(
            [this](const ISerializationNode& element)
            {
                std::string name;
                if (element.read("name", name)) {
                    // By default pend all behaviours, we cannot be sure that NBMan has been loaded yet.
                    log(Info, "Deser: Pending behaviour: {}", name);
                    pending_.emplace(name);
                }
            });
    }

    void NativeBehaviourComponent::attach_pending() noexcept
    {
        log(Info, "Attaching pending behaviours: {}", pending_.size());
        auto pending = std::exchange(pending_, {});
        for (const auto& type_name : pending) {
            if (!NBMan::is_type_registered(type_name)) {
                // Re-pend this behaviour.
                pending_.emplace(type_name);
                log(Warn, "Pending behaviour still hasn't been found: {}", type_name);
            }

            scene()->create_behaviour(handle_, type_name);
        }
    }

    void CameraComponent::serialize_impl(ISerializationNode& node) const
    {
    }

    void CameraComponent::deserialize_impl(const ISerializationNode& node)
    {
    }

    void RigidBody2DComponent::apply_force(const Vector2f& force, const std::optional<Vector2f> point) noexcept
    {
        auto* body = reinterpret_cast<b2Body*>(runtime_body);
        body->ApplyForce(util::to_b2_vec2(force), (point) ? util::to_b2_vec2(*point) : body->GetWorldCenter(), true);
    }

    void RigidBody2DComponent::serialize_impl(ISerializationNode& node) const
    {
        node.write("body_type", enum_name(body_type));
        node.write("fixed_rotation", fixed_rotation);
        node.write("linear_damping", linear_damping);
        node.write("angular_damping", angular_damping);
        node.write("high_velocity", high_velocity);
        node.write("enabled", enabled);
        node.write("gravity_scale", gravity_scale);
    }

    void RigidBody2DComponent::deserialize_impl(const ISerializationNode& node)
    {
        if (std::string str; node.read("body_type", str))
            if (auto val = enum_from<BodyType>(str))
                body_type = *val;
        node.read("fixed_rotation", fixed_rotation);
        node.read("linear_damping", linear_damping);
        node.read("angular_damping", angular_damping);
        node.read("high_velocity", high_velocity);
        node.read("enabled", enabled);
        node.read("gravity_scale", gravity_scale);
    }

    void RigidBody2DComponent::apply_torque(const f32 torque) noexcept
    {
        auto* body = reinterpret_cast<b2Body*>(runtime_body);
        body->ApplyTorque(torque, true);
    }

    void RigidBody2DComponent::apply_linear_impulse(const Vector2f& impulse, const std::optional<Vector2f> point)
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

    void TilemapComponent::add_tile([[maybe_unused]] const Vector3f pos, [[maybe_unused]] const i32 tileId)
    {
    }

    void TilemapComponent::add_tile(const Vector3f pos, const Vector2f atlas)
    {
        if (auto it = std::find_if(tiles.begin(), tiles.end(),
                                   [&](auto& tile) { return tile.pos == pos && tile.layer == current_layer; });
            it != tiles.end()) {
            it->atlas = atlas;
            it->pos   = pos;
        } else
            tiles.push_back({ .pos = pos, .atlas = atlas, .layer = current_layer });
    }

    void TilemapComponent::remove_tile(const Vector3f pos)
    {
        if (auto it = std::find_if(tiles.begin(), tiles.end(),
                                   [&](auto& tile) { return tile.pos == pos && tile.layer == current_layer; });
            it != tiles.end()) {
            tiles.erase(it);
        }
    }

    void TilemapComponent::serialize_impl(ISerializationNode& node) const
    {
        auto& sprite_node = node.create_child("sprite");
        sprite.serialize(sprite_node);

        auto& tile_arr_node = node.begin_array("tiles");
        for (const auto& e : tiles) {
            auto& node = tile_arr_node.add_array_element();
            node.write("pos", e.pos);
            node.write("atlas", e.atlas);
            node.write("layer", e.layer);
        }
        node.end_array();

        node.write("grid_size", grid_size);
        node.write("tile_size", tile_size);
        node.write("current_tile", current_tile);
        node.write("current_state", enum_name(current_state));
        node.write("current_layer", current_layer);
    }

    void TilemapComponent::deserialize_impl(const ISerializationNode& node)
    {
        auto& sprite_node = node.child("sprite");
        sprite.deserialize(sprite_node);

        auto& tile_arr_node = node.array("tiles");
        tile_arr_node.for_each_array_element(
            [this](const auto& inode)
            {
                Tile tile;
                inode.read("pos", tile.pos);
                inode.read("atlas", tile.atlas);
                inode.read("layer", tile.layer);
                tiles.push_back(std::move(tile));
            });

        node.read("grid_size", grid_size);
        node.read("tile_size", tile_size);
        node.read("current_tile", current_tile);
        if (std::string str; node.read("current_state", str))
            if (auto val = enum_from<State>(str))
                current_state = *val;
        node.read("current_layer", current_layer);
    }

    void IDComponent::serialize_impl(ISerializationNode& node) const
    {
        uuid.serialize(node);
    }

    void IDComponent::deserialize_impl(const ISerializationNode& node)
    {
        uuid.deserialize(node);
    }

    void BoxCollider2DComponent::serialize_impl(ISerializationNode& node) const
    {
        node.write("offset", offset);
        node.write("size", size);

        auto& physmat = node.create_child("physics_material");
        physics_material.serialize(physmat);
    }

    void BoxCollider2DComponent::deserialize_impl(const ISerializationNode& node)
    {
        node.read("offset", offset);
        node.read("size", size);

        auto& physmat = node.child("physics_material");
        physics_material.deserialize(physmat);
    }

    void CircleCollider2DComponent::serialize_impl(ISerializationNode& node) const
    {
        node.write("offset", offset);
        node.write("radius", radius);

        auto& physmat = node.create_child("physics_material");
        physics_material.serialize(physmat);
    }

    void CircleCollider2DComponent::deserialize_impl(const ISerializationNode& node)
    {
        node.read("offset", offset);
        node.read("radius", radius);

        auto& physmat = node.child("physics_material");
        physics_material.deserialize(physmat);
    }

    void GridRendererComponent::serialize_impl(ISerializationNode& node) const
    {
        node.write("cell_size", cell_size);
        node.write("colour", colour);
    }

    void GridRendererComponent::deserialize_impl(const ISerializationNode& node)
    {
        node.read("cell_size", cell_size);
        node.read("colour", colour);
    }

    void TilesetAnimationComponent::serialize_impl(ISerializationNode& node) const
    {
        auto& spritenode = node.create_child("sprite");
        sprite.serialize(spritenode);
        node.write("grid_size", grid_size);

        auto& anims_node = node.begin_array("animations");
        for (const auto& anim : animations) {
            auto& inode = anims_node.add_array_element();
            inode.write("name", anim.name);
            inode.write("starting_tile", anim.starting_tile);
            inode.write("frame_count", anim.frame_count);
            inode.write("frame_rate", anim.frame_rate);
        }
    }

    void TilesetAnimationComponent::deserialize_impl(const ISerializationNode& node)
    {
        auto& spritenode = node.child("sprite");
        sprite.deserialize(spritenode);
        node.read("grid_size", grid_size);

        auto& anims_node = node.array("animations");
        animations.clear();

        anims_node.for_each_array_element(
            [this](const auto& inode)
            {
                Animation anim;
                inode.read("name", anim.name);
                inode.read("starting_tile", anim.starting_tile);
                inode.read("frame_count", anim.frame_count);
                inode.read("frame_rate", anim.frame_rate);

                animations.push_back(std::move(anim));
            });
    }

    void AudioSourceComponent::serialize_impl(ISerializationNode& node) const
    {
        node.write("event_path", event_path);
        node.write("sound_path", sound_path.string());
        node.write("volume", volume);
        node.write("pitch", pitch);
        node.write("loop", loop);
        node.write("play_on_start", play_on_start);
        node.write("is_3d", is_3d);
        node.write("min_distance", min_distance);
        node.write("max_distance", max_distance);

        if (!parameters.empty()) {
            auto& params_node = node.begin_map("parameters");
            for (const auto& [name, value] : parameters) {
                auto& entry = params_node.add_map_entry(name);
                entry.write("value", value);
            }
        }
    }

    void AudioSourceComponent::deserialize_impl(const ISerializationNode& node)
    {
        node.read("event_path", event_path);
        std::string sound_path_str;
        if (node.read("sound_path", sound_path_str))
            sound_path = sound_path_str;
        node.read("volume", volume);
        node.read("pitch", pitch);
        node.read("loop", loop);
        node.read("play_on_start", play_on_start);
        node.read("is_3d", is_3d);
        node.read("min_distance", min_distance);
        node.read("max_distance", max_distance);

        try {
            auto& params_node = node.map("parameters");
            params_node.for_each_map_entry(
                [this](const std::string_view key, const auto& entry)
                {
                    f32 value = 0.0f;
                    entry.read("value", value);
                    parameters[std::string{ key }] = value;
                });
        }
        catch (...) {
            // No parameters section — that's fine.
        }
    }
} // namespace codex
