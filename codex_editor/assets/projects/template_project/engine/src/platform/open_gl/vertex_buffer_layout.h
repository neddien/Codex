#pragma once

#include "constants.h"

#include <glad.h>

namespace codex::opengl {
    struct CODEX_API VertexBufferElement
    {
    public:
        u32  type;
        u32  count;
        bool normalized;

    public:
        VertexBufferElement(const u32 type, const u32 count, const bool normalized)
            : type(type)
            , count(count)
            , normalized(normalized)
        {
        }

    public:
        [[nodiscard]] static u32 size_of_type(const u32 type)
        {
            switch (type) {
                case GL_UNSIGNED_INT:
                case GL_FLOAT:
                case GL_INT: return sizeof(GLint);
                case GL_UNSIGNED_BYTE:
                case GL_BYTE: return sizeof(GLbyte);
                case GL_UNSIGNED_SHORT:
                case GL_SHORT: return sizeof(GLshort);
                case GL_DOUBLE: return sizeof(GLdouble);
                default: CX_ASSERT(false, "Unknown type."); return 0;
            }
        }
    };

    class VertexBufferLayout
    {
    public:
        VertexBufferLayout()
            : stride_(0) {};

    public:
        [[nodiscard]] inline const std::vector<VertexBufferElement>& elements() const { return elements_; }
        [[nodiscard]] inline u32                                     stride() const { return stride_; }

    public:
        inline void clear()
        {
            elements_.clear();
            stride_ = 0;
        }

        template <typename T>
        void push(const u32 count)
        {
            char msg[256];
            std::sprintf(msg, "Couldn't push! T type: %s\n", typeid(T).name());
            CX_ASSERT(false, msg);
        }

    private:
        std::vector<VertexBufferElement> elements_;
        u32                              stride_;
    };

    template <>
    void VertexBufferLayout::push<f32>(const u32 count);
    template <>
    void VertexBufferLayout::push<u8>(const u32 count);
    template <>
    void VertexBufferLayout::push<i8>(const u32 count);
    template <>
    void VertexBufferLayout::push<u32>(const u32 count);
    template <>
    void VertexBufferLayout::push<i32>(const u32 count);
} // namespace codex::opengl
