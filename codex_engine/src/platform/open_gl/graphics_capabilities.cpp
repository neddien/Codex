#include "graphics_capabilities.h"

#include "constants.h"

namespace codex::opengl {
    namespace capabilities {
        int max_texture_slot_count() noexcept
        {
            int max_slot_count;
            GL_Call(glGetIntegerv(GL_MAX_TEXTURE_IMAGE_UNITS, &max_slot_count));
            return max_slot_count;
        }
    } // namespace capabilities
} // namespace codex::opengl
