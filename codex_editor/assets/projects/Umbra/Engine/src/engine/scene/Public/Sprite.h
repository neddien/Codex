#pragma once

#include <engine/core/public/geometry.h>
#include <engine/core/public/i_resource.h>
#include <engine/core/public/resource_handler.h>
#include <engine/core/public/serializer.h>
#include <engine/graphics/public/texture2d.h>

namespace codex {
    class CODEX_API Sprite : public ISerializable
    {
    public:
        Sprite() = default;
        Sprite(ResRef<gfx::Texture2D> texture, const Vector4f colour = { 1.0f, 1.0f, 1.0f, 1.0f },
               const i32 z_index = 0)
            : texture_(texture)
            , colour_(colour)
            , z_index_(z_index)
        {
            texture_coords_ = { 0.0f, 0.0f, static_cast<f32>(texture->width()), static_cast<f32>(texture->height()) };
            size_           = { texture_coords_.w, texture_coords_.h };
        }
        Sprite(ResRef<gfx::Texture2D> texture, const Rectf texture_coords,
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
        [[nodiscard]] inline ResRef<gfx::Texture2D> texture() const noexcept { return texture_; }
        inline void set_texture(ResRef<gfx::Texture2D> new_texture) noexcept { Sprite{ new_texture }.swap(*this); }

        [[nodiscard]] inline Rectf texture_coords() const noexcept { return texture_coords_; }
        inline void                set_texture_coords(Rectf new_texture_coords) noexcept
        {
            texture_coords_ = std::move(new_texture_coords);
        }

        [[nodiscard]] inline Vector4f colour() const noexcept { return colour_; }
        inline void                   set_colour(Vector4f new_colour) noexcept { colour_ = std::move(new_colour); }

        [[nodiscard]] inline i32 z_index() const noexcept { return z_index_; }
        inline void              set_z_index(const i32 new_z_index) noexcept { z_index_ = new_z_index; }

        [[nodiscard]] inline Vector2f size() const noexcept { return size_; }
        inline void                   set_size(Vector2f new_size) noexcept { size_ = std::move(new_size); }

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
        [[nodiscard]] inline operator bool() const noexcept { return texture_; }

    public:
        void serialize(ISerializationNode& node) const override
        {
            auto& texture_child = node.create_child("texture");
            texture_->serialize(texture_child);
            node.write("texture_coords", texture_coords_);
            node.write("size", size_);
            node.write("colour", colour_);
            node.write("z_index", z_index_);
        }
        void deserialize(const ISerializationNode& node) override
        {
            if (texture_)
                texture_.reset();

            auto&          texture_child = node.child("texture");
            gfx::Texture2D texture;
            texture.deserialize(texture_child);

            texture_ = Resources::from<gfx::Texture2D>(std::move(texture));

            texture_->deserialize(texture_child);
            node.read("texture_coords", texture_coords_);
            node.read("size", size_);
            node.read("colour", colour_);
            node.read("z_index", z_index_);
        }

    private:
        ResRef<gfx::Texture2D> texture_ = nullptr;
        Rectf                  texture_coords_{};
        Vector2f               size_{};
        Vector4f               colour_{};
        i32                    z_index_ = 0;
    };
} // namespace codex
