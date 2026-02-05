#pragma once

#include <sdafx.h>

#include <Engine/Core/Public/IResource.h>
#include <Engine/Memory/Public/Memory.h>
#include <Platform/OpenGL/Texture.h>

namespace codex::gfx {
    class Texture2D : public codex::IResource
    {
        friend class ResourceHandler;

    private:
        mem::Box<opengl::Texture> m_RawTexture = nullptr;

    public:
        Texture2D() = default;
        Texture2D(std::filesystem::path filePath, const opengl::TextureProperties textureProperties = {})
        {
            m_Path       = filePath;
            m_RawTexture = mem::Box<opengl::Texture>::New(std::move(filePath), textureProperties);
        }

    public:
        explicit inline                         operator bool() const noexcept { return (bool)m_RawTexture; }
        inline u32                              GetGlId() const noexcept { return m_RawTexture->GetGlId(); }
        inline u32                              GetSlot() const { return m_RawTexture->GetSlot(); }
        inline i32                              GetWidth() const { return m_RawTexture->GetWidth(); }
        inline i32                              GetHeight() const { return m_RawTexture->GetHeight(); }
        inline std::filesystem::path            GetFilePath() const { return m_RawTexture->GetFilePath(); }
        inline void                             Bind(u32 slot = 0) { m_RawTexture->Bind(slot); }
        inline void                             Unbind() const { m_RawTexture->Unbind(); }
        inline const opengl::TextureProperties& GetProperties() const noexcept { return m_RawTexture->GetProperties(); }
        inline void New(const std::filesystem::path filePath, const opengl::TextureProperties textureProperties = {})
        {
            m_RawTexture.Reset(new opengl::Texture(filePath, textureProperties));
        }

    public:
        void Serialize(ISerializationNode& node) const override
        {
            auto        path  = std::string{};
            const auto& props = GetProperties();
            node.Write("id", GetId());
            node.Write("file_path", GetFilePath().generic_string());
            node.Write("filter_mode", static_cast<u32>(props.filterMode));
            node.Write("wrap_mode", static_cast<u32>(props.wrapMode));
            node.Write("format", static_cast<u32>(props.format));
        }
        void Deserialize(const ISerializationNode& node) override
        {
            auto props = opengl::TextureProperties{};
            auto path  = std::string{};

            node.Read("id", m_Id);
            node.Read("file_path", path);
            node.Read("filter_mode", *reinterpret_cast<u32*>(&props.filterMode));
            node.Read("wrap_mode", *reinterpret_cast<u32*>(&props.wrapMode));
            node.Read("format", *reinterpret_cast<u32*>(&props.format));

            m_Path       = path;
            m_RawTexture = mem::Box<opengl::Texture>::New(std::move(path), props);
        }
    };
} // namespace codex::gfx
