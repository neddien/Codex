#ifndef CODEX_UTILS_MATH_H
#define CODEX_UTILS_MATH_H

#include <engine/core/public/geometry.h>

namespace codex::utils {
    [[nodiscard]] inline auto to_vec3f(const Vector2f& vec) noexcept
    {
        return Vector3f{ vec.x, vec.y, 0.0f };
    }

    [[nodiscard]] inline auto to_vec3f(const Vector2& vec) noexcept
    {
        return Vector3f{ static_cast<f32>(vec.x), static_cast<f32>(vec.y), 0.0f };
    }

    [[nodiscard]] inline auto to_vec3(const Vector2f& vec) noexcept
    {
        return Vector3{ static_cast<i32>(vec.x), static_cast<i32>(vec.y), 0 };
    }

    [[nodiscard]] inline auto to_vec3(const Vector2& vec) noexcept
    {
        return Vector3{ vec.x, vec.y, 0 };
    }

    //
    [[nodiscard]] inline auto to_vec2f(const Vector3f& vec) noexcept
    {
        return Vector2f{ vec.x, vec.y };
    }

    [[nodiscard]] inline auto to_vec2f(const Vector3& vec) noexcept
    {
        return Vector2f{ static_cast<f32>(vec.x), static_cast<f32>(vec.y) };
    }

    [[nodiscard]] inline auto to_vec2(const Vector3f& vec) noexcept
    {
        return Vector2{ static_cast<i32>(vec.x), static_cast<i32>(vec.y) };
    }

    [[nodiscard]] inline auto to_vec2(const Vector3& vec) noexcept
    {
        return Vector2{ vec.x, vec.y };
    }

    [[nodiscard]] inline auto to_rectf(const Vector2f& vec, const f32 w, const f32 h) noexcept
    {
        return Rectf{ vec.x, vec.y, w, h };
    }

    [[nodiscard]] inline auto to_rectf(const Vector2& vec, const f32 w, const f32 h) noexcept
    {
        return Rectf{ static_cast<f32>(vec.x), static_cast<f32>(vec.y), w, h };
    }

    [[nodiscard]] inline auto to_rect(const Vector2f& vec, const i32 w, const i32 h) noexcept
    {
        return Rect{ static_cast<i32>(vec.x), static_cast<i32>(vec.y), w, h };
    }

    [[nodiscard]] inline auto to_rect(const Vector2& vec, const i32 w, const i32 h) noexcept
    {
        return Rect{ vec.x, vec.y, w, h };
    }

    [[nodiscard]] inline auto snap(const Vector3f& vec, const Vector3f cellSize)
    {
        return glm::floor(vec / cellSize) * cellSize;
    }
} // namespace codex::utils

#endif // CODEX_UTILITIES_MATH_H
