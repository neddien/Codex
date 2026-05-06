#include "vertex_array.h"
#include "renderer.h"
#include "vertex_buffer.h"
#include "vertex_buffer_layout.h"

namespace codex::opengl {
    VertexArray::VertexArray()
    {
        GL_Call(glGenVertexArrays(1, &renderer_id_));
        GL_Call(glBindVertexArray(renderer_id_));
    }

    VertexArray::~VertexArray()
    {
        GL_Call(glBindVertexArray(0));
        GL_Call(glDeleteVertexArrays(1, &renderer_id_));
    }

    void VertexArray::bind() const
    {
        GL_Call(glBindVertexArray(renderer_id_));
    }

    void VertexArray::unbind() const
    {
        GL_Call(glBindVertexArray(0));
    }

    void VertexArray::add_buffer(const VertexBuffer* vbo, const VertexBufferLayout* layout) const
    {
        bind();
        vbo->bind();
        const auto& elements = layout->elements();
        u32         stride   = layout->stride();
        u64         offset   = 0;
        for (u32 i = 0; i < elements.size(); ++i) {
            switch (elements[i].type) {
                case GL_BYTE:
                case GL_UNSIGNED_BYTE:
                case GL_SHORT:
                case GL_UNSIGNED_SHORT:
                case GL_INT:
                case GL_UNSIGNED_INT:
                    GL_Call(
                        glVertexAttribIPointer(i, elements[i].count, elements[i].type, stride, (const void*)offset));
                    break;
                default:
                    GL_Call(glVertexAttribPointer(i, elements[i].count, elements[i].type, elements[i].normalized,
                                                  stride, (const void*)offset));
                    break;
            }
            GL_Call(glEnableVertexAttribArray(i)); // I KEEP FUCKING FORGETING THIS HOLY SHIT
            offset += VertexBufferElement::size_of_type(elements[i].type) * elements[i].count;
        }
    }
} // namespace codex::opengl
