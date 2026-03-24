#include "renderer.h"
#include "geometry.h"
#include "texture.h"
#include "vertex_buffer.h"
#include "vertex_buffer_layout.h"

codex::u32 gl_error_check()
{
    while (codex::u32 error_code = glGetError())
        return error_code;
    return 0;
}

namespace codex::opengl {
    Renderer::Renderer(const i32 width, const i32 height)
        : width_(width)
        , height_(height)
    {
        GL_ClearError();
        GL_Call(glViewport(0, 0, width_, height_));
        GL_Call(glEnable(GL_BLEND));
        GL_Call(glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA));
    }

    Renderer::~Renderer()
    {
    }

    void Renderer::render(const VertexArray* vertex_array, const IndexBuffer* index_buffer, const Shader* shader) const
    {
        vertex_array->bind();
        index_buffer->bind();
        shader->bind();
        GL_Call(glDrawElements(GL_TRIANGLES, index_buffer->indices_, GL_UNSIGNED_INT, nullptr));
    }

    void Renderer::render(const VertexArray* vertex_array, const Shader* shader, u32 count, u32 offset) const
    {
        vertex_array->bind();
        shader->bind();
        GL_Call(glDrawArrays(GL_TRIANGLES, offset, count));
    }

    void Renderer::render_line(const VertexArray* vertex_array, const IndexBuffer* index_buffer,
                              const Shader* shader) const
    {
        vertex_array->bind();
        index_buffer->bind();
        shader->bind();
        GL_Call(glDrawElements(GL_LINES, index_buffer->indices_, GL_UNSIGNED_INT, nullptr));
    }

    void Renderer::clear() const
    {
        GL_Call(glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT));
    }

    void Renderer::set_clear_colour(const f32 r, const f32 g, const f32 b, const f32 a) const
    {
        GL_Call(glClearColor(r, g, b, a));
    }
} // namespace codex::opengl
