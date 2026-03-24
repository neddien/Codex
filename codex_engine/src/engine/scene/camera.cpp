#include "public/camera.h"

namespace codex::scene {
    [[nodiscard]] Vector3f Camera::screen_coordinates_to_world(const Camera& camera, const Vector2f& screen_coord,
                                                               const Vector3f& camera_position) noexcept
    {
        const auto camera_dim = Vector3f{ camera.width() * camera.pan(), camera.height() * camera.pan(), 0.0f };
        return Vector3f{ screen_coord * camera.pan(), 0.0f } + camera_position - camera_dim / 2.0f;
    }

    void Camera::serialize(ISerializationNode& node) const
    {
        node.write("width", width_);
        node.write("height", height_);
        node.write("fov", fov_);
        node.write("near_clip", near_clip_);
        node.write("far_clip", far_clip_);
        node.write("projection_type", static_cast<u32>(projection_type_));
        node.write("pan", pan_);
    }

    void Camera::deserialize(const ISerializationNode& node)
    {
        node.read("width", width_);
        node.read("height", height_);
        node.read("fov", fov_);
        node.read("near_clip", near_clip_);
        node.read("far_clip", far_clip_);
        node.read("projection_type", reinterpret_cast<u32&>(projection_type_));
        node.read("pan", pan_);
    }
} // namespace codex::scene
