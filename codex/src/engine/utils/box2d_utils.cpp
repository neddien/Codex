#include "box2d_utils.h"

namespace codex::util {
    [[nodiscard]] b2BodyType to_b2_type(const RigidBody2DComponent::BodyType& type) noexcept
    {
        switch (type) {
            using enum RigidBody2DComponent::BodyType;

            case Static: return b2_staticBody;
            case Dynamic: return b2_dynamicBody;
            case Kinematic: return b2_kinematicBody;
        }

        CX_ASSERT(false, "Unknown body type.");
        return b2_staticBody;
    }

    [[nodiscard]] b2Vec3 to_b2_vec3(const Vector3f& vec) noexcept
    {
        return b2Vec3(vec.x, vec.y, vec.z);
    }

    [[nodiscard]] b2Vec2 to_b2_vec2(const Vector2f& vec) noexcept
    {
        return b2Vec2(vec.x, vec.y);
    }
} // namespace codex::util
