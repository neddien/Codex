#include "public/texture2d.h"

#include <platform/open_gl/texture.h>

namespace codex::gfx {
    namespace {
        opengl::TextureProperties to_opengl(const TextureProperties& p) noexcept
        {
            opengl::TextureFormat fmt = opengl::TextureFormat::None;
            switch (p.format) {
                using enum TextureFormat;
                case RGBA8: fmt = opengl::TextureFormat::RGBA8; break;
                case RGB32F: fmt = opengl::TextureFormat::RGB32F; break;
                case RGBA32F: fmt = opengl::TextureFormat::RGBA32F; break;
                case Depth: fmt = opengl::TextureFormat::Depth; break;
                case Depth16: fmt = opengl::TextureFormat::Depth16; break;
                case Depth24: fmt = opengl::TextureFormat::Depth24; break;
                case Depth32: fmt = opengl::TextureFormat::Depth32; break;
                case Depth32F: fmt = opengl::TextureFormat::Depth32F; break;
                case Depth24Stencil8: fmt = opengl::TextureFormat::Depth24Stencil8; break;
                case Depth32FStencil8: fmt = opengl::TextureFormat::Depth32FStencil8; break;
                case RedInt32: fmt = opengl::TextureFormat::RedInt32; break;
                case RedUInt32: fmt = opengl::TextureFormat::RedUInt32; break;
                case RedFloat32: fmt = opengl::TextureFormat::RedFloat32; break;
                default: break;
            }

            opengl::TextureWrapMode wrap = opengl::TextureWrapMode::Mirror;
            switch (p.wrap_mode) {
                using enum TextureWrapMode;
                case Mirror: wrap = opengl::TextureWrapMode::Mirror; break;
                case Stretch: wrap = opengl::TextureWrapMode::Stretch; break;
                case Border: wrap = opengl::TextureWrapMode::Border; break;
            }

            opengl::TextureFilterMode filter = opengl::TextureFilterMode::Linear;
            switch (p.filter_mode) {
                using enum TextureFilterMode;
                case Linear: filter = opengl::TextureFilterMode::Linear; break;
                case Nearest: filter = opengl::TextureFilterMode::Nearest; break;
            }

            opengl::TextureMipmapMode mipmap = opengl::TextureMipmapMode::None;
            switch (p.mipmap_mode) {
                using enum TextureMipmapMode;
                case None: mipmap = opengl::TextureMipmapMode::None; break;
                case LinearLinear: mipmap = opengl::TextureMipmapMode::LinearLinear; break;
                case LinearNearest: mipmap = opengl::TextureMipmapMode::LinearNearest; break;
                case NearestNearest: mipmap = opengl::TextureMipmapMode::NearestNearest; break;
                case NearestLinear: mipmap = opengl::TextureMipmapMode::NearestLinear; break;
            }

            return { fmt, wrap, filter, mipmap };
        }
    } // namespace

    Texture2D::Texture2D(const u8* buf, const usize len, const ImportSettings& import_settings)
        : buf_{ buf }
        , len_{ len }
        , props_{ import_settings.props }
        , raw_texture_{ Box<opengl::Texture>::make(buf, len, to_opengl(props_)) }
    {
    }

    Texture2D::Texture2D(const Texture2D& other)
        : buf_{ other.buf_ }
        , len_{ other.len_ }
        , props_{ other.props_ }
        , raw_texture_{ Box<opengl::Texture>::make(buf_, len_, to_opengl(props_)) }
    {
    }

    Texture2D::Texture2D(Texture2D&& other) noexcept
        : buf_{ other.buf_ }
        , len_{ other.len_ }
        , props_{ other.props_ }
        , raw_texture_{ std::move(other.raw_texture_) }
    {
    }

    Texture2D& Texture2D::operator=(const Texture2D& other)
    {
        return Texture2D{ other }.swap(*this);
    }

    Texture2D& Texture2D::operator=(Texture2D&& other) noexcept
    {
        return Texture2D{ std::move(other) }.swap(*this);
    }

    Texture2D::~Texture2D() = default;

    Texture2D::operator bool() const noexcept
    {
        return (bool)raw_texture_;
    }

    u32 Texture2D::gl_id() const noexcept
    {
        return raw_texture_->gl_id();
    }

    u32 Texture2D::slot() const
    {
        return raw_texture_->slot();
    }

    i32 Texture2D::width() const
    {
        return raw_texture_->width();
    }

    i32 Texture2D::height() const
    {
        return raw_texture_->height();
    }

    const TextureProperties& Texture2D::properties() const noexcept
    {
        return props_;
    }

    Texture2D& Texture2D::swap(Texture2D& other) noexcept
    {
        std::swap(buf_, other.buf_);
        std::swap(len_, other.len_);
        std::swap(props_, other.props_);
        raw_texture_.swap(other.raw_texture_);
        return *this;
    }

    void Texture2D::bind(const u32 slot)
    {
        raw_texture_->bind(slot);
    }

    void Texture2D::unbind() const
    {
        raw_texture_->unbind();
    }

    void Texture2D::create(const u8* buf, const usize len, const ImportSettings& import_settings)
    {
        props_       = import_settings.props;
        buf_         = buf;
        len_         = len;
        raw_texture_ = Box<opengl::Texture>::make(buf, len, to_opengl(props_));
    }

    [[nodiscard]] Shared<Texture2D> Texture2DLoader::load(Shared<fs::FileHandle>           fh,
                                                          const Texture2D::ImportSettings& params) const noexcept
    {
        std::vector<u8> buf(fh->size());
        auto            read_len = fh->read(buf.data(), fh->size());
        return Shared<Texture2D>::make(buf.data(), buf.size(), params);
    }

    void Texture2D::ImportSettings::archive(Archive& ar)
    {
        ar("filter_mode", props.filter_mode);
        ar("mipmap_mode", props.mipmap_mode);
        ar("wrap_mode", props.wrap_mode);
        ar("format", props.format);
    }
} // namespace codex::gfx
