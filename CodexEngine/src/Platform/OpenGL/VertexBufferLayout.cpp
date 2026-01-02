#include "VertexBufferLayout.h"

namespace codex::opengl {
    template <>
    void VertexBufferLayout::Push<float>(const uint32_t count)
    {
        m_Elements.emplace_back(GL_FLOAT, count, GL_FALSE);
        m_Stride += sizeof(GLfloat) * count;
    }

    template <>
    void VertexBufferLayout::Push<uint8_t>(const uint32_t count)
    {
        m_Elements.emplace_back(GL_UNSIGNED_BYTE, count, GL_FALSE);
        m_Stride += sizeof(GLubyte) * count;
    }

    template <>
    void VertexBufferLayout::Push<int8_t>(const uint32_t count)
    {
        m_Elements.emplace_back(GL_BYTE, count, GL_TRUE);
        m_Stride += sizeof(GLbyte) * count;
    }

    template <>
    void VertexBufferLayout::Push<uint32_t>(const uint32_t count)
    {
        m_Elements.emplace_back(GL_UNSIGNED_INT, count, GL_FALSE);
        m_Stride += sizeof(GLuint) * count;
    }

    template <>
    void VertexBufferLayout::Push<int32_t>(const uint32_t count)
    {
        m_Elements.emplace_back(GL_INT, count, GL_FALSE);
        m_Stride += sizeof(GLint) * count;
    }
} // namespace codex::opengl
