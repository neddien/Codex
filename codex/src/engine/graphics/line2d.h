#pragma once

#include <engine/core/public/geometry.h>

namespace codex::gfx {
    class CODEX_API Line2D
    {
    public:
        Line2D(const Vector2f source, const Vector2f destination, const Vector4f colour, const i32 lifeTime)
            : source_(source)
            , destination_(destination)
            , colour_(colour)
            , life_time_(lifeTime)
        {
        }

    public:
        [[nodiscard]] inline Vector2f source() const { return source_; }
        [[nodiscard]] inline Vector2f destination() const { return destination_; }
        [[nodiscard]] inline Vector4f colour() const { return colour_; }
        [[nodiscard]] inline i32      life_time() const { return life_time_; }

    public:
        inline i32 begin_frame() noexcept { return --life_time_; }

    private:
        Vector2f source_;
        Vector2f destination_;
        Vector4f colour_;
        i32      life_time_;
    };
} // namespace codex::gfx
