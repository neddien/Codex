#ifndef CODEX_SCENE_SPRITE_H
#define CODEX_SCENE_SPRITE_H

#include <Engine/Core/Public/Geomtryd.h>
#include <Engine/Core/Public/IResource.h>
#include <Engine/Core/Public/ResourceHandler.h>
#include <Engine/Core/Public/Serializer.h>
#include <Engine/Graphics/Public/Texture2D.h>

namespace codex {
    class CODEX_API Sprite : public ISerializable
    {
    private:
        ResRef<gfx::Texture2D> m_Texture = nullptr;
        Rectf                  m_TextureCoords{};
        Vector2f               m_Size{};
        Vector4f               m_Colour{};
        i32                    m_ZIndex = 0;

    public:
        Sprite() = default;
        Sprite(ResRef<gfx::Texture2D> texture, const Vector4f colour = { 1.0f, 1.0f, 1.0f, 1.0f }, const i32 zIndex = 0)
            : m_Texture(texture)
            , m_Colour(colour)
            , m_ZIndex(zIndex)
        {
            m_TextureCoords = { 0.0f, 0.0f, static_cast<f32>(texture->GetWidth()),
                                static_cast<f32>(texture->GetHeight()) };
            m_Size          = { m_TextureCoords.w, m_TextureCoords.h };
        }
        Sprite(ResRef<gfx::Texture2D> texture, const Rectf textureCoords,
               const Vector4f colour = { 1.0f, 1.0f, 1.0f, 1.0f }, const i32 zIndex = 0)
            : m_Texture(texture)
            , m_TextureCoords(textureCoords)
            , m_Colour(colour)
            , m_ZIndex(zIndex)
        {
            m_Size = { (f32)m_Texture->GetWidth(), (f32)m_Texture->GetHeight() };
        }

    public:
        static inline Sprite Empty() noexcept { return Sprite(); }

    public:
        [[nodiscard]] inline ResRef<gfx::Texture2D> GetTexture() const noexcept { return m_Texture; }
        inline void SetTexture(ResRef<gfx::Texture2D> newTexture) noexcept { Sprite{ newTexture }.Swap(*this); }
        CX_PROPERTY(TextureCoords)
        CX_PROPERTY(Colour)
        CX_PROPERTY(ZIndex)
        CX_PROPERTY(Size)

    public:
        inline void Swap(Sprite& other) noexcept
        {
            std::swap(m_Texture, other.m_Texture);
            std::swap(m_TextureCoords, other.m_TextureCoords);
            std::swap(m_Size, other.m_Size);
            std::swap(m_Colour, other.m_Colour);
            std::swap(m_ZIndex, other.m_ZIndex);
        }

    public:
        inline operator bool() const noexcept { return m_Texture; }

    public:
        void Serialize(ISerializationNode& node) const override
        {
            auto& texture_child = node.CreateChild("texture");
            m_Texture->Serialize(texture_child);
            node.Write("texture_coords", m_TextureCoords);
            node.Write("size", m_Size);
            node.Write("colour", m_Colour);
            node.Write("z_index", m_ZIndex);
        }
        void Deserialize(const ISerializationNode& node) override
        {
            if (m_Texture)
            {
                m_Texture.Reset();
            }

            auto&          texture_child = node.GetChild("texture");
            gfx::Texture2D texture;
            texture.Deserialize(texture_child);

            m_Texture = Resources::From<gfx::Texture2D>(std::move(texture));

            m_Texture->Deserialize(texture_child);
            node.Read("texture_coords", m_TextureCoords);
            node.Read("size", m_Size);
            node.Read("colour", m_Colour);
            node.Read("z_index", m_ZIndex);
        }
    }; // namespace codex
} // namespace codex

#endif // CODEX_SCENE_SPRITE_H
