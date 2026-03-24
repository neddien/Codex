#include "renderer.h"

namespace codex::gfx {
    i32               Renderer::s_width_             = 0;
    i32               Renderer::s_height_            = 0;
    opengl::Renderer* Renderer::s_internal_renderer_ = nullptr;

    void Renderer::init(const i32 width, const i32 height)
    {
        s_width_             = width;
        s_height_            = height;
        s_internal_renderer_ = new opengl::Renderer(width, height);
        GL_Call(glBindFramebuffer(GL_FRAMEBUFFER, 0));
        codex::info("Renderer subsystem initialized.");
    }

    void Renderer::dispose() noexcept
    {
        if (s_internal_renderer_)
            delete s_internal_renderer_;

        s_internal_renderer_ = nullptr;
        s_width_             = 0;
        s_height_            = 0;
        codex::info("Renderer subsystem disposed.");
    }

    void Renderer::clear()
    {
        s_internal_renderer_->clear();
    }

    void Renderer::set_clear_colour(const f32 r, const f32 g, const f32 b, const f32 a)
    {
        s_internal_renderer_->set_clear_colour(r, g, b, a);
    }

    void Renderer::render(const opengl::VertexArray* vertex_array, const opengl::IndexBuffer* index_buffer,
                          const opengl::Shader* shader)
    {
        s_internal_renderer_->render(vertex_array, index_buffer, shader);
    }
} // namespace codex::gfx
