#ifndef CODEX_RENDERER_SPRITE_BATCH_RENDERER_H
#define CODEX_RENDERER_SPRITE_BATCH_RENDERER_H

#include <sdafx.h>

#include <Engine/Core/Public/Geomtryd.h>
#include <Engine/Memory/Public/Memory.h>
#include <Engine/Scene/EditorCamera.h>
#include <Engine/Scene/Public/Camera.h>
#include <Engine/Scene/Public/Sprite.h>

#include "Public/Shader.h"
#include "Public/Texture2D.h"
#include "RenderBatch.h"

namespace codex {
    // Forward decelerations
    struct TransformComponent;
} // namespace codex

namespace codex::gfx {
    constexpr auto BATCH_RENDERER_INITIAL_CAPACITY         = 16;
    constexpr auto BATCH_RENDERER_MAX_QUAD_COUNT_PER_BATCH = 1024;

    // Forward decelerations
    class Renderer;

    class CODEX_API BatchRenderer2D
    {
        friend class Renderer;

    private:
        static i32                      s_Capacity;
        static i32                      s_MaxQuadCountPerBatch;
        static Shader*                  s_QuadShader;
        static const scene::Camera*     s_CurrentCamera;
        static Matrix4f                 s_CurrentCameraViewMat;
        static Vector3f                 s_CurrentCameraPos;
        static std::vector<RenderBatch> s_Batches;

    private:
        BatchRenderer2D()                                            = default;
        BatchRenderer2D(const BatchRenderer2D& other)                = delete;
        BatchRenderer2D& operator=(const BatchRenderer2D& other)     = delete;
        BatchRenderer2D(BatchRenderer2D&& other) noexcept            = delete;
        BatchRenderer2D& operator=(BatchRenderer2D&& other) noexcept = delete;
        ~BatchRenderer2D()                                           = default;

    public:
        static Shader*                   GetShader() noexcept;
        static usize                     GeBatchCount() noexcept;
        static usize                     GetQuadCount() noexcept;
        static std::vector<RenderBatch>& GetBatches() noexcept;

    public:
        static void Init(std::filesystem::path shaderPath);
        static void Dispose();
        static void Begin(const scene::Camera& camera, const TransformComponent& cameraTransform);
        static void Begin(const scene::EditorCamera& camera);
        static void End();
        static void RenderRect(Texture2D* texture, const Rectf& srcRect, const Matrix4f& mat, const Vector4f& colour,
                               const i32 zIndex = 0, const i32 entityId = -1);

    public:
        static inline void RenderSprite(const Sprite& sprite, const Matrix4f& transform, const i32 entityId = -1)
        {
            RenderRect(sprite.GetTexture().Get(), sprite.GetTextureCoords(), transform, sprite.GetColour(),
                       sprite.GetZIndex(), entityId);
        }
    };
} // namespace codex::gfx

#endif // CODEX_RENDERER_SPRITE_BATCH_RENDERER_H
