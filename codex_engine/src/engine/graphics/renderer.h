#pragma once

#include <engine/core/public/geometry.h>
#include <engine/graphics/batch_renderer2d.h>
#include <engine/graphics/debug_draw.h>
#include <engine/graphics/line2d.h>
#include <engine/scene/public/sprite.h>

#include "public/shader.h"

namespace codex::gfx {
    class CODEX_API Renderer
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

    private:
        static i32               s_width_;
        static i32               s_height_;
        static opengl::Renderer* s_internal_renderer_;
    };
} // namespace codex::gfx
