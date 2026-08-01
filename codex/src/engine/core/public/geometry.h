#pragma once

#include <engine/core/public/common_third_party_libs.h>
#include <platform/open_gl/geometry.h>

namespace codex::math {
    using mat4  = glm::mat4;
    using mat2  = glm::mat2;
    using vec2  = glm::vec2;
    using vec3  = glm::vec3;
    using vec4  = glm::vec4;
    using ivec2 = glm::ivec2;
    using ivec3 = glm::ivec3;
    using ivec4 = glm::ivec4;
    using quat  = glm::quat;
    using fquat = glm::fquat;
    using rect  = opengl::rect;
    using irect = opengl::irect;

    struct transform
    {
        vec3 position{ 0.0f };
        vec3 rotation{ 0.0f };
        vec3 scale{ 1.0f, 1.0f, 1.0f };
    };
} // namespace codex::math

using namespace codex::math;
