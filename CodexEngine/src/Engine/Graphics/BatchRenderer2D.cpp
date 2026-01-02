#include "BatchRenderer2D.h"

#include <Engine/Core/Application.h>
#include <Engine/Core/Public/ResourceHandler.h>
#include <Engine/Debug/Public/Debug.h>
#include <Engine/Scene/Public/Components.inl>
#include <Platform/OpenGL/GraphicsCapabilities.h>

namespace codex::gfx {
    namespace stdfs = std::filesystem;

    i32                      BatchRenderer2D::s_Capacity             = BATCH_RENDERER_INITIAL_CAPACITY;
    i32                      BatchRenderer2D::s_MaxQuadCountPerBatch = BATCH_RENDERER_MAX_QUAD_COUNT_PER_BATCH;
    Shader*                  BatchRenderer2D::s_QuadShader           = nullptr;
    const scene::Camera*     BatchRenderer2D::s_CurrentCamera        = nullptr;
    Matrix4f                 BatchRenderer2D::s_CurrentCameraViewMat;
    Vector3f                 BatchRenderer2D::s_CurrentCameraPos;
    std::vector<RenderBatch> BatchRenderer2D::s_Batches;

    Shader* BatchRenderer2D::GetShader() noexcept
    {
        return s_QuadShader;
    }

    usize BatchRenderer2D::GeBatchCount() noexcept
    {
        return s_Batches.size();
    }

    usize BatchRenderer2D::GetQuadCount() noexcept
    {
        usize quad_count = 0;
        for (const auto& b : s_Batches)
            quad_count += b.GetCount();
        return quad_count;
    }

    std::vector<RenderBatch>& BatchRenderer2D::GetBatches() noexcept
    {
        return s_Batches;
    }

    void BatchRenderer2D::Init(stdfs::path shaderPath)
    {
        if (!s_QuadShader)
        {
            s_QuadShader = new Shader(std::move(shaderPath));
            // TODO: Retrieve the maximum active texture slot count from the GPU instead of hard coding.
            s_QuadShader->CompileShader(
                { { "CX_MAX_SLOT_COUNT", std::to_string(opengl::capabilities::GetMaxTextureSlotCount()) } });

            s_Batches.reserve(s_Capacity);
            for (i32 i = 0; i < s_Capacity; ++i)
                s_Batches.emplace_back(s_MaxQuadCountPerBatch, s_QuadShader);

            for (auto& batch : s_Batches)
                batch.BindShader(s_QuadShader);
        }
    }

    void BatchRenderer2D::Dispose()
    {
        if (s_QuadShader)
        {
            delete s_QuadShader;

            s_QuadShader    = nullptr;
            s_CurrentCamera = nullptr;
        }
    }

    void BatchRenderer2D::Begin(const scene::Camera& camera, const TransformComponent& transform)
    {
        s_CurrentCamera        = &camera;
        s_CurrentCameraViewMat = glm::inverse(transform.ToMatrix());
        s_CurrentCameraPos     = transform.position;
        std::for_each(s_Batches.begin(), s_Batches.end(),
                      [](auto& b)
                      {
                          if (b.GetCount() > 0)
                              b.Flush();
                      });
    }

    void BatchRenderer2D::Begin(const scene::EditorCamera& camera)
    {
        s_CurrentCamera        = &camera;
        s_CurrentCameraViewMat = camera.GetViewMatrix();
        s_CurrentCameraPos     = camera.GetPos();
        std::for_each(s_Batches.begin(), s_Batches.end(),
                      [](auto& b)
                      {
                          if (b.GetCount() > 0)
                              b.Flush();
                      });
    }

    void BatchRenderer2D::End()
    {
        // std::vector<RenderBatch*> sorted_batch = s_Batches;
        // std::sort(sorted_batch.begin(), sorted_batch.end(), std::less{});

        s_QuadShader->Bind();
        s_QuadShader->SetUniformMat4f("u_View", s_CurrentCameraViewMat);
        s_QuadShader->SetUniformMat4f("u_Proj", s_CurrentCamera->GetProjectionMatrix());
        s_QuadShader->Unbind();

        {
            CX_DEBUG_PROFILE_SCOPE("BatchRender")

            std::for_each(s_Batches.begin(), s_Batches.end(),
                          [](auto& b)
                          {
                              if (b.GetCount() > 0)
                                  b.Render();
                          });
        }

        s_CurrentCamera = nullptr;
    }

    void BatchRenderer2D::RenderRect(Texture2D* texture, const Rectf& srcRect, const Matrix4f& transform,
                                     const Vector4f& colour, const i32 zIndex, const i32 entityId)
    {
        // Check if we're in the camera viewport.
        const Vector2f translation = transform[3];
        const Vector2  size{ glm::length(glm::vec3(transform[0])), glm::length(glm::vec3(transform[1])) };
        // Add ten extra pixels so fix the bug where there's a slight gap between the camera edge and the last
        // sprite inside the viewport of the camera.
        const auto camera_dim = Vector3{ s_CurrentCamera->GetWidth() * s_CurrentCamera->GetPan(),
                                         s_CurrentCamera->GetHeight() * s_CurrentCamera->GetPan(), 0 } +
                                100;
        const auto current_cam_pos = s_CurrentCameraPos - Vector3f{ camera_dim / 2 };

        if (translation.x < current_cam_pos.x + camera_dim.x && translation.x + size.x > current_cam_pos.x &&
            translation.y < current_cam_pos.y + camera_dim.y && translation.y + size.y > current_cam_pos.y)
        {
            for (auto& batch : s_Batches)
            {
                if (batch.HasRoom() && batch.GetZIndex() == zIndex)
                {
                    if (batch.UploadQuad(texture, srcRect, transform, colour, entityId))
                        return;
                    else
                        std::cerr << "failed to upload quad\n";
                }
            }

            // If there was no space then create a new batch
            auto new_batch = RenderBatch{ s_MaxQuadCountPerBatch, s_QuadShader };
            new_batch.SetZIndex(zIndex);
            new_batch.Flush();
            new_batch.UploadQuad(texture, srcRect, transform, colour, entityId);
            s_Batches.emplace_back(std::move(new_batch));
            std::sort(s_Batches.begin(), s_Batches.end(),
                      [](RenderBatch& a, RenderBatch& b) { return a.GetZIndex() < b.GetZIndex(); });
        }
    }
} // namespace codex::gfx
