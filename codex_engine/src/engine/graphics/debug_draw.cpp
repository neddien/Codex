#include "debug_draw.h"

#include <engine/core/engine.h>
#include <engine/core/public/geometry.h>
#include <engine/core/window.h>
#include <engine/scene/public/scene.h>

namespace codex::gfx {
    namespace stdfs = std::filesystem;

    Shader* DebugDraw::s_shader_ = nullptr;

    void DebugDraw::init(stdfs::path shaderPath)
    {
        if (!s_shader_) {
            s_shader_ = new Shader(std::move(shaderPath));
            s_shader_->compile_shader();
        }
    }

    void DebugDraw::dispose() noexcept
    {
        if (s_shader_) {
            delete s_shader_;
            s_shader_ = nullptr;
        }
    }

    DebugDraw::DebugDraw()
    {
        if (!s_shader_)
            throw InvalidOperationException("Tried creating a DebugDraw object before calling DebugDraw::init().");

        vao_ = Box<opengl::VertexArray>::make();
        vao_->bind();

        vbo_ = Box<opengl::VertexBuffer>::make();
        vbo_->bind();
        vbo_->set_buffer<f32>(nullptr, verticies_.size() * sizeof(f32), opengl::BufferUsage::DYNAMIC_DRAW);

        layout_ = Box<opengl::VertexBufferLayout>::make();
        layout_->push<f32>(3); // a_Pos
        layout_->push<f32>(4); // a_Colour

        vao_->add_buffer(vbo_.get(), layout_.get());

        // Thicc and smooth but not cury lines.
        // GL_Call(glLineWidth(2.5f));
        GL_Call(glEnable(GL_LINE_SMOOTH));

        lines_.reserve(LINE2D_MAX_COUNT);
    }

    void DebugDraw::begin(const scene::EditorCamera& camera)
    {
        current_camera_             = &camera;
        current_camera_view_matrix_ = camera.view_matrix();

        for (usize i = 0; i < lines_.size(); ++i) {
            if (lines_[i].begin_frame() < 0)
                lines_.erase(lines_.begin() + i--);
        }
    }

    void DebugDraw::begin(const scene::Camera& camera, const TransformComponent& transform)
    {
        current_camera_             = &camera;
        current_camera_view_matrix_ = glm::inverse(transform.to_matrix());
        if (!s_shader_)
            return;

        for (usize i = 0; i < lines_.size(); ++i) {
            if (lines_[i].begin_frame() < 0)
                lines_.erase(lines_.begin() + i--);
        }
    }

    void DebugDraw::end()
    {
        if (!s_shader_)
            return;

        if (lines_.size() <= 0)
            return;

        auto count = 0;
        for (const auto& line : lines_) {
            const auto&    colour = line.colour();
            const Vector2f pos[]  = { line.source(), line.destination() };
            for (auto i = 0; i < LINE2D_VERTEX_COMPONENT_COUNT * LINE2D_VERTEX_COUNT;
                 i += LINE2D_VERTEX_COMPONENT_COUNT) {
                verticies_[count + i]     = pos[i / LINE2D_VERTEX_COMPONENT_COUNT].x;
                verticies_[count + i + 1] = pos[i / LINE2D_VERTEX_COMPONENT_COUNT].y;
                verticies_[count + i + 2] = 0.0f;
                verticies_[count + i + 3] = colour.x;
                verticies_[count + i + 4] = colour.y;
                verticies_[count + i + 5] = colour.z;
                verticies_[count + i + 6] = colour.w;
            }
            count += LINE2D_VERTEX_COMPONENT_COUNT * LINE2D_VERTEX_COUNT;
        }

        s_shader_->bind();
        s_shader_->set_uniform_mat4f("u_View", current_camera_view_matrix_);
        s_shader_->set_uniform_mat4f("u_Proj", current_camera_->projection_matrix());

        vao_->bind();
        vbo_->bind();
        vbo_->set_buffer_sub_data<f32>(verticies_.data(), 0, verticies_.size() * sizeof(f32));

        GL_Call(glDrawArrays(GL_LINES, 0, lines_.size() * 2));

        s_shader_->unbind();
        vao_->unbind();
        vbo_->unbind();
    }

    void DebugDraw::draw_line_2d(const Vector2f source, const Vector2f destination, const Vector4f colour,
                                 const i32 lifeTime)
    {
        if (lines_.size() < LINE2D_MAX_COUNT)
            lines_.emplace_back(source, destination, colour, lifeTime);
    }

    void DebugDraw::draw_rect_2d(const Rectf rect, const f32 angle, const Vector4f colour, const i32 lifeTime)
    {
        if (lines_.size() + 4 < LINE2D_MAX_COUNT) {
            const auto min = Vector2f{ rect.x - rect.w / 2.0f, rect.y - rect.h / 2.0f };
            const auto max = Vector2f{ rect.x + rect.w / 2.0f, rect.y + rect.h / 2.0f };

            Vector2f lines[] = { { min.x, min.y }, { min.x, max.y }, { max.x, max.y }, { max.x, min.y } };

            if (angle != 0.0f) {
                for (auto& e : lines) {
                    const auto origin = Vector2f{ rect.x, rect.y };
                    e                 = glm::rotate((e - origin), math::to_radf(angle)) + origin;
                }
            }

            for (usize i = 1; i < array_length(lines); ++i)
                lines_.emplace_back(lines[i - 1], lines[i], colour, lifeTime);
            lines_.emplace_back(lines[3], lines[0], colour, lifeTime);
        }
    }

    void DebugDraw::draw_circle_2d(const Vector2f centrePos, const i32 radius, const f32 angle, const i32 segments,
                                   const Vector4f colour, const i32 lifeTime)
    {
        if (lines_.size() + segments < LINE2D_MAX_COUNT) {
            const auto segment_angle   = 360.0f / segments;
            auto       current_segment = glm::rotate(Vector2f{ 0.0f, radius }, -angle);
            for (auto i = 0; i < segments; ++i) {
                const auto src  = current_segment;
                current_segment = glm::rotate(current_segment, math::to_radf(segment_angle));
                lines_.emplace_back(centrePos + src, centrePos + current_segment, colour, lifeTime);
            }

            lines_.emplace_back(centrePos, centrePos + glm::rotate(Vector2f{ radius, 0.0f }, math::to_radf(angle)),
                                colour, lifeTime);
        }
    }
} // namespace codex::gfx
