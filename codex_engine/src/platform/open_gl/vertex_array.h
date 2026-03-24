#pragma once

#include "constants.h"

namespace codex::opengl {
    // Forward declerations
    class VertexBuffer;
    class VertexBufferLayout;

    class CODEX_API VertexArray
    {
    public:
        VertexArray();
        ~VertexArray();

    public:
        void bind() const;
        void unbind() const;
        void add_buffer(const VertexBuffer* vbo, const VertexBufferLayout* layout) const;

    private:
        u32 renderer_id_;
    };
} // namespace codex::opengl
