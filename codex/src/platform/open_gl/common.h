#pragma once

#include <engine/core/public/common_def.h>

// Stupid macro defined by Xorg header, and then they say WinAPI is bad.
#ifdef None
#undef None
#endif
#ifdef Always
#undef Always
#endif

#ifndef __gl_h__
#include <glad/glad.h>
#endif

#define MGL_ASSERT(...) cxassert(__VA_ARGS__)

#define GL_ClearError() while (glGetError() != 0)
#ifdef MGL_DEBUG
#define GL_Call(x)                                                                                                     \
    GL_ClearError();                                                                                                   \
    x;                                                                                                                 \
    {                                                                                                                  \
        codex::u32 error_code = gl_error_check();                                                                      \
        MGL_ASSERT(error_code == 0, "GL Error occured! Error Code: " + std::to_string(error_code));                    \
    }
#else
#define GL_Call(x) x;
#endif

namespace codex::opengl {
    enum class BufferUsage
    {
        // Just for good measure
        STREAM_DRAW,
        STREAM_READ,
        STREAM_COPY,
        STATIC_DRAW,
        STATIC_READ,
        STATIC_COPY,
        DYNAMIC_DRAW,
        DYNAMIC_READ,
        DYNAMIC_COPY,
    };

    enum class Enum
    {
        Always,
        NotEqual,
        Less,
        Never,
        LEqual,
        Greater,
        GEqual,
        Keep,
        Zero,
        Replace,
        Incr,
        IncrWrap,
        Decr,
        DecrWrap,
        Invert,
    };

    [[nodiscard]] u32 to_glenum(const BufferUsage usage) noexcept;
    [[nodiscard]] u32 to_glenum(const Enum renum) noexcept;
} // namespace codex::opengl

CODEX_API codex::u32 gl_error_check();

/*
inline bool GL_LogCall(const char* functionName, const char* srcFile, const int line)
{
    while (uint32_t error = glGetError())
    {
        printf("[GL Error @ line %d]:\n\tFile: %s\n\tFunction: %s\n\tGL Error Code: 0x%X",
            line,
            srcFile,
            functionName,
            error);
        return false;
    }
    return true;
}
*/
