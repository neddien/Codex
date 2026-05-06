#pragma once

#include <engine/asset_manager/public/asset_manager.h>
#include <engine/filesystem/public/file_handle.h>
#include <engine/memory/public/memory.h>
#include <platform/open_gl/texture.h>

namespace codex::gfx {
    class CODEX_API Texture2D : public IAsset
    {
        CX_ASSET(Texture2D)

    public:
        struct ImportSettings : public IAssetImportSettings
        {
            CX_ASSET_IMPORT_SETTINGS(ImportSettings)

        public:
            opengl::TextureProperties props;

        public:
            ImportSettings() noexcept = default;
            ImportSettings(const opengl::TextureProperties& props) noexcept
                : props{ props }
            {
            }

        public:
            void serialize(ISerializationNode& node) const
            {
                node.write("filter_mode", opengl::to_string(props.filter_mode));
                node.write("mipmap_mode", opengl::to_string(props.mipmap_mode));
                node.write("wrap_mode", opengl::to_string(props.wrap_mode));
                node.write("format", opengl::to_string(props.format));
            }
            void deserialize(const ISerializationNode& node)
            {
                if (std::string str; node.read("filter_mode", str))
                    props.filter_mode = opengl::texture_filter_mode_from_string(str);
                if (std::string str; node.read("mipmap_mode", str))
                    props.mipmap_mode = opengl::texture_mipmap_mode_from_string(str);
                if (std::string str; node.read("wrap_mode", str))
                    props.wrap_mode = opengl::texture_wrap_mode_from_string(str);
                if (std::string str; node.read("format", str))
                    props.format = opengl::texture_format_from_string(str);
            }
        };

    public:
        Texture2D() = default;
        explicit Texture2D(const u8* buf, const usize len, const ImportSettings& import_settings = {})
            : buf_{ buf }
            , len_{ len }
        {
            raw_texture_ = Box<opengl::Texture>::make(buf, len, import_settings.props);
        }
        Texture2D(const Texture2D& other)
            : buf_{ other.buf_ }
            , len_{ other.len_ }
        {
            raw_texture_ = Box<opengl::Texture>::make(buf_, len_, other.properties());
        }
        Texture2D(Texture2D&& other) noexcept
            : buf_{ std::move(other.buf_) }
            , len_{ std::move(other.len_) }
            , raw_texture_{ std::move(other.raw_texture_) }
        {
        }
        Texture2D& operator=(const Texture2D& other) { return Texture2D{ other }.swap(*this); }
        Texture2D& operator=(Texture2D&& other) noexcept { return Texture2D{ std::move(other) }.swap(*this); }

    public:
        [[nodiscard]] explicit inline operator bool() const noexcept { return (bool)raw_texture_; }
        [[nodiscard]] inline u32      gl_id() const noexcept { return raw_texture_->gl_id(); }
        [[nodiscard]] inline u32      slot() const { return raw_texture_->slot(); }
        [[nodiscard]] inline i32      width() const { return raw_texture_->width(); }
        [[nodiscard]] inline i32      height() const { return raw_texture_->height(); }
        [[nodiscard]] inline const opengl::TextureProperties& properties() const noexcept
        {
            return raw_texture_->properties();
        }

    public:
        inline Texture2D& swap(Texture2D& other) noexcept
        {
            std::swap(buf_, other.buf_);
            std::swap(len_, other.len_);
            std::swap(raw_texture_, other.raw_texture_);
            return *this;
        }
        inline void bind(u32 slot = 0) { raw_texture_->bind(slot); }
        inline void unbind() const { raw_texture_->unbind(); }
        inline void create(const u8* buf, const usize len, const ImportSettings& import_settings = {})
        {
            raw_texture_.reset(new opengl::Texture(buf, len, import_settings.props));
        }

    private:
        const u8*            buf_         = nullptr;
        usize                len_         = 0;
        Box<opengl::Texture> raw_texture_ = nullptr;
    };

    class Texture2DLoader : public AssetLoaderBase<Texture2D, Texture2D::ImportSettings>
    {
        [[nodiscard]] Shared<Texture2D> load(Shared<fs::FileHandle>           fh,
                                             const Texture2D::ImportSettings& params) const noexcept override
        {
            std::vector<u8> buf(fh->size());

            // Read it into memory since textures can't be that large (max ~300mb).
            auto red_len = fh->read(buf.data(), fh->size());

            return Shared<Texture2D>::make(buf.data(), buf.size(), params);
        }
    };
} // namespace codex::gfx
