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
} // namespace codex
