#pragma once

#include "components.h"
#include "entity.h"
#include "scene.h"

// Template implementation for Entity.h
// If you want full Entity class then include Entity.inl

namespace codex {
    inline const Scene* Entity::scene() const noexcept
    {
        return scene_;
    }

    inline Scene* Entity::scene() noexcept
    {
        return const_cast<Scene*>(std::as_const(*this).scene());
    }

    inline Entity::operator bool() const noexcept
    {
        return scene_ && handle != entt::entity{ entt::null } && scene_->is_valid(*this);
    }

    template <typename T, typename... TArgs>
        requires(std::is_base_of_v<Component, T>)
    T& Entity::add_component(TArgs&&... args)
    {
        CX_ASSERT(!scene_->registry_->all_of<T>(handle), "Entity already has that component.");

        // IDComponent is always the first component.
        Component* comp = &first_component();
        while (comp->next_)
            comp = comp->next_;

        auto& c     = scene_->registry_->emplace<T>(handle, std::forward<TArgs>(args)...);
        comp->next_ = &c;
        c.parent_   = *this;
        c.on_init(); // TODO: This being called here is questionable
        return c;
    }

    template <typename T, typename... TArgs>
        requires(std::is_base_of_v<Component, T>)
    T& Entity::add_or_replace_component(TArgs&&... args)
    {
        // IDComponent is always the first component.
        Component* comp = &first_component();
        while (comp->next_)
            comp = comp->next_;

        if (has_component<T>()) {
            auto& c     = scene_->registry_->emplace_or_replace<T>(handle, std::forward<TArgs>(args)...);
            comp->next_ = &c;
            c.on_init();
            return c;
        }

        auto& c     = scene_->registry_->emplace<T>(handle, std::forward<TArgs>(args)...);
        comp->next_ = &c;
        c.parent_   = *this;
        c.on_init();
        return c;
    }

    template <typename T>
        requires(std::is_base_of_v<Component, T>)
    void Entity::remove_component()
    {
        CX_ASSERT(scene_->registry_->all_of<T>(handle), "Entity does not have the component to remove.");
        scene_->registry_->remove<T>(handle);
    }

    template <typename T>
        requires(std::is_base_of_v<Component, T>)
    T& Entity::get_component()
    {
        CX_ASSERT(scene_->registry_->all_of<T>(handle), "Entity does not have the component to retrieve.");
        return scene_->registry_->get<T>(handle);
    }

    template <typename T>
        requires(std::is_base_of_v<Component, T>)
    const T& Entity::get_component() const
    {
        return const_cast<Entity*>(this)->get_component<T>();
    }

    template <typename T>
        requires(std::is_base_of_v<Component, T>)
    bool Entity::has_component() const
    {
        return scene_->registry_->all_of<T>(handle);
    }
} // namespace codex
