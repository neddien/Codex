#include "DebugDraw.h"

#include <Engine/Core/Application.h>
#include <Engine/Core/Public/Geomtryd.h>
#include <Engine/Core/Window.h>
#include <Engine/Scene/Public/Scene.h>

namespace codex::gfx {
    namespace stdfs = std::filesystem;

    Shader* DebugDraw::s_Shader = nullptr;

    void DebugDraw::Init(stdfs::path shaderPath)
    {
        if (!s_Shader)
        {
            s_Shader = new Shader(std::move(shaderPath));
            s_Shader->CompileShader();
        }
    }

    void DebugDraw::Dispose() noexcept
    {
        if (s_Shader)
        {
            delete s_Shader;
            s_Shader = nullptr;
        }
    }

    DebugDraw::DebugDraw()
    {
        if (!s_Shader)
            cx_throw(InvalidOperationException, "Tried creating a DebugDraw object before calling DebugDraw::Init().");

        m_Vao = mem::Box<opengl::VertexArray>::New();
        m_Vao->Bind();

        m_Vbo = mem::Box<opengl::VertexBuffer>::New();
        m_Vbo->Bind();
        m_Vbo->SetBuffer<f32>(nullptr, m_Verticies.size() * sizeof(f32), opengl::BufferUsage::DYNAMIC_DRAW);

        m_Layout = mem::Box<opengl::VertexBufferLayout>::New();
        m_Layout->Push<f32>(3); // a_Pos
        m_Layout->Push<f32>(4); // a_Colour

        m_Vao->AddBuffer(m_Vbo.Get(), m_Layout.Get());

        // Thicc and smooth but not cury lines.
        // GL_Call(glLineWidth(2.5f));
        GL_Call(glEnable(GL_LINE_SMOOTH));

        m_Lines.reserve(LINE2D_MAX_COUNT);
    }

    void DebugDraw::Begin(const scene::EditorCamera& camera)
    {
        m_CurrentCamera           = &camera;
        m_CurrentCameraViewMatrix = camera.GetViewMatrix();

        for (usize i = 0; i < m_Lines.size(); ++i)
        {
            if (m_Lines[i].BeginFrame() < 0)
                m_Lines.erase(m_Lines.begin() + i--);
        }
    }

    void DebugDraw::Begin(const scene::Camera& camera, const TransformComponent& transform)
    {
        m_CurrentCamera           = &camera;
        m_CurrentCameraViewMatrix = glm::inverse(transform.ToMatrix());
        if (!s_Shader)
            return;

        for (usize i = 0; i < m_Lines.size(); ++i)
        {
            if (m_Lines[i].BeginFrame() < 0)
                m_Lines.erase(m_Lines.begin() + i--);
        }
    }

    void DebugDraw::End()
    {
        if (!s_Shader)
            return;

        if (m_Lines.size() <= 0)
            return;

        auto count = 0;
        for (const auto& line : m_Lines)
        {
            const auto&    colour = line.GetColour();
            const Vector2f pos[]  = { line.GetSource(), line.GetDestination() };
            for (auto i = 0; i < LINE2D_VERTEX_COMPONENT_COUNT * LINE2D_VERTEX_COUNT;
                 i += LINE2D_VERTEX_COMPONENT_COUNT)
            {
                m_Verticies[count + i]     = pos[i / LINE2D_VERTEX_COMPONENT_COUNT].x;
                m_Verticies[count + i + 1] = pos[i / LINE2D_VERTEX_COMPONENT_COUNT].y;
                m_Verticies[count + i + 2] = 0.0f;
                m_Verticies[count + i + 3] = colour.x;
                m_Verticies[count + i + 4] = colour.y;
                m_Verticies[count + i + 5] = colour.z;
                m_Verticies[count + i + 6] = colour.w;
            }
            count += LINE2D_VERTEX_COMPONENT_COUNT * LINE2D_VERTEX_COUNT;
        }

        s_Shader->Bind();
        s_Shader->SetUniformMat4f("u_View", m_CurrentCameraViewMatrix);
        s_Shader->SetUniformMat4f("u_Proj", m_CurrentCamera->GetProjectionMatrix());

        m_Vao->Bind();
        m_Vbo->Bind();
        m_Vbo->SetBufferSubData<f32>(m_Verticies.data(), 0, m_Verticies.size() * sizeof(f32));

        GL_Call(glDrawArrays(GL_LINES, 0, m_Lines.size() * 2));

        s_Shader->Unbind();
        m_Vao->Unbind();
        m_Vbo->Unbind();
    }

    void DebugDraw::DrawLine2D(const Vector2f source, const Vector2f destination, const Vector4f colour,
                               const i32 lifeTime)
    {
        if (m_Lines.size() < LINE2D_MAX_COUNT)
            m_Lines.emplace_back(source, destination, colour, lifeTime);
    }

    void DebugDraw::DrawRect2D(const Rectf rect, const f32 angle, const Vector4f colour, const i32 lifeTime)
    {
        if (m_Lines.size() + 4 < LINE2D_MAX_COUNT)
        {
            const auto min = Vector2f{ rect.x - rect.w / 2.0f, rect.y - rect.h / 2.0f };
            const auto max = Vector2f{ rect.x + rect.w / 2.0f, rect.y + rect.h / 2.0f };

            Vector2f lines[] = { { min.x, min.y }, { min.x, max.y }, { max.x, max.y }, { max.x, min.y } };

            if (angle != 0.0f)
            {
                for (auto& e : lines)
                {
                    const auto origin = Vector2f{ rect.x, rect.y };
                    e                 = glm::rotate((e - origin), math::ToRadf(angle)) + origin;
                }
            }

            for (usize i = 1; i < ArrayLength(lines); ++i)
                m_Lines.emplace_back(lines[i - 1], lines[i], colour, lifeTime);
            m_Lines.emplace_back(lines[3], lines[0], colour, lifeTime);
        }
    }

    void DebugDraw::DrawCircle2D(const Vector2f centrePos, const i32 radius, const f32 angle, const i32 segments,
                                 const Vector4f colour, const i32 lifeTime)
    {
        if (m_Lines.size() + segments < LINE2D_MAX_COUNT)
        {
            const auto segment_angle   = 360.0f / segments;
            auto       current_segment = glm::rotate(Vector2f{ 0.0f, radius }, -angle);
            for (auto i = 0; i < segments; ++i)
            {
                const auto src  = current_segment;
                current_segment = glm::rotate(current_segment, math::ToRadf(segment_angle));
                m_Lines.emplace_back(centrePos + src, centrePos + current_segment, colour, lifeTime);
            }

            m_Lines.emplace_back(centrePos, centrePos + glm::rotate(Vector2f{ radius, 0.0f }, math::ToRadf(angle)),
                                 colour, lifeTime);
        }
    }
} // namespace codex::gfx
