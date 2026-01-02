#pragma once

#include <sdafx.h>

#include <Platform/OpenGL/Geometry.h>
#include <glm/glm.hpp>

namespace codex::math {
    using Matrix4f = glm::mat4;
    using Matrix2f = glm::mat2;
    using Vector2f = glm::vec2;
    using Vector3f = glm::vec3;
    using Vector4f = glm::vec4;
    using Vector2  = glm::ivec2;
    using Vector3  = glm::ivec3;
    using Vector4  = glm::ivec4;
    using Rect     = opengl::Rect;
    using Rectf    = opengl::Rectf;
} // namespace codex::math

using namespace codex::math;
