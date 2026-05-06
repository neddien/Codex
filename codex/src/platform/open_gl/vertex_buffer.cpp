#include "vertex_buffer.h"
#include "renderer.h"
#include "vertex_array.h"

namespace codex::opengl {
    VertexBuffer::VertexBuffer()
    {
        GL_Call(glGenBuffers(1, &renderer_id_));
        GL_Call(glBindBuffer(GL_ARRAY_BUFFER, renderer_id_));
    }

    VertexBuffer::~VertexBuffer()
    {
        GL_Call(glBindBuffer(GL_ARRAY_BUFFER, renderer_id_));
        glDeleteBuffers(1, &renderer_id_);
    }

    void VertexBuffer::bind() const
    {
        GL_Call(glBindBuffer(GL_ARRAY_BUFFER, renderer_id_));
    }

    void VertexBuffer::unbind() const
    {
        GL_Call(glBindBuffer(GL_ARRAY_BUFFER, 0));
    }
} // namespace codex::opengl
