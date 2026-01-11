#pragma once

#include <cstdint>

#include "Renderer.h"

namespace codex::opengl {
    class CODEX_API VertexBuffer
    {
    private:
        uint32_t m_RendererId;
        size_t   m_VertexCount;

    public:
        VertexBuffer();
        ~VertexBuffer();

    public:
        inline size_t GetVertexCount() const { return m_VertexCount; }
        template <typename T>
        void SetBuffer(const void* data, const size_t size, const BufferUsage usage = BufferUsage::STATIC_DRAW)
        {
            GL_Call(glBindBuffer(GL_ARRAY_BUFFER, m_RendererId));
            GL_Call(glBufferData(GL_ARRAY_BUFFER, size, data, (GLenum)(usage)));
            m_VertexCount = size / sizeof(T);
        }
        template <typename T>
        void SetBufferSubData(const void* data, const size_t offset, const size_t size)
        {
            GL_Call(glBindBuffer(GL_ARRAY_BUFFER, m_RendererId));
            GL_Call(glBufferSubData(GL_ARRAY_BUFFER, offset, size, data));
            m_VertexCount = size / sizeof(T);
        }

    public:
        void Bind() const;
        void Unbind() const;
    };
} // namespace codex::opengl
