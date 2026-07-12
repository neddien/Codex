#pragma once

#include <engine/core/public/geometry.h>
#include <engine/graphics/batch_renderer2d.h>
#include <engine/graphics/debug_draw.h>
#include <engine/graphics/line2d.h>
#include <engine/scene/public/sprite.h>

namespace codex::gfx {
    class CODEX_API Renderer : public System<Renderer>, public Loggable<"Renderer">
    {
    public:
        Renderer() = default;

    public:
        static void init(const i32 width, const i32 height);
        static void dispose() noexcept;
        static void clear();
        static void set_clear_colour(const f32 r, const f32 g, const f32 b, const f32 a);
        static void render(const opengl::VertexArray* vertex_array, const opengl::IndexBuffer* index_buffer,
                           const opengl::Shader* shader);
        static void stencil_test(const bool enable) noexcept;
        static void stencil_mask(const u32 mask) noexcept;
        static void stencil_op(const opengl::Enum sfail, const opengl::Enum dpfail, const opengl::Enum dppass) noexcept;
        static void stencil_fn(const opengl::Enum fn, const i32 ref, const u32 mask) noexcept;
        static void colour_mask(const bool r, const bool g, const bool b, const bool a) noexcept;
        static void resize_viewport(u16 new_width, u16 new_height, u16 x = 0, u16 y = 0) noexcept;

    private:
        i32               width_;
        i32               height_;
        opengl::Renderer* internal_renderer_;
    };
} // namespace codex::gfx
