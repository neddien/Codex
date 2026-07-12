#pragma once

#include <engine/core/public/geometry.h>
#include <engine/graphics/public/texture2d.h>

#include "sprite.h"

namespace codex {
    class CODEX_API SpriteSheet
    {
    public:
        SpriteSheet(ResRef<gfx::Texture2D> texture, i32 sprite_width, i32 sprite_height, i32 sprite_count, i32 space)
            : texture_(texture)
            , width_(texture->width())
            , height_(texture->height())
            , total_sprite_count_(sprite_count)
            , sprite_width_(sprite_width)
            , sprite_height_(sprite_height)
        {
            i32 x = space, y = space;
            i32 tex_width = texture->width();
            for (i32 i = 0; i < sprite_count; ++i) {
                if (x >= tex_width + space) {
                    y += sprite_height + space;
                    x = space;
                }
                sprite_coords_.emplace_back(x, y);
                sprites_.emplace_back(texture, rect((f32)x, (f32)y, (f32)sprite_width, (f32)sprite_height));
                x += sprite_width + space;
            }
        }

    public:
        [[nodiscard]] inline std::vector<Sprite> sprites() const { return sprites_; }
        [[nodiscard]] inline Sprite              sprite_at(const usize index) const { return sprites_[index]; }
        [[nodiscard]] inline Sprite              sprite_at(const i32 x, const i32 y) const
        {
            return sprites_[(y / sprite_width_) * (width_ / sprite_width_) + (x / sprite_height_)];
        }
        [[nodiscard]] inline i32 sprite_count() const { return total_sprite_count_; }

    private:
        ResRef<gfx::Texture2D> texture_;
        std::vector<vec2>  sprite_coords_;
        std::vector<Sprite>    sprites_;
        i32                    width_;
        i32                    height_;
        i32                    total_sprite_count_;
        i32                    sprite_width_;
        i32                    sprite_height_;
    };
} // namespace codex
