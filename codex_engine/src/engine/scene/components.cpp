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
    {
        tag = "default tag";
    }

    TagComponent::TagComponent(const std::string_view tag)
        : tag(std::string(tag))
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
        : position(position)
        , rotation(rotation)
        , scale(scale)
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
        : sprite_(std::move(sprite))
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

    NativeBehaviourComponent::~NativeBehaviourComponent() noexcept { dispose_behaviours(); }

    NativeBehaviourComponent::NativeBehaviourComponent(const NativeBehaviourComponent& other)
        : pending_scripts_(other.pending_scripts_)
    {
        behaviour_list_.reserve(other.behaviour_list_.capacity());
        for (const auto& e : other.behaviours_)
            attach(e.second->clone());
    }

    void NativeBehaviourComponent::on_update(const f32 deltaTime)
    {
        // TODO: This guy should NOT be inline, it throws an exception?
        for (auto& e : behaviour_list_)
            e->on_update(deltaTime);
    }

    void NativeBehaviourComponent::on_fixed_update(const f32 deltaTime)
    {
        // TODO: This guy should NOT be inline, it throws an exception?
        for (auto& e : behaviour_list_)
            e->on_fixed_update(deltaTime);
    }

    void NativeBehaviourComponent::serialize_impl(ISerializationNode& node) const
    {
        auto& scripts = node.begin_array("attached_scripts");
        for (const auto& [x, y] : behaviours_) {
            scripts.add_array_element().write("name", x);
        }
    }

    void NativeBehaviourComponent::deserialize_impl(const ISerializationNode& node)
    {
        auto& scripts = node.array("attached_scripts");
        scripts.for_each_array_element(
            [this](const ISerializationNode& element)
            {
                std::string name;
                if (element.read("name", name))
                    pending_scripts_.push_back(std::move(name));
            });
    }

    NativeBehaviourComponent& NativeBehaviourComponent::operator=(const NativeBehaviourComponent& other)
    {
        NativeBehaviourComponent{ other }.swap(*this);
        return *this;
    }

    // TODO: Should be noexcept since we're handling the exceptions here.
    void NativeBehaviourComponent::on_init()
    {
        for (auto& e : behaviour_list_) {
            try {
                e->on_init();
            }
            catch (CodexException& ex) {
                // The behaviour threw an exception, wrap it inside a NativeBehaviourException and re-throw it.
                auto exi             = NativeBehaviourException("NBC failed to initialise Behaviours.");
                exi.inner_exception_ = std::move(ex);
                throw std::move(exi);
            }
        }
    }

    void NativeBehaviourComponent::attach(Box<NativeBehaviour> bh)
    {
        // TODO: This should happen OnScenePlay().
        // Optionally, you could have a OnAttach() or OnConstruct() method
        // that will be called during attachment.

        auto* ptr   = bh.get();
        bh->parent_ = this->parent_;
        auto type   = std::string{ bh->type_info().type_name() };
        if (!behaviours_.contains(type)) {
            behaviours_[std::move(type)] = std::move(bh);
            behaviour_list_.push_back(ptr);
        } else {
            throw DuplicateBehaviourException("Behaviour {} is already attached to this entity.", type);
        }
    }

    Box<NativeBehaviour> NativeBehaviourComponent::detach(const std::string& class_name)
    {
        auto it = behaviours_.find(class_name);
        if (it != behaviours_.end()) {
            auto ptr = std::move(it->second);
            behaviours_.erase(it);
            behaviour_list_.erase(std::remove(behaviour_list_.begin(), behaviour_list_.end(), ptr.get()));
            return ptr;
        } else {
            throw ScriptException("Tried to detach a behaviour ({}) that is not attached "
                                  "on the first place.",
                                  class_name);
        }
        return nullptr;
    }

    void NativeBehaviourComponent::instantiate_behaviour(const std::string& class_name)
    {
        if (behaviours_.contains(class_name)) {
            try {
                behaviours_.at(class_name)->on_init();
            }
            catch (CodexException& ex) {
                // The behaviour threw an exception, wrap it inside a NativeBehaviourException and re-throw it.
                auto exi             = NativeBehaviourException("NBC failed to initialise Behaviours.");
                exi.inner_exception_ = std::move(ex);
                throw std::move(exi);
            }
        } else
            throw ScriptException("Tried to instantiate a non-existent behaviour class {}.", class_name);
    }

    void NativeBehaviourComponent::dispose_behaviours()
    {
        behaviours_.clear();
    }

    void NativeBehaviourComponent::set_parent(const Entity entity) const noexcept
    {
        for (auto* e : behaviour_list_)
            e->parent_ = entity;
    }

    void NativeBehaviourComponent::dispose(const std::string& class_name)
    {
        auto it = behaviours_.find(class_name);
        if (it != behaviours_.end()) {
            auto ptr = std::move(it->second);
            behaviours_.erase(it, behaviours_.end());
            behaviour_list_.erase(std::remove(behaviour_list_.begin(), behaviour_list_.end(), ptr.get()));
        } else {
            throw ScriptException("Tried to dispose a behaviour ({}) that is not attached "
                                  "on first place.",
                                  class_name);
        }
    }

    void NativeBehaviourComponent::attach_pending_scripts()
    {
        auto it = pending_scripts_.begin();
        while (it != pending_scripts_.end()) {
            if (behaviours_.contains(*it)) {
                it = pending_scripts_.erase(it);
                continue;
            }

            auto instance = NBMan::create_instance(*it);
            if (instance) {
                attach(std::move(instance));
                it = pending_scripts_.erase(it);
            } else {
                codex::warn("Failed to attach pending script: {}", *it);
                ++it;
            }
        }
    }

    void NativeBehaviourComponent::save_attached_to_pending()
    {
        pending_scripts_.clear();
        for (const auto& [name, _] : behaviours_)
            pending_scripts_.push_back(name);
        dispose_behaviours();
        behaviour_list_.clear();
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

        auto& anims_node = node.begin_map("animations");
        for (const auto& [x, y] : animations) {
            auto& inode = anims_node.add_map_entry(x);
            inode.write("starting_tile", y.starting_tile);
            inode.write("frame_count", y.frame_count);
            inode.write("frame_rate", y.frame_rate);
        }
    }

    void TilesetAnimationComponent::deserialize_impl(const ISerializationNode& node)
    {
        auto& spritenode = node.child("sprite");
        sprite.deserialize(spritenode);

        auto& anims_node = node.map("animations");
        animations.clear();

        anims_node.for_each_map_entry(
            [this](const std::string_view key, const auto& inode)
            {
                Animation anim;
                inode.read("starting_tile", anim.starting_tile);
                inode.read("frame_count", anim.frame_count);
                inode.read("frame_rate", anim.frame_rate);

                animations[std::string{ key }] = std::move(anim);
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
