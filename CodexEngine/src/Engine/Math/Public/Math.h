#pragma once

#include <Engine/Core/Public/CommonDef.h>

#include <glm/ext/matrix_clip_space.hpp> // glm::perspective
#include <glm/ext/matrix_transform.hpp>  // glm::translate, glm::rotate, glm::scale
#include <glm/ext/scalar_constants.hpp>  // glm::pi
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp> // glm::value_ptr
#include <glm/gtx/matrix_decompose.hpp>
#include <glm/gtx/rotate_vector.hpp> // glm::rotate(glm::vecX)
#include <glm/mat4x4.hpp>            // glm::mat4
#include <glm/vec2.hpp>
#include <glm/vec3.hpp> // glm::vec3
#include <glm/vec4.hpp> // glm::vec4

namespace codex::math {
    constexpr f32 PI = 3.14159265358979323f;

    [[nodiscard]] constexpr f32 ToRadf(const f32 degree) noexcept
    {
        return degree * (PI / 180.0f);
    }

    [[nodiscard]] constexpr f32 ToDegf(const f32 radians) noexcept
    {
        return radians * (180.0f / PI);
    }

    bool TransformDecompose(const glm::mat4& transform, glm::vec3& translation, glm::vec3& rotation, glm::vec3& scale);

    template <typename T>
    [[nodiscard]] constexpr T Clamp(const T& value, const T& min, const T& max) noexcept
    {
        return std::max(min, std::min(value, max));
    }

    [[nodiscard]] constexpr f32 Clampf(const f32 value, const f32 min, const f32 max) noexcept
    {
        return std::max(min, std::min(value, max));
    }
} // namespace codex::math
