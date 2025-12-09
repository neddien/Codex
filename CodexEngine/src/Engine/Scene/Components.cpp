#include "Public/Components.h"

#include <Engine/Core/Application.h>
#include <Engine/Core/Window.h>
#include <Engine/Graphics/DebugDraw.h>
#include <Engine/Utils/Box2DUtils.h>

#include "Public/ECS.h"

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

    TransformComponent::TransformComponent(const Vector3f position, const Vector3f rotation, const Vector3f scale)
        : position(position)
        , rotation(rotation)
        , scale(scale)
    {
    }

    SpriteRendererComponent::SpriteRendererComponent(Sprite sprite)
        : m_Sprite(std::move(sprite))
    {
    }

    NativeBehaviourComponent::NativeBehaviourComponent(const NativeBehaviourComponent& other)
    {
        m_BehaviourList.reserve(other.m_BehaviourList.capacity());
        for (const auto& e : other.m_Behaviours)
            Attach(e.second->Clone());
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

        bh->Serialize();
        // bh->m_Parent             = this->GetParent();
        const std::string& name = bh->m_SerializedData.begin().key();
        if (!m_Behaviours.contains(name))
        {
            m_BehaviourList.push_back(bh.Get());
            m_Behaviours[name] = std::move(bh);
        }
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

    void RigidBody2DComponent::ApplyForce(const Vector2f& force, const std::optional<Vector2f> point) noexcept
    {
        auto* body = reinterpret_cast<b2Body*>(runtimeBody);
        body->ApplyForce(utils::ToB2Vec2(force), (point) ? utils::ToB2Vec2(*point) : body->GetWorldCenter(), true);
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
} // namespace codex
