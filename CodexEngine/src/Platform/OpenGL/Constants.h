#pragma once

// Stupid macro defined by Xorg header, and then they say WinAPI is bad.
#ifdef None
#undef None
#endif

#include <glad.h>
#include <sdafx.h>

#define MGL_ASSERT(...) CX_ASSERT(__VA_ARGS__)

#define GL_ClearError() while (glGetError() != 0)
#ifdef MGL_DEBUG
#define GL_Call(x)                                                                                                     \
    GL_ClearError();                                                                                                   \
    x;                                                                                                                 \
    {                                                                                                                  \
        uint32_t error_code = GL_ErrorCheck();                                                                         \
        MGL_ASSERT(error_code == 0, "GL Error occured! Error Code: " + std::to_string(error_code))                     \
    }
#else
#define GL_Call(x) x;
#endif

namespace codex::opengl {
    enum class BufferUsage
    {
        // Just for good measure
        STREAM_DRAW  = GL_STREAM_DRAW,
        STREAM_READ  = GL_STREAM_READ,
        STREAM_COPY  = GL_STREAM_COPY,
        STATIC_DRAW  = GL_STATIC_DRAW,
        STATIC_READ  = GL_STATIC_READ,
        STATIC_COPY  = GL_STATIC_COPY,
        DYNAMIC_DRAW = GL_DYNAMIC_DRAW,
        DYNAMIC_READ = GL_DYNAMIC_READ,
        DYNAMIC_COPY = GL_DYNAMIC_COPY
    };
} // namespace codex::opengl

CODEX_API uint32_t GL_ErrorCheck();

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
