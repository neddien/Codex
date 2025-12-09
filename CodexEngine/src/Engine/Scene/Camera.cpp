#include "Public/Camera.h"

namespace codex::scene {
    [[nodiscard]] Vector3f Camera::ScreenCoordinatesToWorld(const Camera& camera, const Vector2f& screenCoord,
                                                            const Vector3f& cameraPosition) noexcept
    {
        const auto camera_dim =
            Vector3f{ camera.GetWidth() * camera.GetPan(), camera.GetHeight() * camera.GetPan(), 0.0f };
        return Vector3f{ screenCoord * camera.GetPan(), 0.0f } + cameraPosition - camera_dim / 2.0f;
    }
} // namespace codex::scene
