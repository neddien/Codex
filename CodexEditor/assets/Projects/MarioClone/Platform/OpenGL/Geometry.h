#pragma once

namespace codex::opengl {
    struct Rect;
    struct Rectf;

    struct Rect
    {
    public:
        int x, y, w, h;

    public:
        Rect(const int x = 0, const int y = 0, const int w = 0, const int h = 0);
        Rect(const Rectf rectf);
    };

    struct Rectf
    {
    public:
        float x, y, w, h;

    public:
        Rectf(const float x = 0.0f, const float y = 0.0f, const float w = 0.0f, const float h = 0.0f);
        Rectf(const Rect rect);
    };
} // namespace codex::opengl
