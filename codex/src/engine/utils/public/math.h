#ifndef CODEX_UTILS_MATH_H
#define CODEX_UTILS_MATH_H

#include <engine/core/public/geometry.h>

namespace codex::util {
    [[nodiscard]] inline auto to_vec3(const vec2& vec) noexcept
    {
        return vec3{ vec.x, vec.y, 0.0f };
    }

    [[nodiscard]] inline auto to_vec3(const ivec2& vec) noexcept
    {
        return vec3{ static_cast<f32>(vec.x), static_cast<f32>(vec.y), 0.0f };
    }

    [[nodiscard]] inline auto to_ivec3(const vec2& vec) noexcept
    {
        return ivec3{ static_cast<i32>(vec.x), static_cast<i32>(vec.y), 0 };
    }

    [[nodiscard]] inline auto to_ivec3(const ivec2& vec) noexcept
    {
        return ivec3{ vec.x, vec.y, 0 };
    }

    //
    [[nodiscard]] inline auto to_vec2(const vec3& vec) noexcept
    {
        return vec2{ vec.x, vec.y };
    }

    [[nodiscard]] inline auto to_vec2(const ivec3& vec) noexcept
    {
        return vec2{ static_cast<f32>(vec.x), static_cast<f32>(vec.y) };
    }

    [[nodiscard]] inline auto to_ivec2(const vec3& vec) noexcept
    {
        return ivec2{ static_cast<i32>(vec.x), static_cast<i32>(vec.y) };
    }

    [[nodiscard]] inline auto to_ivec2(const ivec3& vec) noexcept
    {
        return ivec2{ vec.x, vec.y };
    }

    [[nodiscard]] inline auto to_rect(const vec2& vec, const f32 w, const f32 h) noexcept
    {
        return rect{ vec.x, vec.y, w, h };
    }

    [[nodiscard]] inline auto to_rect(const ivec2& vec, const f32 w, const f32 h) noexcept
    {
        return rect{ static_cast<f32>(vec.x), static_cast<f32>(vec.y), w, h };
    }

    [[nodiscard]] inline auto to_irect(const vec2& vec, const i32 w, const i32 h) noexcept
    {
        return irect{ static_cast<i32>(vec.x), static_cast<i32>(vec.y), w, h };
    }

    [[nodiscard]] inline auto to_irect(const ivec2& vec, const i32 w, const i32 h) noexcept
    {
        return irect{ vec.x, vec.y, w, h };
    }

    [[nodiscard]] inline auto snap(const vec3& vec, const vec3 cellSize)
    {
        return glm::floor(vec / cellSize) * cellSize;
    }
} // namespace codex::util

#endif // CODEX_UTILITIES_MATH_H
