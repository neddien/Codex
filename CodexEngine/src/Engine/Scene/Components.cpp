#include "Public/Components.inl"

#include <Engine/Core/Application.h>
#include <Engine/Core/Window.h>
#include <Engine/Graphics/DebugDraw.h>
#include <Engine/Utils/Box2DUtils.h>

#include "Public/Entity.inl"

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

    void TagComponent::SerializeImpl(ISerializationNode& node) const
    {
        node.Write("tag", tag);
    }

    void TagComponent::DeserializeImpl(const ISerializationNode& node)
    {
        node.Read("tag", tag);
    }

    TransformComponent::TransformComponent(const Vector3f position, const Vector3f rotation, const Vector3f scale)
        : position(position)
        , rotation(rotation)
        , scale(scale)
    {
    }

    void TransformComponent::SerializeImpl(ISerializationNode& node) const
    {
        node.Write("position", position);
        node.Write("rotation", rotation);
        node.Write("scale", scale);
    }

    void TransformComponent::DeserializeImpl(const ISerializationNode& node)
    {
        node.Read("position", position);
        node.Read("rotation", rotation);
        node.Read("scale", scale);
    }

    SpriteRendererComponent::SpriteRendererComponent(Sprite sprite)
        : m_Sprite(std::move(sprite))
    {
    }

    void SpriteRendererComponent::SerializeImpl(ISerializationNode& node) const
    {
        m_Sprite.Serialize(node);
    }

    void SpriteRendererComponent::DeserializeImpl(const ISerializationNode& node)
    {
        m_Sprite.Deserialize(node);
    }

    NativeBehaviourComponent::NativeBehaviourComponent(const NativeBehaviourComponent& other)
    {
        m_BehaviourList.reserve(other.m_BehaviourList.capacity());
        for (const auto& e : other.m_Behaviours)
            Attach(e.second->Clone());
    }

    void NativeBehaviourComponent::OnUpdate(const f32 deltaTime)
    {
        // TODO: This guy should NOT be inline, it throws an exception?
        for (auto& e : m_BehaviourList)
            e->OnUpdate(deltaTime);
    }

    void NativeBehaviourComponent::OnFixedUpdate(const f32 deltaTime)
    {
        // TODO: This guy should NOT be inline, it throws an exception?
        for (auto& e : m_BehaviourList)
            e->OnFixedUpdate(deltaTime);
    }

    void NativeBehaviourComponent::SerializeImpl(ISerializationNode& node) const
    {
        auto& scripts = node.BeginArray("attached_scripts");
        for (const auto& [x, y] : m_Behaviours)
        {
            scripts.AddArrayElement().Write("name", x);
        }
    }

    void NativeBehaviourComponent::DeserializeImpl(const ISerializationNode& node)
    {
        // Whatafak
    }

    NativeBehaviourComponent& NativeBehaviourComponent::operator=(const NativeBehaviourComponent& other)
    {
        NativeBehaviourComponent{ other }.Swap(*this);
        return *this;
    }

    // TODO: Should be noexcept since we're handling the exceptions here.
    void NativeBehaviourComponent::OnInit()
    {
        for (auto& [k, v] : m_Behaviours)
        {
            try
            {
                v->OnInit();
            }
            catch (CodexException& ex)
            {
                // The behaviour threw an exception, wrap it inside a NativeBehaviourException and re-throw it.
                auto exi             = cx_except(NativeBehaviourException, "NBC failed to initialise Behaviours.");
                exi.m_InnerException = std::move(ex);
                throw std::move(exi);
            }
        }
    }

    void NativeBehaviourComponent::Attach(mem::Box<NativeBehaviour> bh)
    {
        // TODO: This should happen OnScenePlay().
        // Optionally, you could have a OnAttach() or OnConstruct() method
        // that will be called during attachment.
        // bh->OnInit();

        // bh->Serialize();
        // bh->m_Parent             = this->GetParent();
        /*const std::string& name = bh->m_SerializedData.begin().key();
        if (!m_Behaviours.contains(name))
        {
            m_BehaviourList.push_back(bh.Get());
            m_Behaviours[name] = std::move(bh);
        }*/
    }

    mem::Box<NativeBehaviour> NativeBehaviourComponent::Detach(const std::string& className)
    {
        auto it = m_Behaviours.find(className);
        if (it != m_Behaviours.end())
        {
            auto ptr = std::move(it->second);
            m_Behaviours.erase(it);
            m_BehaviourList.erase(std::remove(m_BehaviourList.begin(), m_BehaviourList.end(), ptr.Get()));
            return ptr;
        }
        else
        {
            cx_throw(ScriptException,
                     "Tried to detach a behaviour ({}) that is not attached "
                     "on the first place.",
                     className);
        }
        return nullptr;
    }

    void NativeBehaviourComponent::InstantiateBehaviour(const std::string& className)
    {
        if (m_Behaviours.contains(className))
        {
            try
            {
                m_Behaviours.at(className)->OnInit();
            }
            catch (CodexException& ex)
            {
                // The behaviour threw an exception, wrap it inside a NativeBehaviourException and re-throw it.
                auto exi             = cx_except(NativeBehaviourException, "NBC failed to initialise Behaviours.");
                exi.m_InnerException = std::move(ex);
                throw std::move(exi);
            }
        }
        else
            cx_throw(ScriptException, "Tried to instantiate a non-existent behaviour class {}.", className);
    }

    void NativeBehaviourComponent::DisposeBehaviours()
    {
        m_Behaviours.clear();
        m_BehaviourList.clear();
    }

    void NativeBehaviourComponent::SetParent(const Entity entity) const noexcept
    {
        for (auto& e : m_BehaviourList)
            e->m_Parent = entity;
    }

    void NativeBehaviourComponent::Dispose(const std::string& className)
    {
        auto it = m_Behaviours.find(className);
        if (it != m_Behaviours.end())
        {
            m_Behaviours.erase(it, m_Behaviours.end());
        }
        else
        {
            cx_throw(ScriptException,
                     "Tried to dispose a behaviour ({}) that is not attach "
                     "on first place.",
                     className);
        }
    }

    void CameraComponent::SerializeImpl(ISerializationNode& node) const
    {
    }

    void CameraComponent::DeserializeImpl(const ISerializationNode& node)
    {
    }

    void RigidBody2DComponent::ApplyForce(const Vector2f& force, const std::optional<Vector2f> point) noexcept
    {
        auto* body = reinterpret_cast<b2Body*>(runtimeBody);
        body->ApplyForce(utils::ToB2Vec2(force), (point) ? utils::ToB2Vec2(*point) : body->GetWorldCenter(), true);
    }

    void RigidBody2DComponent::SerializeImpl(ISerializationNode& node) const
    {
        node.Write("body_type", static_cast<u32>(bodyType));
        node.Write("fixed_rotation", fixedRotation);
        node.Write("linear_damping", linearDamping);
        node.Write("angular_damping", angularDamping);
        node.Write("high_velocity", highVelocity);
        node.Write("enabled", enabled);
        node.Write("gravity_scale", gravityScale);
    }

    void RigidBody2DComponent::DeserializeImpl(const ISerializationNode& node)
    {
        node.Read("body_type", reinterpret_cast<u32&>(bodyType));
        node.Read("fixed_rotation", fixedRotation);
        node.Read("linear_damping", linearDamping);
        node.Read("angular_damping", angularDamping);
        node.Read("high_velocity", highVelocity);
        node.Read("enabled", enabled);
        node.Read("gravity_scale", gravityScale);
    }

    void RigidBody2DComponent::ApplyTorque(const f32 torque) noexcept
    {
        auto* body = reinterpret_cast<b2Body*>(runtimeBody);
        body->ApplyTorque(torque, true);
    }

    void RigidBody2DComponent::ApplyLinearImpulse(const Vector2f& impulse, const std::optional<Vector2f> point)
    {
        auto* body = reinterpret_cast<b2Body*>(runtimeBody);
        body->ApplyLinearImpulse(utils::ToB2Vec2(impulse), (point) ? utils::ToB2Vec2(*point) : body->GetWorldCenter(),
                                 true);
    }

    void RigidBody2DComponent::ApplyAngularImpulse(const f32 torque)
    {
        auto* body = reinterpret_cast<b2Body*>(runtimeBody);
        body->ApplyAngularImpulse(torque, true);
    }

    void TilemapComponent::AddTile([[maybe_unused]] const Vector3f pos, [[maybe_unused]] const i32 tileId)
    {
    }

    void TilemapComponent::AddTile(const Vector3f pos, const Vector2f atlas)
    {
        if (auto it = std::find_if(tiles.begin(), tiles.end(),
                                   [&](auto& tile) { return tile.pos == pos && tile.layer == currentLayer; });
            it != tiles.end())
        {
            it->atlas = atlas;
            it->pos   = pos;
        }
        else
            tiles.push_back({ .pos = pos, .atlas = atlas, .layer = currentLayer });
    }

    void TilemapComponent::RemoveTile(const Vector3f pos)
    {
        if (auto it = std::find_if(tiles.begin(), tiles.end(),
                                   [&](auto& tile) { return tile.pos == pos && tile.layer == currentLayer; });
            it != tiles.end())
        {
            tiles.erase(it);
        }
    }

    void TilemapComponent::SerializeImpl(ISerializationNode& node) const
    {
        auto& sprite_node = node.CreateChild("sprite");
        sprite.Serialize(sprite_node);

        auto& tile_arr_node = node.BeginArray("tiles");
        for (const auto& e : tiles)
        {
            auto& node = tile_arr_node.AddArrayElement();
            node.Write("pos", e.pos);
            node.Write("atlas", e.atlas);
            node.Write("layer", e.layer);
        }

        node.Write("grid_size", gridSize);
        node.Write("tile_size", tileSize);
        node.Write("current_tile", currentTile);
        node.Write("current_state", static_cast<u32>(currentState));
        node.Write("current_layer", currentLayer);
    }

    void TilemapComponent::DeserializeImpl(const ISerializationNode& node)
    {
        auto& sprite_node = node.GetChild("sprite");
        sprite.Deserialize(sprite_node);

        auto& tile_arr_node = node.GetArray("tiles");
        tile_arr_node.ForEachArrayElement(
            [this](const auto& inode)
            {
                Tile tile;
                inode.Read("pos", tile.pos);
                inode.Read("atlas", tile.atlas);
                inode.Read("layer", tile.layer);
                tiles.push_back(std::move(tile));
            });

        node.Read("grid_size", gridSize);
        node.Read("tile_size", tileSize);
        node.Read("current_tile", currentTile);
        node.Read("current_state", reinterpret_cast<u32&>(currentState));
        node.Read("current_layer", currentLayer);
    }

    void IDComponent::SerializeImpl(ISerializationNode& node) const
    {
        node.Write("uuid", static_cast<u64>(uuid));
    }

    void IDComponent::DeserializeImpl(const ISerializationNode& node)
    {
        u64 id;
        node.Read("uuid", id);
        uuid = UUID(id);
    }

    void BoxCollider2DComponent::SerializeImpl(ISerializationNode& node) const
    {
        node.Write("offset", offset);
        node.Write("size", size);

        auto& physmat = node.CreateChild("physics_material");
        physicsMaterial.Serialize(physmat);
    }

    void BoxCollider2DComponent::DeserializeImpl(const ISerializationNode& node)
    {
        node.Read("offset", offset);
        node.Read("size", size);

        auto& physmat = node.GetChild("physics_material");
        physicsMaterial.Deserialize(physmat);
    }

    void CircleCollider2DComponent::SerializeImpl(ISerializationNode& node) const
    {
        node.Write("offset", offset);
        node.Write("radius", radius);

        auto& physmat = node.CreateChild("physics_material");
        physicsMaterial.Serialize(physmat);
    }

    void CircleCollider2DComponent::DeserializeImpl(const ISerializationNode& node)
    {
        node.Read("offset", offset);
        node.Read("radius", radius);

        auto& physmat = node.GetChild("physics_material");
        physicsMaterial.Deserialize(physmat);
    }

    void GridRendererComponent::SerializeImpl(ISerializationNode& node) const
    {
        node.Write("cell_size", cellSize);
        node.Write("colour", colour);
    }

    void GridRendererComponent::DeserializeImpl(const ISerializationNode& node)
    {
        node.Read("cell_size", cellSize);
        node.Read("colour", colour);
    }

    void TilesetAnimationComponent::SerializeImpl(ISerializationNode& node) const
    {
        auto& sprite_node = node.CreateChild("sprite");
        sprite.Serialize(sprite_node);

        auto& anims_node = node.BeginMap("animations");
        for (const auto& [x, y] : animations)
        {
            auto& inode = anims_node.AddMapEntry(x);
            inode.Write("starting_tile", y.startingTile);
            inode.Write("frame_count", y.frameCount);
            inode.Write("frame_rate", y.frameRate);
        }
    }

    void TilesetAnimationComponent::DeserializeImpl(const ISerializationNode& node)
    {
        auto& sprite_node = node.GetChild("sprite");
        sprite.Deserialize(sprite_node);

        auto& anims_node = node.GetMap("animations");
        animations.clear();

        anims_node.ForEachMapEntry(
            [this](const std::string_view key, const auto& inode)
            {
                Animation anim;
                inode.Read("starting_tile", anim.startingTile);
                inode.Read("frame_count", anim.frameCount);
                inode.Read("frame_rate", anim.frameRate);

                animations[std::string{ key }] = std::move(anim);
            });
    }
} // namespace codex
