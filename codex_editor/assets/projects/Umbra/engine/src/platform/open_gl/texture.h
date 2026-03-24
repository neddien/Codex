#pragma once

#include <cstdint>
#include <filesystem>

#include "constants.h"

namespace codex::opengl {
    enum class TextureFormat
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

    enum class TextureFormatType
    {
        RGB          = GL_RGB,
        RGBA         = GL_RGBA,
        DepthStencil = GL_DEPTH_STENCIL,
        Depth        = GL_DEPTH_COMPONENT
    };

    enum class TextureWrapMode
    {
        Mirror  = GL_MIRRORED_REPEAT,
        Stretch = GL_CLAMP_TO_EDGE,
        Border  = GL_CLAMP_TO_BORDER
    };

    enum class TextureFilterMode
    {
        Linear  = GL_LINEAR,
        Nearest = GL_NEAREST
    };

    enum class TextureMipmapMode
    {
        None           = -1,
        LinearLinear   = GL_LINEAR_MIPMAP_LINEAR,
        LinearNearest  = GL_LINEAR_MIPMAP_NEAREST,
        NearestNearest = GL_NEAREST_MIPMAP_NEAREST,
        NearestLinear  = GL_NEAREST_MIPMAP_LINEAR
    };

    struct TextureProperties
    {
        TextureFormat     format      = TextureFormat::None;
        TextureWrapMode   wrap_mode   = TextureWrapMode::Mirror;
        TextureFilterMode filter_mode = TextureFilterMode::Linear;
        TextureMipmapMode mipmap_mode = TextureMipmapMode::None;
    };

    class CODEX_API Texture
    {
    public:
        Texture(std::filesystem::path file_path, TextureProperties properties = {});
        ~Texture();

    public:
        [[nodiscard]] inline u32                      gl_id() const { return renderer_id_; }
        [[nodiscard]] inline i32                      width() const { return width_; }
        [[nodiscard]] inline i32                      height() const { return height_; }
        [[nodiscard]] inline u32                      slot() const { return slot_; }
        [[nodiscard]] inline std::filesystem::path    file_path() const { return file_path_; }
        [[nodiscard]] inline const TextureProperties& properties() const { return props_; }

    public:
        void bind(const u32 slot = 0);
        void unbind() const;

    public:
        [[nodiscard]] static GLenum format_type(const TextureProperties props);

    private:
        u32                         renderer_id_;
        const std::filesystem::path file_path_;
        TextureProperties           props_;
        u8*                         buffer_;
        i32                         width_;
        i32                         height_;
        i32                         gpp_;
        u32                         slot_;
    };
} // namespace codex::opengl
