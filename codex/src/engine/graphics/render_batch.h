#pragma once

#include <engine/core/public/geometry.h>
#include <platform/open_gl/index_buffer.h>
#include <platform/open_gl/vertex_array.h>
#include <platform/open_gl/vertex_buffer.h>
#include <platform/open_gl/vertex_buffer_layout.h>

#include "public/shader.h"
#include "public/texture2d.h"

namespace codex::gfx {
    constexpr auto QUAD2D_VERTEX_COUNT = 4; // How many vertices does the buffer have?

    CX_PACKED(struct QuadVertex {
        mat4 model;
        vec4 vertex;
        vec4 colour;
        vec2 tex_coord;
        vec2 tex_size;
        i32  tex_id;
        i32  entity_id;
    });

    class CODEX_API RenderBatch
    {
    public:
        RenderBatch() = default;
        RenderBatch(const i32 max_quad_count, Shader* shader);
        RenderBatch(RenderBatch&& other) noexcept;
        RenderBatch& operator=(RenderBatch&& other) noexcept;
        ~RenderBatch();

    public:
        [[nodiscard]] inline bool        has_room() const { return has_room_; }
        [[nodiscard]] inline i32         z_index() const { return z_index_; }
        [[nodiscard]] inline i32         count() const { return quad_count_; }
        [[nodiscard]] inline QuadVertex* quads() noexcept { return vertices_; }

    public:
        inline void set_z_index(i32 new_index) { z_index_ = new_index; }
        inline void bind_shader(Shader* shader) { shader_ = shader; }
        void        swap(RenderBatch& other) noexcept
        {
            std::swap(quad_count_, other.quad_count_);
            std::swap(max_texture_slot_count_, other.max_texture_slot_count_);
            std::swap(max_quad_count_, other.max_quad_count_);
            std::swap(z_index_, other.z_index_);
            std::swap(has_room_, other.has_room_);
            std::swap(vertices_, other.vertices_);
            std::swap(vertex_ptr_, other.vertex_ptr_);
            std::swap(vao_, other.vao_);
            std::swap(vbo_, other.vbo_);
            std::swap(ebo_, other.ebo_);
            std::swap(layout_, other.layout_);
            std::swap(shader_, other.shader_);
            std::swap(texture_list_, other.texture_list_);
            std::swap(current_tex_index_, other.current_tex_index_);
        }
        void flush();
        bool upload_quad(Texture2D* texture, const rect& src_rect, const mat4& transform, const vec4& colour,
                         const i32 entity_id);
        std::vector<u32> generate_indices(u32& size);
        void             render();

    private:
        i32                                         quad_count_             = 0;
        i32                                         max_texture_slot_count_ = 32;
        i32                                         max_quad_count_         = 1000;
        i32                                         z_index_                = 0;
        bool                                        has_room_               = true;
        QuadVertex*                                 vertices_               = nullptr;
        QuadVertex*                                 vertex_ptr_             = nullptr;
        std::unique_ptr<opengl::VertexArray>        vao_                    = nullptr;
        std::unique_ptr<opengl::VertexBuffer>       vbo_                    = nullptr;
        std::unique_ptr<opengl::IndexBuffer>        ebo_                    = nullptr;
        std::unique_ptr<opengl::VertexBufferLayout> layout_                 = nullptr;
        Shader*                                     shader_                 = nullptr;
        std::vector<Texture2D*>                     texture_list_;
        u16                                         current_tex_index_ = 0;
    };
} // namespace codex::gfx
