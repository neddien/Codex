#include "IndexBuffer.h"
#include "Renderer.h"

#include <iostream>

namespace codex::opengl {
    IndexBuffer::IndexBuffer()
    {
        GL_Call(glGenBuffers(1, &m_RendererId));
        GL_Call(glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, m_RendererId));
        // GL_Call(glBufferData(GL_ELEMENT_ARRAY_BUFFER, indexCount * sizeof(uint32_t), data, GL_STATIC_DRAW));
    }

    IndexBuffer::~IndexBuffer()
    {
        GL_Call(glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0));
        GL_Call(glDeleteBuffers(1, &m_RendererId));
    }

    void IndexBuffer::Bind() const
    {
        GL_Call(glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, m_RendererId));
    }

    void IndexBuffer::Unbind() const
    {
        GL_Call(glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0));
    }

    void IndexBuffer::SetBuffer(const uint32_t* data, size_t indexCount, BufferUsage usage)
    {
        GL_Call(glGenBuffers(1, &m_RendererId));
        GL_Call(glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, m_RendererId));
        GL_Call(glBufferData(GL_ELEMENT_ARRAY_BUFFER, indexCount * sizeof(uint32_t), data, GL_STATIC_DRAW));
        m_Indicies = indexCount;
    }
} // namespace codex::opengl
