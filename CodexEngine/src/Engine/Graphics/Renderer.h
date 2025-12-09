#ifndef CODEX_RENDERER_H
#define CODEX_RENDERER_H

#include <sdafx.h>

#include <Engine/Core/Public/Geomtryd.h>
#include <Engine/Scene/Public/Sprite.h>
#include <Engine/Graphics/BatchRenderer2D.h>
#include <Engine/Graphics/Line2D.h>
#include <Engine/Graphics/DebugDraw.h>

#include "Public/Shader.h"

namespace codex::gfx {
    // TODO: This is bad... fix it please.
    class CODEX_API Renderer
    {
    private:
        static i32            m_Width;
        static i32            m_Height;
        static mgl::Renderer* m_InternalRenderer;

    public:
        Renderer() = default;

    public:
        static void Init(const i32 width, const i32 height);
        static void Dispose() noexcept;

    public:
        static void Clear();
        static void SetClearColour(const f32 r, const f32 g, const f32 b, const f32 a);
        static void Render(const mgl::VertexArray* vertexArray, const mgl::IndexBuffer* indexBuffer,
                           const mgl::Shader* shader);
    };
} // namespace codex::gfx

#endif // CODEX_RENDERER_H
