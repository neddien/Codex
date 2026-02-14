#include "Public/Entity.h"
#include "Public/Entity.inl"

#include "Public/Components.inl"

namespace codex {
    UUID Entity::GetUUID() const noexcept
    {
        return GetComponent<IDComponent>().uuid;
    }

    TransformComponent& Entity::GetTransform() noexcept
    {
        return GetComponent<TransformComponent>();
    }

    const TransformComponent& Entity::GetTransform() const noexcept
    {
        return GetComponent<TransformComponent>();
    }

    Component& Entity::GetFirstComponent() noexcept
    {
        // For now IDComponent is always the first component.
        return static_cast<Component&>(GetComponent<IDComponent>());
    }

    const Component& Entity::GetFirstComponent() const noexcept
    {
        return const_cast<Entity*>(this)->GetFirstComponent();
    }
} // namespace codex
