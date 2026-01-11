#pragma once

#include "Entity.h"
#include "Scene.h"
#include "Components.h"

// Template implementation for Entity.h
// If you want full Entity class then include Entity.inl

namespace codex {
    inline Entity::operator bool() const noexcept
    {
        return m_Scene && m_Handle != entt::entity{ entt::null } && m_Scene->IsValid(*this);
    }

    template <typename T, typename... TArgs>
        requires(std::is_base_of_v<Component, T>)
    T& Entity::AddComponent(TArgs&&... args)
    {
        CX_ASSERT(!m_Scene->m_Registry->all_of<T>(m_Handle), "Entity already has that component.");
        
        // IDComponent is always the first Component 
        Component* comp = &m_Scene->m_Registry->get<IDComponent>(m_Handle);
        while (comp->m_Next != nullptr)
        {
            comp = comp->m_Next;
        }
        
        auto& c = m_Scene->m_Registry->emplace<T>(m_Handle, std::forward<TArgs>(args)...);
        comp->m_Next = &c;
        c.OnInit(); // TODO: This being called here is questionable
        c.m_Parent = *this;
        return c;
    }

    template <typename T, typename... TArgs>
        requires(std::is_base_of_v<Component, T>)
    T& Entity::AddOrReplaceComponent(TArgs&&... args)
    {
        // IDComponent is always the first Component 
        Component* comp = &m_Scene->m_Registry->get<IDComponent>(m_Handle);
        while (comp->m_Next != nullptr)
        {
            comp = comp->m_Next;
        }

        if (HasComponent<T>()) {
            auto& c = m_Scene->m_Registry->emplace_or_replace<T>(m_Handle, std::forward<TArgs>(args)...);
            comp->m_Next = &c;
            c.OnInit();
            return c;
        }

        auto& c = m_Scene->m_Registry->emplace<T>(m_Handle, std::forward<TArgs>(args)...);
        comp->m_Next = &c;
        c.OnInit();
        c.m_Parent = *this;
        return c;
    }

    template <typename T>
        requires(std::is_base_of_v<Component, T>)
    void Entity::RemoveComponent()
    {
        CX_ASSERT(m_Scene->m_Registry->all_of<T>(m_Handle), "Entity does not have the component to remove.");
        m_Scene->m_Registry->remove<T>(m_Handle);
    }

    template <typename T>
        requires(std::is_base_of_v<Component, T>)
    T& Entity::GetComponent()
    {
        CX_ASSERT(m_Scene->m_Registry->all_of<T>(m_Handle), "Entity does not have the component to retrieve.");
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
        return m_Scene->m_Registry->all_of<T>(m_Handle);
    }
} // namespace codex
