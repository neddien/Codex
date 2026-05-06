#pragma once

#include "public/camera.h"

namespace codex::scene {
    class CODEX_API EditorCamera : public Camera
    {
    public:
        using Camera::Camera;

    public:
        [[nodiscard]] inline Vector3f pos() const noexcept { return pos_; }
        inline void                   set_pos(Vector3f new_pos) noexcept { pos_ = std::move(new_pos); }
        [[nodiscard]] inline Matrix4f view_matrix() const noexcept
        {
            const auto camera_front = Vector3f(0.0f, 0.0f, -1.0f);
            const auto camera_up    = Vector3f(0.0f, 1.0f, 0.0f);
            view_mat_               = glm::lookAt(pos_, pos_ + camera_front, camera_up);
            return view_mat_;
        }

    private:
        Vector3f pos_ = Vector3f(0.0f);
        // TODO: Uncomment for perspective camera.
        // Vector3f         rotation_    = Vector3f(0.0f);
        // Vector3f         focal_point_ = Vector3f(0.0f);
        mutable Matrix4f view_mat_;
    };
} // namespace codex::scene
