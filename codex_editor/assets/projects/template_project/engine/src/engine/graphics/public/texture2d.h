#pragma once

#include <engine/asset_manager/public/asset_manager.h>
#include <engine/core/public/i_resource.h>
#include <engine/filesystem/public/file_handle.h>
#include <engine/memory/public/memory.h>
#include <platform/open_gl/texture.h>

namespace codex::gfx {
    class Texture2DLoader;

    class CODEX_API Texture2D : public codex::IResource
    {
        friend class ResourceHandler;

    public:
        using loader_type = Texture2DLoader;

    public:
        Texture2D() = default;
        explicit Texture2D(std::filesystem::path file_path, const opengl::TextureProperties texture_properties = {})
        {
            path_        = file_path;
            raw_texture_ = Box<opengl::Texture>::make(std::move(file_path), texture_properties);
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
            node.write("filter_mode", opengl::to_string(props.filter_mode));
            node.write("wrap_mode", opengl::to_string(props.wrap_mode));
            node.write("format", opengl::to_string(props.format));
        }
        void deserialize(const ISerializationNode& node) override
        {
            auto props = opengl::TextureProperties{};
            auto path  = std::string{};

            node.read("id", id_);
            node.read("file_path", path);

            if (std::string str; node.read("filter_mode", str))
                props.filter_mode = opengl::texture_filter_mode_from_string(str);
            if (std::string str; node.read("wrap_mode", str))
                props.wrap_mode = opengl::texture_wrap_mode_from_string(str);
            if (std::string str; node.read("format", str))
                props.format = opengl::texture_format_from_string(str);

            path_        = path;
            raw_texture_ = Box<opengl::Texture>::make(std::move(path), props);
        }

    private:
        Box<opengl::Texture> raw_texture_ = nullptr;
    };

    class Texture2DLoader : public AssetLoaderBase<Texture2D, opengl::TextureProperties>
    {
    public:
        Shared<Texture2D> load_asset(Shared<fs::FileHandle> fh, const opengl::TextureProperties& props) override { return {}; }
    };
} // namespace codex::gfx
