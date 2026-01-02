#include "Geometry.h"

namespace codex::opengl {
    Rect::Rect(const int x, const int y, const int w, const int h)
        : x(x)
        , y(y)
        , w(w)
        , h(h)
    {
    }

    Rect::Rect(const Rectf rectf)
        : x((int)(rectf.x))
        , y((int)(rectf.y))
        , w((int)(rectf.w))
        , h((int)(rectf.h))
    {
    }

    Rectf::Rectf(const float x, const float y, const float w, const float h)
        : x(x)
        , y(y)
        , w(w)
        , h(h)
    {
    }

    Rectf::Rectf(const Rect rect)
        : x((float)(rect.x))
        , y((float)(rect.y))
        , w((float)(rect.w))
        , h((float)(rect.h))
    {
    }
} // namespace codex::opengl
