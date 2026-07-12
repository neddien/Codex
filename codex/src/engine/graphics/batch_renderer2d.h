#pragma once

#include <engine/core/public/geometry.h>
#include <engine/filesystem/vfs.h>
#include <engine/memory/public/memory.h>
#include <engine/scene/editor_camera.h>
#include <engine/scene/public/camera.h>
#include <engine/scene/public/sprite.h>

#include "public/shader.h"
#include "public/texture2d.h"
#include "render_batch.h"

namespace codex {
    // Forward decelerations
    struct TransformComponent;
} // namespace codex

namespace codex::gfx {
    class Renderer;

    class CODEX_API BatchRenderer2D
    {
        friend class Renderer;

    public:
        static constexpr auto INITIAL_CAPACITY         = 16;
        static constexpr auto MAX_QUAD_COUNT_PER_BATCH = 1024;

    private:
        BatchRenderer2D()                                            = default;
        BatchRenderer2D(const BatchRenderer2D& other)                = delete;
        BatchRenderer2D& operator=(const BatchRenderer2D& other)     = delete;
        BatchRenderer2D(BatchRenderer2D&& other) noexcept            = delete;
        BatchRenderer2D& operator=(BatchRenderer2D&& other) noexcept = delete;
        ~BatchRenderer2D()                                           = default;

    public:
        [[nodiscard]] static Shader*                   shader() noexcept; // raw ptr for RenderBatch internals
        [[nodiscard]] static usize                     batch_count() noexcept;
        [[nodiscard]] static usize                     quad_count() noexcept;
        [[nodiscard]] static std::vector<RenderBatch>& batches() noexcept;

    public:
        static void init(fs::VirtualFilesystem& vfs, std::string_view path);
        static void dispose();
        static void begin(const scene::Camera& camera, const TransformComponent& camera_transform);
        static void begin(const scene::EditorCamera& camera);
        static void end(gfx::Shader* custom_end_shader = nullptr);
        static void render_rect(Texture2D* texture, const rect& src_rect, const mat4& mat, const vec4& colour,
                                const i32 z_index = 0, const i32 entity_id = -1);

    public:
        static inline void render_sprite(const Sprite& sprite, const mat4& transform, const i32 entity_id = -1)
        {
            render_rect(sprite.texture().as_shared().get(), sprite.texture_coords(), transform, sprite.colour(),
                        sprite.z_index(), entity_id);
        }

    private:
        static i32                      s_capacity_;
        static i32                      s_max_quad_count_per_batch_;
        static Shader*                  s_quad_shader_;
        static const scene::Camera*     s_current_camera_;
        static mat4                     s_current_camera_view_mat_;
        static vec3                    s_current_camera_pos_;
        static std::vector<RenderBatch> s_batches_;
    };
} // namespace codex::gfx
