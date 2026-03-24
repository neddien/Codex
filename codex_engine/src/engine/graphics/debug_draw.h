#pragma once

#include <engine/scene/editor_camera.h>
#include <engine/scene/public/camera.h>
#include <engine/scene/public/components.inl>
#include <platform/open_gl/vertex_array.h>
#include <platform/open_gl/vertex_buffer.h>
#include <platform/open_gl/vertex_buffer_layout.h>

#include "line2d.h"
#include "public/shader.h"

namespace codex::gfx {
    constexpr auto LINE2D_MAX_COUNT              = 5000; // Maximum amount of a line a single batch can contain.
    constexpr auto LINE2D_INDEX_COUNT            = 6;    // How many indices does a single vertex buffer have
    constexpr auto LINE2D_VERTEX_COMPONENT_COUNT = 7;    // How many components does a vertex have?
    constexpr auto LINE2D_VERTEX_COUNT           = 2;    // How many vertices does the buffer have?
    constexpr auto LINE2D_VERTEX_SIZE = LINE2D_VERTEX_COUNT * LINE2D_VERTEX_COMPONENT_COUNT; // The total count of the
                                                                                             // elements in the buffer

    class CODEX_API DebugDraw
    {
    public:
        DebugDraw(const DebugDraw&)                = delete;
        DebugDraw& operator=(const DebugDraw&)     = delete;
        DebugDraw(DebugDraw&&) noexcept            = default;
        DebugDraw& operator=(DebugDraw&&) noexcept = default;

    public:
        DebugDraw();

    public:
        void begin(const scene::Camera& camera, const TransformComponent& transform);
        void begin(const scene::EditorCamera& camera);
        void end();

        void draw_line_2d(const Vector2f source, const Vector2f destination,
                          const Vector4f colour = { 0.0f, 1.0f, 0.0f, 1.0f }, const i32 lifeTime = 1);
        void draw_rect_2d(const Rectf rect, const f32 angle = 0.0f, const Vector4f colour = { 0.0f, 1.0f, 0.0f, 1.0f },
                          const i32 lifeTime = 1);
        void draw_circle_2d(const Vector2f centrePos, const i32 radius = 50, const f32 angle = 0.0f,
                            const i32 segments = 20, const Vector4f colour = { 0.0f, 1.0f, 0.0f, 1.0f },
                            i32 lifeTime = 1);

    public:
        static void init(std::filesystem::path shaderPath);
        static void dispose() noexcept;

    private:
        mem::Box<opengl::VertexArray>                          vao_;
        mem::Box<opengl::VertexBuffer>                         vbo_;
        mem::Box<opengl::VertexBufferLayout>                   layout_;
        std::vector<Line2D>                                    lines_;
        std::array<f32, LINE2D_MAX_COUNT * LINE2D_VERTEX_SIZE> verticies_;
        const scene::Camera*                                   current_camera_;
        Matrix4f                                               current_camera_view_matrix_;
        static Shader*                                         s_shader_;
    };
} // namespace codex::gfx
