#include "Public/Camera.h"

namespace codex::scene {
    [[nodiscard]] Vector3f Camera::ScreenCoordinatesToWorld(const Camera& camera, const Vector2f& screenCoord,
                                                            const Vector3f& cameraPosition) noexcept
    {
        const auto camera_dim =
            Vector3f{ camera.GetWidth() * camera.GetPan(), camera.GetHeight() * camera.GetPan(), 0.0f };
        return Vector3f{ screenCoord * camera.GetPan(), 0.0f } + cameraPosition - camera_dim / 2.0f;
    }

    void Camera::Serialize(ISerializationNode& node) const
    {
        node.Write("width", m_Width);
        node.Write("height", m_Height);
        node.Write("fov", m_Fov);
        node.Write("near_clip", m_NearClip);
        node.Write("far_clip", m_FarClip);
        node.Write("projection_type", static_cast<u32>(m_ProjectionType));
        node.Write("pan", m_Pan);
    }

    void Camera::Deserialize(const ISerializationNode& node)
    {
        node.Read("width", m_Width);
        node.Read("height", m_Height);
        node.Read("fov", m_Fov);
        node.Read("near_clip", m_NearClip);
        node.Read("far_clip", m_FarClip);
        node.Read("projection_type", reinterpret_cast<u32&>(m_ProjectionType));
        node.Read("pan", m_Pan);
    }
} // namespace codex::scene
