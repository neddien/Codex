#include "Renderer.h"

namespace codex::gfx {
    i32            Renderer::m_Width            = 0;
    i32            Renderer::m_Height           = 0;
    opengl::Renderer* Renderer::m_InternalRenderer = nullptr;

    void Renderer::Init(const i32 width, const i32 height)
    {
        m_Width            = width;
        m_Height           = height;
        m_InternalRenderer = new opengl::Renderer(width, height);
        GL_Call(glBindFramebuffer(GL_FRAMEBUFFER, 0));
        lgx::Get("engine").Log(lgx::Info, "Renderer subsystem initialized.");
    }

    void Renderer::Dispose() noexcept
    {
        if (m_InternalRenderer)
            delete m_InternalRenderer;

        m_InternalRenderer = nullptr;
        m_Width            = 0;
        m_Height           = 0;
        lgx::Get("engine").Log(lgx::Info, "Renderer subsystem disposed.");
    }

    void Renderer::Clear()
    {
        m_InternalRenderer->Clear();
    }

    void Renderer ::SetClearColour(const f32 r, const f32 g, const f32 b, const f32 a)
    {
        m_InternalRenderer->SetClearColour(r, g, b, a);
    }

    void Renderer::Render(const opengl::VertexArray* vertexArray, const opengl::IndexBuffer* indexBuffer,
                          const opengl::Shader* shader)
    {
        m_InternalRenderer->Render(vertexArray, indexBuffer, shader);
    }
} // namespace codex::gfx
