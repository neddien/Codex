#pragma once

#include <cstdint>
#include <filesystem>

#include "common.h"

namespace codex::opengl {
    enum class TextureFormat : i32
    {
        None             = -1,
        RGBA8            = GL_RGBA8,
        RGB32F           = GL_RGB32F,
        RGBA32F          = GL_RGBA32F,
        Depth            = GL_DEPTH_COMPONENT,
        Depth16          = GL_DEPTH_COMPONENT16,
        Depth24          = GL_DEPTH_COMPONENT24,
        Depth32          = GL_DEPTH_COMPONENT32,
        Depth32F         = GL_DEPTH_COMPONENT32F,
        Depth24Stencil8  = GL_DEPTH24_STENCIL8,
        Depth32FStencil8 = GL_DEPTH32F_STENCIL8,
        RedInt32         = GL_R32I,
        RedUInt32        = GL_R32UI,
        RedFloat32       = GL_R32F
    };

    enum class TextureFormatType : i32
    {
        RGB          = GL_RGB,
        RGBA         = GL_RGBA,
        DepthStencil = GL_DEPTH_STENCIL,
        Depth        = GL_DEPTH_COMPONENT
    };

    enum class TextureWrapMode : i32
    {
        Mirror  = GL_MIRRORED_REPEAT,
        Stretch = GL_CLAMP_TO_EDGE,
        Border  = GL_CLAMP_TO_BORDER
    };

    enum class TextureFilterMode : i32
    {
        Linear  = GL_LINEAR,
        Nearest = GL_NEAREST
    };

    enum class TextureMipmapMode : i32
    {
        None           = -1,
        LinearLinear   = GL_LINEAR_MIPMAP_LINEAR,
        LinearNearest  = GL_LINEAR_MIPMAP_NEAREST,
        NearestNearest = GL_NEAREST_MIPMAP_NEAREST,
        NearestLinear  = GL_NEAREST_MIPMAP_LINEAR
    };

    [[nodiscard]] constexpr std::string_view to_string(const TextureFormat v) noexcept
    {
        switch (v) {
            case TextureFormat::None: return "None";
            case TextureFormat::RGBA8: return "RGBA8";
            case TextureFormat::RGB32F: return "RGB32F";
            case TextureFormat::RGBA32F: return "RGBA32F";
            case TextureFormat::Depth: return "Depth";
            case TextureFormat::Depth16: return "Depth16";
            case TextureFormat::Depth24: return "Depth24";
            case TextureFormat::Depth32: return "Depth32";
            case TextureFormat::Depth32F: return "Depth32F";
            case TextureFormat::Depth24Stencil8: return "Depth24Stencil8";
            case TextureFormat::Depth32FStencil8: return "Depth32FStencil8";
            case TextureFormat::RedInt32: return "RedInt32";
            case TextureFormat::RedUInt32: return "RedUInt32";
            case TextureFormat::RedFloat32: return "RedFloat32";
            default: return "";
        }
    }

    [[nodiscard]] constexpr TextureFormat texture_format_from_string(const std::string_view s) noexcept
    {
        if (s == "RGBA8")
            return TextureFormat::RGBA8;
        if (s == "RGB32F")
            return TextureFormat::RGB32F;
        if (s == "RGBA32F")
            return TextureFormat::RGBA32F;
        if (s == "Depth")
            return TextureFormat::Depth;
        if (s == "Depth16")
            return TextureFormat::Depth16;
        if (s == "Depth24")
            return TextureFormat::Depth24;
        if (s == "Depth32")
            return TextureFormat::Depth32;
        if (s == "Depth32F")
            return TextureFormat::Depth32F;
        if (s == "Depth24Stencil8")
            return TextureFormat::Depth24Stencil8;
        if (s == "Depth32FStencil8")
            return TextureFormat::Depth32FStencil8;
        if (s == "RedInt32")
            return TextureFormat::RedInt32;
        if (s == "RedUInt32")
            return TextureFormat::RedUInt32;
        if (s == "RedFloat32")
            return TextureFormat::RedFloat32;
        return TextureFormat::None;
    }

    [[nodiscard]] constexpr std::string_view to_string(const TextureWrapMode v) noexcept
    {
        switch (v) {
            case TextureWrapMode::Mirror: return "Mirror";
            case TextureWrapMode::Stretch: return "Stretch";
            case TextureWrapMode::Border: return "Border";
            default: return "";
        }
    }

    [[nodiscard]] constexpr TextureWrapMode texture_wrap_mode_from_string(const std::string_view s) noexcept
    {
        if (s == "Stretch")
            return TextureWrapMode::Stretch;
        if (s == "Border")
            return TextureWrapMode::Border;
        return TextureWrapMode::Mirror;
    }

    [[nodiscard]] constexpr std::string_view to_string(const TextureFilterMode v) noexcept
    {
        switch (v) {
            case TextureFilterMode::Linear: return "Linear";
            case TextureFilterMode::Nearest: return "Nearest";
            default: return "";
        }
    }

    [[nodiscard]] constexpr TextureFilterMode texture_filter_mode_from_string(const std::string_view s) noexcept
    {
        if (s == "Nearest")
            return TextureFilterMode::Nearest;
        return TextureFilterMode::Linear;
    }

    [[nodiscard]] constexpr std::string_view to_string(const TextureMipmapMode v) noexcept
    {
        switch (v) {
            using enum TextureMipmapMode;

            case None: return "None";
            case LinearLinear: return "LinearLinear";
            case LinearNearest: return "LinearNearest";
            case NearestNearest: return "NearestNearest";
            case NearestLinear: return "NearestLinear";
            default: return "";
        }
    }

    [[nodiscard]] constexpr TextureMipmapMode texture_mipmap_mode_from_string(const std::string_view s) noexcept
    {
        using enum TextureMipmapMode;

        if (s == "LinearLinear")
            return LinearLinear;
        if (s == "LinearNearest")
            return LinearNearest;
        if (s == "NearestNearest")
            return NearestNearest;
        if (s == "NearestLinear")
            return NearestLinear;
        return None;
    }

    struct TextureProperties
    {
        TextureFormat     format                 = TextureFormat::None;
        TextureWrapMode   wrap_mode              = TextureWrapMode::Mirror;
        TextureFilterMode filter_mode            = TextureFilterMode::Linear;
        TextureMipmapMode mipmap_mode            = TextureMipmapMode::None;
        opt<vec3>         chroma_key             = std::nullopt;
        f32               chroma_inner_tolerance = 30.0f;
        f32               chroma_outer_tolerance = 70.0f;
    };

    class CODEX_API Texture
    {
    public:
        Texture(const u8* buf, usize len, TextureProperties properties = {});
        ~Texture();

    public:
        [[nodiscard]] inline u32                      gl_id() const { return renderer_id_; }
        [[nodiscard]] inline i32                      width() const { return width_; }
        [[nodiscard]] inline i32                      height() const { return height_; }
        [[nodiscard]] inline u32                      slot() const { return slot_; }
        [[nodiscard]] inline const TextureProperties& properties() const { return props_; }

    public:
        void bind(const u32 slot = 0);
        void unbind() const;

    public:
        [[nodiscard]] static GLenum format_type(const TextureProperties props);

    private:
        u32               renderer_id_;
        TextureProperties props_;
        u8*               buffer_;
        i32               width_;
        i32               height_;
        i32               gpp_;
        u32               slot_;
    };
} // namespace codex::opengl
