#pragma once

#ifdef None
#undef None
#endif

namespace codex::gfx {
    enum class TextureFormat
    {
        None,
        RGBA8,
        RGB32F,
        RGBA32F,
        Depth,
        Depth16,
        Depth24,
        Depth32,
        Depth32F,
        Depth24Stencil8,
        Depth32FStencil8,
        RedInt32,
        RedUInt32,
        RedFloat32
    };

    enum class TextureWrapMode
    {
        Mirror,
        Stretch,
        Border
    };

    enum class TextureFilterMode
    {
        Linear,
        Nearest
    };

    enum class TextureMipmapMode
    {
        None,
        LinearLinear,
        LinearNearest,
        NearestNearest,
        NearestLinear
    };

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
} // namespace codex::gfx
