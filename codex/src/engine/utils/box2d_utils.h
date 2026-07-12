#ifndef CODEX_UTILS_BOX2D_UTILITIES_H
#define CODEX_UTILS_BOX2D_UTILITIES_H

#include <box2d/box2d.h>

#include <engine/scene/public/components.inl>

namespace codex::util {
    [[nodiscard]] CODEX_API b2BodyType to_b2_type(const RigidBody2DComponent::BodyType& type) noexcept;

    [[nodiscard]] CODEX_API b2Vec3 to_b2_vec3(const vec3& vec) noexcept;

    [[nodiscard]] CODEX_API b2Vec2 to_b2_vec2(const vec2& vec) noexcept;
} // namespace codex::util

#endif // CODEX_UTILS_BOX2D_UTILITIES_H
