#pragma once

#include <sdafx.h>

#include <Engine/Scene/EditorCamera.h>
#include <Engine/Scene/Public/Camera.h>
#include <Engine/Scene/Public/Components.inl>
#include <Platform/OpenGL/VertexArray.h>
#include <Platform/OpenGL/VertexBuffer.h>
#include <Platform/OpenGL/VertexBufferLayout.h>

#include "Line2D.h"
#include "Public/Shader.h"

namespace codex::gfx {
    constexpr auto LINE2D_MAX_COUNT              = 5000; // Maximum amount of a line a single batch can contain.
    constexpr auto LINE2D_INDEX_COUNT            = 6;    // How many indices does a single vertex buffer have
    constexpr auto LINE2D_VERTEX_COMPONENT_COUNT = 7;    // How many components does a vertex have?
    constexpr auto LINE2D_VERTEX_COUNT           = 2;    // How many vertices does the buffer have?
    constexpr auto LINE2D_VERTEX_SIZE = LINE2D_VERTEX_COUNT * LINE2D_VERTEX_COMPONENT_COUNT; // The total count of the
                                                                                             // elements in the buffer

    class CODEX_API DebugDraw
    {
    private:
        mem::Box<opengl::VertexArray>                             m_Vao;
        mem::Box<opengl::VertexBuffer>                            m_Vbo;
        mem::Box<opengl::VertexBufferLayout>                      m_Layout;
        std::vector<Line2D>                                    m_Lines;
        std::array<f32, LINE2D_MAX_COUNT * LINE2D_VERTEX_SIZE> m_Verticies;
        const scene::Camera*                                   m_CurrentCamera;
        Matrix4f                                               m_CurrentCameraViewMatrix;

    private:
        static Shader* s_Shader;

    public:
        DebugDraw();

    public:
        void Begin(const scene::Camera& camera, const TransformComponent& transform);
        void Begin(const scene::EditorCamera& camera);
        void End();

        void DrawLine2D(const Vector2f source, const Vector2f destination,
                        const Vector4f colour = { 0.0f, 1.0f, 0.0f, 1.0f }, const i32 lifeTime = 1);
        void DrawRect2D(const Rectf rect, const f32 angle = 0.0f, const Vector4f colour = { 0.0f, 1.0f, 0.0f, 1.0f },
                        const i32 lifeTime = 1);
        void DrawCircle2D(const Vector2f centrePos, const i32 radius = 50, const f32 angle = 0.0f,
                          const i32 segments = 20, const Vector4f colour = { 0.0f, 1.0f, 0.0f, 1.0f },
                          i32 lifeTime = 1);

    public:
        static void Init(std::filesystem::path shaderPath);
        static void Dispose() noexcept;
    };
} // namespace codex::gfx
