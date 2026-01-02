#pragma once

#include <sdafx.h>

#include <Engine/Core/Public/Geomtryd.h>
#include <Platform/OpenGL/IndexBuffer.h>
#include <Platform/OpenGL/VertexArray.h>
#include <Platform/OpenGL/VertexBuffer.h>
#include <Platform/OpenGL/VertexBufferLayout.h>

#include "Public/Shader.h"
#include "Public/Texture2D.h"

namespace codex::gfx {
    constexpr auto QUAD2D_VERTEX_COUNT = 4; // How many vertices does the buffer have?

    CX_PACKED(struct QuadVertex {
        Matrix4f model;
        Vector4f vertex;
        Vector4f colour;
        Vector2f texCoord;
        Vector2f texSize;
        i32      texId;
        i32      entityId;
    });

    class CODEX_API RenderBatch
    {
    private:
        i32                                         m_QuadCount           = 0;
        i32                                         m_MaxTextureSlotCount = 32;
        i32                                         m_MaxQuadCount        = 1000;
        i32                                         m_ZIndex              = 0;
        bool                                        m_HasRoom             = true;
        QuadVertex*                                 m_Verticies           = nullptr;
        QuadVertex*                                 m_VertexPtr           = nullptr;
        std::unique_ptr<opengl::VertexArray>        m_Vao                 = nullptr;
        std::unique_ptr<opengl::VertexBuffer>       m_Vbo                 = nullptr;
        std::unique_ptr<opengl::IndexBuffer>        m_Ebo                 = nullptr;
        std::unique_ptr<opengl::VertexBufferLayout> m_Layout              = nullptr;
        Shader*                                     m_Shader              = nullptr;
        std::vector<Texture2D*>                     m_TextureList;
        u16                                         m_CurrentTexIndex = 0;

    public:
        RenderBatch() = default;
        RenderBatch(const i32 maxQuadCount, Shader* shader);
        RenderBatch(RenderBatch&& other) noexcept;
        RenderBatch& operator=(RenderBatch&& other) noexcept;
        ~RenderBatch();

    public:
        bool        HasRoom() const { return m_HasRoom; }
        i32         GetZIndex() const { return m_ZIndex; }
        i32         GetCount() const { return m_QuadCount; }
        void        SetZIndex(i32 newIndex) { m_ZIndex = newIndex; }
        void        BindShader(Shader* shader) { m_Shader = shader; }
        QuadVertex* GetQuads() noexcept { return m_Verticies; }
        void        Swap(RenderBatch& other) noexcept
        {
            std::swap(m_QuadCount, other.m_QuadCount);
            std::swap(m_MaxTextureSlotCount, other.m_MaxQuadCount);
            std::swap(m_MaxQuadCount, other.m_MaxQuadCount);
            std::swap(m_ZIndex, other.m_ZIndex);
            std::swap(m_HasRoom, other.m_HasRoom);
            std::swap(m_Verticies, other.m_Verticies);
            std::swap(m_VertexPtr, other.m_VertexPtr);
            std::swap(m_Vao, other.m_Vao);
            std::swap(m_Vbo, other.m_Vbo);
            std::swap(m_Ebo, other.m_Ebo);
            std::swap(m_Layout, other.m_Layout);
            std::swap(m_Shader, other.m_Shader);
            std::swap(m_TextureList, other.m_TextureList);
            std::swap(m_CurrentTexIndex, other.m_CurrentTexIndex);
        }

    public:
        void Flush();
        bool UploadQuad(Texture2D* texture, const Rectf& srcRect, const Matrix4f& transform, const Vector4f& colour,
                        const i32 entityId);
        std::vector<u32> GenerateIndicies(u32& size);
        void             Render();
    };
} // namespace codex::gfx
