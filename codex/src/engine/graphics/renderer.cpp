#include "renderer.h"

namespace codex::gfx {
    void Renderer::init(const i32 width, const i32 height)
    {
        auto& inst              = get();
        inst.width_             = width;
        inst.height_            = height;
        inst.internal_renderer_ = new opengl::Renderer(width, height);

        GL_Call(glBindFramebuffer(GL_FRAMEBUFFER, 0));
        inst.log(Info, "Subsystem initialized.");
    }

    void Renderer::dispose() noexcept
    {
        auto& inst = get();
        if (inst.internal_renderer_)
            delete inst.internal_renderer_;

        inst.internal_renderer_ = nullptr;
        inst.width_             = 0;
        inst.height_            = 0;
        inst.log(Info, "Subsystem disposed.");
    }

    void Renderer::clear()
    {
        get().internal_renderer_->clear();
    }

    void Renderer::set_clear_colour(const f32 r, const f32 g, const f32 b, const f32 a)
    {
        get().internal_renderer_->set_clear_colour(r, g, b, a);
    }

    void Renderer::render(const opengl::VertexArray* vertex_array, const opengl::IndexBuffer* index_buffer,
                          const opengl::Shader* shader)
    {
        get().internal_renderer_->render(vertex_array, index_buffer, shader);
    }

    void Renderer::stencil_test(const bool enable) noexcept
    {
        get().internal_renderer_->stencil_test(enable);
    }

    void Renderer::stencil_mask(const u32 mask) noexcept
    {
        get().internal_renderer_->stencil_mask(mask);
    }

    void Renderer::stencil_op(const opengl::Enum sfail, const opengl::Enum dpfail, const opengl::Enum dppass) noexcept
    {
        get().internal_renderer_->stencil_op(sfail, dpfail, dppass);
    }

    void Renderer::stencil_fn(const opengl::Enum fn, const i32 ref, const u32 mask) noexcept
    {
        get().internal_renderer_->stencil_fn(fn, ref, mask);
    }

    void Renderer::colour_mask(const bool r, const bool g, const bool b, const bool a) noexcept
    {
        get().internal_renderer_->colour_mask(r, g, b, a);
    }

    void Renderer::resize_viewport(u16 new_width, u16 new_height, u16 x, u16 y) noexcept
    {
        get().internal_renderer_->resize_viewport(new_width, new_height, x, y);
    }
} // namespace codex::gfx
