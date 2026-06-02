#include "public/camera.h"

namespace codex::scene {
    [[nodiscard]] Vector3f Camera::screen_coordinates_to_world(const Camera& camera, const Vector2f& screen_coord,
                                                               const Vector3f& camera_position) noexcept
    {
        const auto camera_dim = Vector3f{ camera.width() * camera.pan(), camera.height() * camera.pan(), 0.0f };
        return Vector3f{ screen_coord * camera.pan(), 0.0f } + camera_position - camera_dim / 2.0f;
    }

    void Camera::archive(Archive& ar)
    {
        ar("width", width_);
        ar("height", height_);
        ar("fov", fov_);
        ar("near_clip", near_clip_);
        ar("far_clip", far_clip_);
        ar("projection_type", projection_type_);
        ar("pan", pan_);
    }
} // namespace codex::scene
