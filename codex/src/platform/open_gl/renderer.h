#pragma once

#include <array>
#include <cstdint>
#include <cstdio>
#include <glad/glad.h>

#include "common.h"
#include "index_buffer.h"
#include "shader.h"
#include "vertex_array.h"

namespace codex::opengl {
    // Forward declerations
    class Texture;
    struct irect;
    struct rect;

    class CODEX_API Renderer
    {
    public:
        Renderer(const i32 width, const i32 height);
        ~Renderer();

    public:
        void render(const VertexArray* vertex_array, const IndexBuffer* index_buffer, const Shader* shader) const;
        void render(const VertexArray* vertex_array, const Shader* shader, u32 count, u32 offset = 0) const;
        void render_line(const VertexArray* vertex_array, const IndexBuffer* index_buffer, const Shader* shader) const;
        void clear() const;
        void set_clear_colour(const f32 r, const f32 g, const f32 b, const f32 a) const;
        void stencil_test(const bool enable) noexcept;
        void stencil_mask(const u32 mask) noexcept;
        void stencil_op(const Enum sfail, const Enum dpfail, const Enum dppass) noexcept;
        void stencil_fn(const Enum fn, const i32 ref, const u32 mask) noexcept;
        void colour_mask(const bool r, const bool g, const bool b, const bool a) noexcept;
        void resize_viewport(u16 new_width, u16 new_height, u16 x = 0, u16 y = 0) noexcept;

    private:
        i32 width_;
        i32 height_;
    };
} // namespace codex::opengl
