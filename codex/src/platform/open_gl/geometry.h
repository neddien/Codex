#pragma once

namespace codex::opengl {
    struct irect;
    struct rect;

    struct irect
    {
    public:
        int x, y, w, h;

    public:
        constexpr irect(const int x = 0, const int y = 0, const int w = 0, const int h = 0) noexcept
            : x{ x }
            , y{ y }
            , w{ w }
            , h{ h }
        {
        }
        /*
        constexpr irect(const rect& rectf) noexcept
            : x{ rectf.x }
            , y{ rectf.y }
            , w{ rectf.w }
            , h{ rectf.h }
        {
        }
        */
    };

    struct rect
    {
    public:
        float x, y, w, h;

    public:
        constexpr rect(const float x = 0.0f, const float y = 0.0f, const float w = 0.0f, const float h = 0.0f) noexcept
            : x{ x }
            , y{ y }
            , w{ w }
            , h{ h }
        {
        }
        /*
        constexpr rect(const irect& rect) noexcept
            : x{ rect.x }
            , y{ rect.y }
            , w{ rect.w }
            , h{ rect.h }
        {
        }
        */
    };
} // namespace codex::opengl
