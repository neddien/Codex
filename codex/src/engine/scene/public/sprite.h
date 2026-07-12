#pragma once

#include <engine/asset_manager/public/asset_manager.h>
#include <engine/core/public/archive.h>
#include <engine/core/public/geometry.h>
#include <engine/graphics/public/texture2d.h>

namespace codex {
    class CODEX_API Sprite : public ISerializable
    {
    public:
        Sprite() = default;
        Sprite(Asset<gfx::Texture2D> texture, const vec4 colour = { 1.0f, 1.0f, 1.0f, 1.0f }, const i32 z_index = 0)
            : texture_{ texture }
            , colour_{ colour }
            , z_index_{ z_index }
        {
            texture_coords_ = { 0.0f, 0.0f, static_cast<f32>(texture->width()), static_cast<f32>(texture->height()) };
            size_           = { texture_coords_.w, texture_coords_.h };
        }
        Sprite(Asset<gfx::Texture2D> texture, const rect texture_coords,
               const vec4 colour = { 1.0f, 1.0f, 1.0f, 1.0f }, const i32 z_index = 0)
            : texture_{ texture }
            , texture_coords_{ texture_coords }
            , colour_{ colour }
            , z_index_{ z_index }
        {
            size_ = { (f32)texture_->width(), (f32)texture_->height() };
        }

    public:
        [[nodiscard]] static inline Sprite empty() noexcept { return Sprite(); }

    public:
        [[nodiscard]] inline Asset<gfx::Texture2D> texture() const noexcept { return texture_; }
        inline void set_texture(Asset<gfx::Texture2D> new_texture) noexcept { Sprite{ new_texture }.swap(*this); }

        [[nodiscard]] inline rect texture_coords() const noexcept { return texture_coords_; }
        inline void               set_texture_coords(rect new_texture_coords) noexcept
        {
            texture_coords_ = std::move(new_texture_coords);
        }

        [[nodiscard]] inline vec4 colour() const noexcept { return colour_; }
        inline void                set_colour(const vec4& new_colour) noexcept { colour_ = new_colour; }

        [[nodiscard]] inline i32 z_index() const noexcept { return z_index_; }
        inline void              set_z_index(const i32 new_z_index) noexcept { z_index_ = new_z_index; }

        [[nodiscard]] inline vec2 size() const noexcept { return size_; }
        inline void               set_size(const vec2& new_size) noexcept { size_ = new_size; }

    public:
        inline void swap(Sprite& other) noexcept
        {
            std::swap(texture_, other.texture_);
            std::swap(texture_coords_, other.texture_coords_);
            std::swap(size_, other.size_);
            std::swap(colour_, other.colour_);
            std::swap(z_index_, other.z_index_);
        }

    public:
        [[nodiscard]] inline operator bool() const noexcept { return texture_.valid(); }

    public:
        void archive(Archive& ar) override
        {
            AssetPath path = ar.saving() ? texture_.path() : AssetPath{};
            ar("asset", path);
            if (ar.loading()) {
                if (texture_)
                    texture_.reset();
                texture_ = AssetManager::load<gfx::Texture2D>(path.uuid());
            }
            ar("texture_coords", texture_coords_);
            ar("size", size_);
            ar("colour", colour_);
            ar("z_index", z_index_);
        }

    private:
        Asset<gfx::Texture2D> texture_{};
        rect                  texture_coords_{};
        vec2                  size_{};
        vec4                 colour_{};
        i32                   z_index_{};
    };
} // namespace codex
