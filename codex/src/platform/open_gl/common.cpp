#include "common.h"

#include <glad/glad.h>

namespace codex::opengl {
    u32 to_glenum(const BufferUsage usage) noexcept
    {
        switch (usage) {
            using enum BufferUsage;
            case STREAM_DRAW: return GL_STREAM_DRAW;
            case STREAM_READ: return GL_STREAM_READ;
            case STREAM_COPY: return GL_STREAM_COPY;
            case STATIC_DRAW: return GL_STATIC_DRAW;
            case STATIC_READ: return GL_STATIC_READ;
            case STATIC_COPY: return GL_STATIC_COPY;
            case DYNAMIC_DRAW: return GL_DYNAMIC_DRAW;
            case DYNAMIC_READ: return GL_DYNAMIC_READ;
            case DYNAMIC_COPY: return GL_DYNAMIC_COPY;
        }
        return GL_STATIC_DRAW;
    }

    u32 to_glenum(const Enum renum) noexcept
    {
        switch (renum) {
            using enum Enum;

            case Always: return GL_ALWAYS;
            case NotEqual: return GL_NOTEQUAL;
            case Less: return GL_LESS;
            case Never: return GL_NEVER;
            case LEqual: return GL_LEQUAL;
            case Greater: return GL_GREATER;
            case GEqual: return GL_GEQUAL;
            case Keep: return GL_KEEP;
            case Zero: return GL_ZERO;
            case Replace: return GL_REPLACE;
            case Incr: return GL_INCR;
            case IncrWrap: return GL_INCR_WRAP;
            case Decr: return GL_DECR;
            case DecrWrap: return GL_DECR_WRAP;
            case Invert: return GL_INVERT;
        }
        return GL_NONE;
    }
} // namespace codex::opengl
