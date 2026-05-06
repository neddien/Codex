#pragma once

#include <engine/asset_manager/public/asset_manager.h>
#include <engine/core/public/geometry.h>
#include <engine/core/public/serializer.h>
#include <engine/graphics/public/texture2d.h>

namespace codex {
    class CODEX_API Sprite : public ISerializable
    {
    public:
        Sprite() = default;
        Sprite(Asset<gfx::Texture2D> texture, const Vector4f colour = { 1.0f, 1.0f, 1.0f, 1.0f }, const i32 z_index = 0)
            : texture_(texture)
            , colour_(colour)
            , z_index_(z_index)
        {
            texture_coords_ = { 0.0f, 0.0f, static_cast<f32>(texture->width()), static_cast<f32>(texture->height()) };
            size_           = { texture_coords_.w, texture_coords_.h };
        }
        Sprite(Asset<gfx::Texture2D> texture, const Rectf texture_coords,
               const Vector4f colour = { 1.0f, 1.0f, 1.0f, 1.0f }, const i32 z_index = 0)
            : texture_(texture)
            , texture_coords_(texture_coords)
            , colour_(colour)
            , z_index_(z_index)
        {
            size_ = { (f32)texture_->width(), (f32)texture_->height() };
        }

    public:
        [[nodiscard]] static inline Sprite empty() noexcept { return Sprite(); }

    public:
        [[nodiscard]] inline Asset<gfx::Texture2D> texture() const noexcept { return texture_; }
        inline void set_texture(Asset<gfx::Texture2D> new_texture) noexcept { Sprite{ new_texture }.swap(*this); }

        [[nodiscard]] inline Rectf texture_coords() const noexcept { return texture_coords_; }
        inline void                set_texture_coords(Rectf new_texture_coords) noexcept
        {
            texture_coords_ = std::move(new_texture_coords);
        }

        [[nodiscard]] inline Vector4f colour() const noexcept { return colour_; }
        inline void                   set_colour(const Vector4f& new_colour) noexcept { colour_ = new_colour; }

        [[nodiscard]] inline i32 z_index() const noexcept { return z_index_; }
        inline void              set_z_index(const i32 new_z_index) noexcept { z_index_ = new_z_index; }

        [[nodiscard]] inline Vector2f size() const noexcept { return size_; }
        inline void                   set_size(const Vector2f& new_size) noexcept { size_ = new_size; }

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
        void serialize(ISerializationNode& node) const override
        {
            texture_.path().serialize(node.create_child("asset"));
            node.write("texture_coords", texture_coords_);
            node.write("size", size_);
            node.write("colour", colour_);
            node.write("z_index", z_index_);
        }
        void deserialize(const ISerializationNode& node) override
        {
            if (texture_)
                texture_.reset();

            AssetPath path;
            auto&     asset_child = node.child("asset");
            path.deserialize(asset_child);

            texture_ = AssetManager::load<gfx::Texture2D>(path.uuid());

            node.read("texture_coords", texture_coords_);
            node.read("size", size_);
            node.read("colour", colour_);
            node.read("z_index", z_index_);
        }

    private:
        Asset<gfx::Texture2D> texture_{};
        Rectf                 texture_coords_{};
        Vector2f              size_{};
        Vector4f              colour_{};
        i32                   z_index_{};
    };
} // namespace codex
