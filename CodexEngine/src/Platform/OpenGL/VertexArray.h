#pragma once

#include "Constants.h"

namespace codex::opengl {
    // Forward declerations
    class VertexBuffer;
    class VertexBufferLayout;

    class CODEX_API VertexArray
    {
    private:
        uint32_t m_RendererId;

    public:
        VertexArray();
        ~VertexArray();

    public:
        void Bind() const;
        void Unbind() const;
        void AddBuffer(const VertexBuffer* vbo, const VertexBufferLayout* layout) const;
    };
} // namespace codex::opengl
