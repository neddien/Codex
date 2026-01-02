#pragma once

// Stupid macro defined by Xorg header, and then they say WinAPI is bad.
#ifdef None
#undef None
#endif

#include <glad.h>
#include <iostream>
#include <stdio.h>
#include <string>

#ifdef __GNUC__
#define MGL_DEBUG_TRAP() __builtin_trap()
#elif _MSC_VER
#define MGL_DEBUG_TRAP() __debugbreak()
#elif __clang__
#define MGL_DEBUG_TRAP() __builtin_debugtrap()
#endif

#define MGL_ASSERT(x, msg)                                                                                             \
    if (!(x))                                                                                                          \
    {                                                                                                                  \
        std::cerr << "[MGL_DEBUG] :: Assertion failed: " << msg << "\n\tStack trace:"                                  \
                  << "\n\t\tFunction: " << __FUNCTION__ << "\n\t\tFile: " << __FILE__ << "\n\t\tLine: " << __LINE__    \
                  << std::endl;                                                                                        \
        MGL_DEBUG_TRAP();                                                                                              \
    }

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

inline uint32_t GL_ErrorCheck()
{
    while (uint32_t errorCode = glGetError())
        return errorCode;
    return 0;
}

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
