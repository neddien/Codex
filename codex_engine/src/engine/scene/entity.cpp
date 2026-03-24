#include "public/entity.h"
#include "public/entity.inl"

#include "public/components.inl"

namespace codex {
    UUID Entity::uuid() const noexcept
    {
        return get_component<IDComponent>().uuid;
    }

    TransformComponent& Entity::transform() noexcept
    {
        return get_component<TransformComponent>();
    }

    const TransformComponent& Entity::transform() const noexcept
    {
        return get_component<TransformComponent>();
    }

    Component& Entity::first_component() noexcept
    {
        // For now IDComponent is always the first component.
        return static_cast<Component&>(get_component<IDComponent>());
    }

    const Component& Entity::first_component() const noexcept
    {
        return const_cast<Entity*>(this)->first_component();
    }
} // namespace codex
