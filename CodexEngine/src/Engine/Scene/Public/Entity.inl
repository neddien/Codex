#pragma once

#include "Entity.h"

#include "Scene.h"

// Template implementation for Entity.h
// If you want full Entity class then include Entity.inl

namespace codex {
    Entity::operator bool() const noexcept
    {
        return m_Scene && m_Handle != entt::entity{ entt::null } && m_Scene->IsValid(*this);
    }

    template <typename T, typename... TArgs>
        requires(std::is_base_of_v<Component, T>)
    T& Entity::AddComponent(TArgs&&... args)
    {
        CX_ASSERT(m_Scene && !(m_Scene->m_Registry->any_of<T>(m_Handle)), "Entity already has that component.");
        auto& c = m_Scene->m_Registry->emplace<T>(m_Handle, std::forward<TArgs>(args)...);
        // c.m_Parent = Entity(m_Handle, m_Scene);
        c.OnInit();
        return c;
    }

    template <typename T>
        requires(std::is_base_of_v<Component, T>)
    void Entity::RemoveComponent()
    {
        CX_ASSERT(m_Scene && !(m_Scene->m_Registry->any_of<T>(m_Handle)),
                  "Entity does not have the component to remove.");
        m_Scene->m_Registry->remove<T>(m_Handle);
    }

    template <typename T>
        requires(std::is_base_of_v<Component, T>)
    T& Entity::GetComponent()
    {
        CX_ASSERT(m_Scene && !(m_Scene->m_Registry->any_of<T>(m_Handle)),
                  "Entity does not have the component to retrieve.");
        return m_Scene->m_Registry->get<T>(m_Handle);
    }

    template <typename T>
        requires(std::is_base_of_v<Component, T>)
    const T& Entity::GetComponent() const
    {
        return const_cast<Entity*>(this)->GetComponent<T>();
    }

    template <typename T>
        requires(std::is_base_of_v<Component, T>)
    bool Entity::HasComponent() const
    {
        return m_Scene->m_Registry->any_of<T>(m_Handle);
    }
} // namespace codex