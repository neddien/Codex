#pragma once

#include <cstdint>

#include "renderer.h"

namespace codex::opengl {
    class CODEX_API VertexBuffer
    {
    public:
        VertexBuffer();
        ~VertexBuffer();

    public:
        [[nodiscard]] inline usize vertex_count() const { return vertex_count_; }

        template <typename T>
        void set_buffer(const void* data, const usize size, const BufferUsage usage = BufferUsage::STATIC_DRAW)
        {
            GL_Call(glBindBuffer(GL_ARRAY_BUFFER, renderer_id_));
            GL_Call(glBufferData(GL_ARRAY_BUFFER, size, data, (GLenum)(usage)));
            vertex_count_ = size / sizeof(T);
        }

        template <typename T>
        void set_buffer_sub_data(const void* data, const usize offset, const usize size)
        {
            GL_Call(glBindBuffer(GL_ARRAY_BUFFER, renderer_id_));
            GL_Call(glBufferSubData(GL_ARRAY_BUFFER, offset, size, data));
            vertex_count_ = size / sizeof(T);
        }

    public:
        void bind() const;
        void unbind() const;

    private:
        u32   renderer_id_;
        usize vertex_count_;
    };
} // namespace codex::opengl
