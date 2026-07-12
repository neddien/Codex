#pragma once

#include <engine/core/public/archive.h>

namespace codex::scene {
    class CODEX_API Camera : public ISerializable
    {
    public:
        enum class ProjectionType
        {
            Orthographic,
            Perspective
        };

    public:
        Camera(const i32 width = 1280, const i32 height = 720, const f32 near_clip = 0.0f, const f32 far_clip = 100.0f,
               const ProjectionType projection_type = ProjectionType::Orthographic, const f32 fov = 90.0f) noexcept
            : width_(width)
            , height_(height)
            , fov_(fov)
            , near_clip_(near_clip)
            , far_clip_(far_clip)
            , projection_type_(projection_type)
            , projection_(mat4(1.0f))
            , pan_(1.0f)
        {
            update_projection_matrix();
        }

    public:
        [[nodiscard]] inline auto projection_matrix() const noexcept { return projection_; }
        [[nodiscard]] inline auto width() const noexcept { return width_; }
        [[nodiscard]] inline auto height() const noexcept { return height_; }
        [[nodiscard]] inline auto projection_type() const noexcept { return projection_type_; }
        [[nodiscard]] inline auto fov() const noexcept { return fov_; }
        [[nodiscard]] inline auto near_clip() const noexcept { return near_clip_; }
        [[nodiscard]] inline auto far_clip() const noexcept { return far_clip_; }
        [[nodiscard]] inline auto pan() const noexcept { return pan_; }

        inline void set_projection_type(const ProjectionType new_projection_type) noexcept
        {
            projection_type_ = new_projection_type;
            update_projection_matrix();
        }
        inline void set_fov(const f32 new_fov) noexcept
        {
            fov_ = new_fov;
            update_projection_matrix();
        }
        inline void set_near_clip(const f32 new_near_clip) noexcept
        {
            near_clip_ = new_near_clip;
            update_projection_matrix();
        }
        inline void set_far_clip(const f32 new_far_clip) noexcept
        {
            far_clip_ = new_far_clip;
            update_projection_matrix();
        }
        inline void set_pan(const f32 new_pan) noexcept
        {
            pan_ = new_pan;
            update_projection_matrix();
        }
        inline void set_width(const i32 new_width) noexcept
        {
            width_ = new_width;
            update_projection_matrix();
        }
        inline void set_height(const i32 new_height) noexcept
        {
            height_ = new_height;
            update_projection_matrix();
        }

    public:
        inline void update_projection_matrix() noexcept
        {
            if (projection_type_ == ProjectionType::Orthographic)
                projection_ =
                    glm::ortho((f32)width_ * pan_ / -2.0f, (f32)width_ * pan_ / 2.0f, (f32)height_ * pan_ / -2.0f,
                               (f32)height_ * pan_ / 2.0f, near_clip_, far_clip_);
            else
                projection_ = glm::perspective(fov_, (f32)width_ / (f32)height_, near_clip_, far_clip_);
        }

    public:
        [[nodiscard]] static vec3 screen_coordinates_to_world(const Camera& camera, const vec2& screen_coord,
                                                               const vec3& camera_position) noexcept;

    public:
        void archive(Archive& archive) override;

    private:
        i32            width_;
        i32            height_;
        f32            fov_;
        f32            near_clip_;
        f32            far_clip_;
        ProjectionType projection_type_;
        mat4           projection_;
        f32            pan_;
    };
} // namespace codex::scene
