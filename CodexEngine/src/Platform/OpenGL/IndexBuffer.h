#pragma once

#include <cstdint>

#include "Constants.h"

namespace codex::opengl {
    // Forward declerations
    class Renderer;

    class IndexBuffer
    {
        friend class Renderer;

    private:
        uint32_t m_RendererId;
        size_t   m_Indicies;

    public:
        IndexBuffer();
        ~IndexBuffer();

    public:
        inline size_t GetIndexCount() const { return m_Indicies; }

    public:
        void Bind() const;
        void Unbind() const;
        void SetBuffer(const uint32_t* data, const size_t indexCount,
                       const BufferUsage usage = BufferUsage::STATIC_DRAW);
        void SetBufferSubData(const uint32_t* data, const size_t indexCount);
    };
} // namespace codex::opengl
