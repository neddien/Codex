#pragma once

namespace codex::opengl {
    struct Rect;
    struct Rectf;

    struct Rect
    {
    public:
        int x, y, w, h;

    public:
        constexpr Rect(const int x = 0, const int y = 0, const int w = 0, const int h = 0) noexcept
            : x{ x }
            , y{ y }
            , w{ w }
            , h{ h }
        {
        }
        /*
        constexpr Rect(const Rectf& rectf) noexcept
            : x{ rectf.x }
            , y{ rectf.y }
            , w{ rectf.w }
            , h{ rectf.h }
        {
        }
        */
    };

    struct Rectf
    {
    public:
        float x, y, w, h;

    public:
        constexpr Rectf(const float x = 0.0f, const float y = 0.0f, const float w = 0.0f, const float h = 0.0f) noexcept
            : x{ x }
            , y{ y }
            , w{ w }
            , h{ h }
        {
        }
        /*
        constexpr Rectf(const Rect& rect) noexcept
            : x{ rect.x }
            , y{ rect.y }
            , w{ rect.w }
            , h{ rect.h }
        {
        }
        */
    };
} // namespace codex::opengl
