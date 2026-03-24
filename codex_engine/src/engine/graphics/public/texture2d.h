#pragma once

#include <engine/core/public/i_resource.h>
#include <engine/memory/public/memory.h>
#include <platform/open_gl/texture.h>

namespace codex::gfx {
    class CODEX_API Texture2D : public codex::IResource
    {
        friend class ResourceHandler;

    public:
        Texture2D() = default;
        explicit Texture2D(std::filesystem::path file_path, const opengl::TextureProperties texture_properties = {})
        {
            path_        = file_path;
            raw_texture_ = mem::Box<opengl::Texture>::make(std::move(file_path), texture_properties);
        }

    public:
        [[nodiscard]] explicit inline              operator bool() const noexcept { return (bool)raw_texture_; }
        [[nodiscard]] inline u32                   gl_id() const noexcept { return raw_texture_->gl_id(); }
        [[nodiscard]] inline u32                   slot() const { return raw_texture_->slot(); }
        [[nodiscard]] inline i32                   width() const { return raw_texture_->width(); }
        [[nodiscard]] inline i32                   height() const { return raw_texture_->height(); }
        [[nodiscard]] inline std::filesystem::path file_path() const { return raw_texture_->file_path(); }
        [[nodiscard]] inline const opengl::TextureProperties& properties() const noexcept
        {
            return raw_texture_->properties();
        }

    public:
        inline void bind(u32 slot = 0) { raw_texture_->bind(slot); }
        inline void unbind() const { raw_texture_->unbind(); }
        inline void create(const std::filesystem::path     file_path,
                           const opengl::TextureProperties texture_properties = {})
        {
            raw_texture_.reset(new opengl::Texture(file_path, texture_properties));
        }

    public:
        void serialize(ISerializationNode& node) const override
        {
            const auto& props = properties();
            node.write("id", id());
            node.write("file_path", file_path().generic_string());
            node.write("filter_mode", static_cast<u32>(props.filter_mode));
            node.write("wrap_mode", static_cast<u32>(props.wrap_mode));
            node.write("format", static_cast<u32>(props.format));
        }
        void deserialize(const ISerializationNode& node) override
        {
            auto props = opengl::TextureProperties{};
            auto path  = std::string{};

            node.read("id", id_);
            node.read("file_path", path);
            node.read("filter_mode", reinterpret_cast<u32&>(props.filter_mode));
            node.read("wrap_mode", reinterpret_cast<u32&>(props.wrap_mode));
            node.read("format", reinterpret_cast<u32&>(props.format));

            path_        = path;
            raw_texture_ = mem::Box<opengl::Texture>::make(std::move(path), props);
        }

    private:
        mem::Box<opengl::Texture> raw_texture_ = nullptr;
    };
} // namespace codex::gfx
