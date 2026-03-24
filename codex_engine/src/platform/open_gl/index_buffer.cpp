#include "index_buffer.h"
#include "renderer.h"

#include <iostream>

namespace codex::opengl {
    IndexBuffer::IndexBuffer()
    {
        GL_Call(glGenBuffers(1, &renderer_id_));
        GL_Call(glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, renderer_id_));
    }

    IndexBuffer::~IndexBuffer()
    {
        GL_Call(glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0));
        GL_Call(glDeleteBuffers(1, &renderer_id_));
    }

    void IndexBuffer::bind() const
    {
        GL_Call(glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, renderer_id_));
    }

    void IndexBuffer::unbind() const
    {
        GL_Call(glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0));
    }

    void IndexBuffer::set_buffer(const u32* data, size_t index_count, BufferUsage usage)
    {
        GL_Call(glGenBuffers(1, &renderer_id_));
        GL_Call(glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, renderer_id_));
        GL_Call(glBufferData(GL_ELEMENT_ARRAY_BUFFER, index_count * sizeof(u32), data, GL_STATIC_DRAW));
        indices_ = index_count;
    }
} // namespace codex::opengl
