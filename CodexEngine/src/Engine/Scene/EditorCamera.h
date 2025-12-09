#pragma once

#include <sdafx.h>

#include "Public/Camera.h"

namespace codex::scene {
    class CODEX_API EditorCamera : public Camera
    {
    private:
        Vector3f m_Pos = Vector3f(0.0f);
        // TODO: Uncomment for perspective camera.
        // Vector3f         m_Rotation   = Vector3f(0.0f);
        // Vector3f         m_FocalPoint = Vector3f(0.0f);
        mutable Matrix4f m_ViewMat;

    public:
        using Camera::Camera;

    public:
        [[nodiscard]] inline Vector3f GetPos() const noexcept { return m_Pos; }
        inline void                   SetPos(Vector3f newPos) noexcept { m_Pos = std::move(newPos); }
        [[nodiscard]] inline Matrix4f GetViewMatrix() const noexcept
        {
            const auto camera_front = Vector3f(0.0f, 0.0f, -1.0f);
            const auto camera_up    = Vector3f(0.0f, 1.0f, 0.0f);
            m_ViewMat               = glm::lookAt(m_Pos, m_Pos + camera_front, camera_up);
            return m_ViewMat;
        }
    };
} // namespace codex::scene
