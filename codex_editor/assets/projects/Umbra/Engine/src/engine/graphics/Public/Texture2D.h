#pragma once

#include <engine/core/public/i_resource.h>
#include <engine/memory/public/memory.h>
#include <platform/open_gl/texture.h>

namespace codex::gfx {
    class Texture2D : public codex::IResource
    {
        friend class ResourceHandler;

    public:
        Texture2D() = default;
        Texture2D(std::filesystem::path filePath, const opengl::TextureProperties textureProperties = {})
        {
            path_        = filePath;
            raw_texture_ = mem::Box<opengl::Texture>::make(std::move(filePath), textureProperties);
        }

    public:
        explicit inline                         operator bool() const noexcept { return (bool)raw_texture_; }
        inline u32                              gl_id() const noexcept { return raw_texture_->GetGlId(); }
        inline u32                              slot() const { return raw_texture_->GetSlot(); }
        inline i32                              width() const { return raw_texture_->GetWidth(); }
        inline i32                              height() const { return raw_texture_->GetHeight(); }
        inline std::filesystem::path            file_path() const { return raw_texture_->GetFilePath(); }
        inline void                             bind(u32 slot = 0) { raw_texture_->Bind(slot); }
        inline void                             unbind() const { raw_texture_->Unbind(); }
        inline const opengl::TextureProperties& properties() const noexcept { return raw_texture_->GetProperties(); }
        inline void create(const std::filesystem::path filePath, const opengl::TextureProperties textureProperties = {})
        {
            raw_texture_.reset(new opengl::Texture(filePath, textureProperties));
        }

    public:
        void serialize(ISerializationNode& node) const override
        {
            auto        path  = std::string{};
            const auto& props = properties();
            node.write("id", id());
            node.write("file_path", file_path().generic_string());
            node.write("filter_mode", static_cast<u32>(props.filterMode));
            node.write("wrap_mode", static_cast<u32>(props.wrapMode));
            node.write("format", static_cast<u32>(props.format));
        }
        void deserialize(const ISerializationNode& node) override
        {
            auto props = opengl::TextureProperties{};
            auto path  = std::string{};

            node.read("id", id_);
            node.read("file_path", path);
            node.read("filter_mode", *reinterpret_cast<u32*>(&props.filterMode));
            node.read("wrap_mode", *reinterpret_cast<u32*>(&props.wrapMode));
            node.read("format", *reinterpret_cast<u32*>(&props.format));

            path_        = path;
            raw_texture_ = mem::Box<opengl::Texture>::make(std::move(path), props);
        }

    private:
        mem::Box<opengl::Texture> raw_texture_ = nullptr;
    };
} // namespace codex::gfx
