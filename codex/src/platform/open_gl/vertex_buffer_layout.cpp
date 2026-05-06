#include "vertex_buffer_layout.h"

namespace codex::opengl {
    template <>
    void VertexBufferLayout::push<f32>(const u32 count)
    {
        elements_.emplace_back(GL_FLOAT, count, GL_FALSE);
        stride_ += sizeof(GLfloat) * count;
    }

    template <>
    void VertexBufferLayout::push<u8>(const u32 count)
    {
        elements_.emplace_back(GL_UNSIGNED_BYTE, count, GL_FALSE);
        stride_ += sizeof(GLubyte) * count;
    }

    template <>
    void VertexBufferLayout::push<i8>(const u32 count)
    {
        elements_.emplace_back(GL_BYTE, count, GL_TRUE);
        stride_ += sizeof(GLbyte) * count;
    }

    template <>
    void VertexBufferLayout::push<u32>(const u32 count)
    {
        elements_.emplace_back(GL_UNSIGNED_INT, count, GL_FALSE);
        stride_ += sizeof(GLuint) * count;
    }

    template <>
    void VertexBufferLayout::push<i32>(const u32 count)
    {
        elements_.emplace_back(GL_INT, count, GL_FALSE);
        stride_ += sizeof(GLint) * count;
    }
} // namespace codex::opengl
