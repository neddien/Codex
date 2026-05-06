#pragma once

#include <array>
#include <cstdint>
#include <cstdio>
#include <glad/glad.h>

#include "constants.h"
#include "index_buffer.h"
#include "shader.h"
#include "vertex_array.h"

namespace codex::opengl {
    // Forward declerations
    class Texture;
    struct Rect;
    struct Rectf;

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

    private:
        i32 width_;
        i32 height_;
        u32 stride_ = 9 * sizeof(f32);
    };
} // namespace codex::opengl
