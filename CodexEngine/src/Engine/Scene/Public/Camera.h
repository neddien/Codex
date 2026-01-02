#ifndef CODEX_CORE_CAMERA_H
#define CODEX_CORE_CAMERA_H

#include <sdafx.h>

#include <Engine/Core/Public/Serializer.h>

namespace codex::scene {
    class CODEX_API Camera : public ISerializable
    {
    public:
        enum class ProjectionType
        {
            Orthographic,
            Perspective
        };

    private:
        i32            m_Width;
        i32            m_Height;
        f32            m_Fov;
        f32            m_NearClip;
        f32            m_FarClip;
        ProjectionType m_ProjectionType;
        Matrix4f       m_Projection;
        f32            m_Pan;

    public:
        Camera(const i32 width = 1280, const i32 height = 720, const f32 nearClip = 0.0f, const f32 farClip = 100.0f,
               const ProjectionType projectionType = ProjectionType::Orthographic, const f32 fov = 90.0f) noexcept
            : m_Width(width)
            , m_Height(height)
            , m_Fov(fov)
            , m_NearClip(nearClip)
            , m_FarClip(farClip)
            , m_ProjectionType(projectionType)
            , m_Projection(Matrix4f(1.0f))
            , m_Pan(1.0f)
        {
            UpdateProjectionMatrix();
        }

    public:
        inline void UpdateProjectionMatrix() noexcept
        {
            if (m_ProjectionType == ProjectionType::Orthographic)
                m_Projection =
                    glm::ortho((f32)m_Width * m_Pan / -2.0f, (f32)m_Width * m_Pan / 2.0f, (f32)m_Height * m_Pan / -2.0f,
                               (f32)m_Height * m_Pan / 2.0f, m_NearClip, m_FarClip);
            else
                m_Projection = glm::perspective(m_Fov, (f32)m_Width / (f32)m_Height, m_NearClip, m_FarClip);
        }
        CX_PROPERTY(Pan)
        [[nodiscard]] inline auto GetProjectionMatrix() const noexcept { return m_Projection; }
        [[nodiscard]] inline auto GetWidth() const noexcept { return m_Width; }
        [[nodiscard]] inline auto GetHeight() const noexcept { return m_Height; }
        [[nodiscard]] inline auto GetProjectionType() const noexcept { return m_ProjectionType; }
        [[nodiscard]] inline auto GetFOV() const noexcept { return m_Fov; }
        [[nodiscard]] inline auto GetNearClip() const noexcept { return m_NearClip; }
        [[nodiscard]] inline auto GetFarClip() const noexcept { return m_FarClip; }
        inline void               SetProjectionType(const ProjectionType projectionType) noexcept
        {
            m_ProjectionType = projectionType;
            UpdateProjectionMatrix();
        }
        inline void SetFOV(const f32 fov) noexcept
        {
            m_Fov = fov;
            UpdateProjectionMatrix();
        }
        inline void SetNearClip(const f32 nearClip) noexcept { m_NearClip = nearClip; }
        inline void SetFarClip(const f32 farClip) noexcept { m_FarClip = farClip; }
        inline void SetWidth(const i32 newWidth) noexcept
        {
            m_Width = newWidth;
            UpdateProjectionMatrix();
        }
        inline void SetHeight(const i32 newHeight) noexcept
        {
            m_Height = newHeight;
            UpdateProjectionMatrix();
        }

    public:
        [[nodiscard]] static Vector3f ScreenCoordinatesToWorld(const Camera& camera, const Vector2f& screenCoord,
                                                               const Vector3f& cameraPosition) noexcept;

    public:
        void Serialize(ISerializationNode& node) const override;
        void Deserialize(const ISerializationNode& node) override;
    };
} // namespace codex::scene

#endif // CODEX_CORE_CAMERA_H
