#pragma once

#include "public/camera.h"

namespace codex::scene {
    class CODEX_API EditorCamera : public Camera
    {
    public:
        using Camera::Camera;

    public:
        [[nodiscard]] inline vec3 pos() const noexcept { return pos_; }
        inline void                set_pos(vec3 new_pos) noexcept { pos_ = std::move(new_pos); }
        [[nodiscard]] inline mat4  view_matrix() const noexcept
        {
            const auto camera_front = vec3(0.0f, 0.0f, -1.0f);
            const auto camera_up    = vec3(0.0f, 1.0f, 0.0f);
            view_mat_               = glm::lookAt(pos_, pos_ + camera_front, camera_up);
            return view_mat_;
        }

    private:
        vec3 pos_ = vec3(0.0f);
        // TODO: Uncomment for perspective camera.
        // vec3         rotation_    = vec3(0.0f);
        // vec3         focal_point_ = vec3(0.0f);
        mutable mat4 view_mat_;
    };
} // namespace codex::scene
