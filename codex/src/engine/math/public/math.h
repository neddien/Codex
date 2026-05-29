#pragma once

#include <engine/core/public/common_def.h>
#include <engine/core/public/common_third_party_libs.h>

namespace codex::math {
    constexpr f32 PI = 3.14159265358979323f;

    [[nodiscard]] constexpr f32 to_radf(const f32 degree) noexcept
    {
        return degree * (PI / 180.0f);
    }

    [[nodiscard]] constexpr f32 to_degf(const f32 radians) noexcept
    {
        return radians * (180.0f / PI);
    }

    CODEX_API bool transform_decompose(const glm::mat4& transform, glm::vec3& translation, glm::vec3& rotation,
                                       glm::vec3& scale);

    template <typename T>
    [[nodiscard]] constexpr T clamp(const T& value, const T& min, const T& max) noexcept
    {
        return std::max(min, std::min(value, max));
    }

    [[nodiscard]] constexpr f32 clampf(const f32 value, const f32 min, const f32 max) noexcept
    {
        return std::max(min, std::min(value, max));
    }
} // namespace codex::math
