#ifndef CODEX_CORE_TEXTURE_2D_H
#define CODEX_CORE_TEXTURE_2D_H

#include <sdafx.h>

#include <Engine/Core/Public/IResource.h>
#include <Engine/Memory/Public/Memory.h>

#include <MGL.h>

namespace codex::gfx {
    using TextureProperties     = mgl::TextureProperties;
    using TextureFilterMode     = mgl::TextureFilterMode;
    using TextureWrapMode       = mgl::TextureWrapMode;
    using TextureFormat         = mgl::TextureFormat;
    using TextureFormatType     = mgl::TextureFormatType;
    using TextureMipmapMode     = mgl::TextureMipmapMode;
    using FrameBuffer           = mgl::FrameBuffer;
    using FrameBufferProperties = mgl::FrameBufferProperties;

    class Texture2D : public codex::IResource
    {
        friend class ResourceHandler;

    private:
        mem::Box<mgl::Texture> m_RawTexture = nullptr;

    public:
        Texture2D() = default;
        Texture2D(std::filesystem::path filePath, const TextureProperties textureProperties = {})
        {
            m_RawTexture = mem::Box<mgl::Texture>::New(std::move(filePath), textureProperties);
        }

    public:
        explicit inline                 operator bool() const noexcept { return (bool)m_RawTexture; }
        inline u32                      GetGlId() const noexcept { return m_RawTexture->GetGlId(); }
        inline u32                      GetSlot() const { return m_RawTexture->GetSlot(); }
        inline i32                      GetWidth() const { return m_RawTexture->GetWidth(); }
        inline i32                      GetHeight() const { return m_RawTexture->GetHeight(); }
        inline std::filesystem::path    GetFilePath() const { return m_RawTexture->GetFilePath(); }
        inline void                     Bind(u32 slot = 0) { m_RawTexture->Bind(slot); }
        inline void                     Unbind() const { m_RawTexture->Unbind(); }
        inline const TextureProperties& GetProperties() const noexcept { return m_RawTexture->GetProperties(); }
        inline void New(const std::filesystem::path filePath, const TextureProperties textureProperties = {})
        {
            m_RawTexture.Reset(new mgl::Texture(filePath, textureProperties));
        }

    public:
        void Serialize(ISerializationNode& node) const override
        {
            const auto& props = GetProperties();
            node.Write("id", GetId());
            node.Write("file_path", GetFilePath());
            node.Write("filter_mode", static_cast<u32>(props.filterMode));
            node.Write("wrap_mode", static_cast<u32>(props.wrapMode));
            node.Write("format", static_cast<u32>(props.format));
        }
        void Deserialize(const ISerializationNode& node) override
        {
            auto props = TextureProperties{};
            auto path  = std::filesystem::path{};

            node.Read("id", m_Id);
            node.Read("file_path", path);
            node.Read("filter_mode", *reinterpret_cast<u32*>(&props.filterMode));
            node.Read("wrap_mode", *reinterpret_cast<u32*>(&props.wrapMode));
            node.Read("format", *reinterpret_cast<u32*>(&props.format));

            m_RawTexture = mem::Box<mgl::Texture>::New(std::move(path), props);
        }
    };
} // namespace codex::gfx

#endif // CODEX_CORE_TEXTURE_2D_H
