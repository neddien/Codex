#pragma once

#include <engine/core/public/geometry.h>

namespace codex::gfx {
    class CODEX_API Line2D
    {
    public:
        Line2D(const vec2 source, const vec2 destination, const vec4 colour, const i32 lifeTime)
            : source_(source)
            , destination_(destination)
            , colour_(colour)
            , life_time_(lifeTime)
        {
        }

    public:
        [[nodiscard]] inline vec2  source() const { return source_; }
        [[nodiscard]] inline vec2  destination() const { return destination_; }
        [[nodiscard]] inline vec4 colour() const { return colour_; }
        [[nodiscard]] inline i32   life_time() const { return life_time_; }

    public:
        inline i32 begin_frame() noexcept { return --life_time_; }

    private:
        vec2  source_;
        vec2  destination_;
        vec4 colour_;
        i32   life_time_;
    };
} // namespace codex::gfx
