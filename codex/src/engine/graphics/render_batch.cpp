#include "render_batch.h"

#include <platform/open_gl/graphics_capabilities.h>

namespace codex::gfx {
    RenderBatch::RenderBatch(const i32 max_quad_count, Shader* shader)
        : max_quad_count_(max_quad_count)
        , shader_(shader)
    {
        max_texture_slot_count_ = opengl::capabilities::max_texture_slot_count();

        vertices_   = new QuadVertex[max_quad_count * QUAD2D_VERTEX_COUNT];
        vertex_ptr_ = vertices_;
        texture_list_.resize(max_texture_slot_count_);

        vao_ = std::make_unique<opengl::VertexArray>();
        vao_->bind();

        vbo_ = std::make_unique<opengl::VertexBuffer>();
        vbo_->bind();
        vbo_->set_buffer<QuadVertex>(nullptr, sizeof(QuadVertex) * QUAD2D_VERTEX_COUNT * max_quad_count_,
                                     opengl::BufferUsage::DYNAMIC_DRAW); // you basically allocate space and
                                                                         // then upload the data

        ebo_ = std::make_unique<opengl::IndexBuffer>();
        ebo_->bind();
        u32  size              = 0;
        auto index_buffer_data = generate_indices(size);
        ebo_->set_buffer(index_buffer_data.data(), size);

        layout_ = std::make_unique<opengl::VertexBufferLayout>();
        layout_->push<f32>(4); // a_Vertex
        layout_->push<f32>(4); // a_Colour
        layout_->push<f32>(3); // a_Centre
        layout_->push<f32>(2); // a_TexCoord
        layout_->push<f32>(2); // a_TexDim
        layout_->push<i32>(1); // a_TexId
        layout_->push<i32>(1); // a_EntityId

        vao_->add_buffer(vbo_.get(), layout_.get());

        // For some weird reason this does not work.
        /*i32 textures[] = {0, 1, 2, 3, 4, 5, 6, 7};
        shader_->set_uniform_1i_arr("u_Textures", 8, textures);*/

        quad_count_        = 0;
        has_room_          = true;
        current_tex_index_ = 0;
        std::fill(texture_list_.begin(), texture_list_.end(), nullptr);
    }

    RenderBatch::RenderBatch(RenderBatch&& other) noexcept
        : quad_count_(std::move(other.quad_count_))
        , max_texture_slot_count_(std::move(other.max_texture_slot_count_))
        , max_quad_count_(std::move(other.max_quad_count_))
        , z_index_(std::move(other.z_index_))
        , has_room_(std::move(other.has_room_))
        , vertices_(std::move(other.vertices_))
        , vertex_ptr_(std::move(other.vertex_ptr_))
        , vao_(std::move(other.vao_))
        , vbo_(std::move(other.vbo_))
        , ebo_(std::move(other.ebo_))
        , layout_(std::move(other.layout_))
        , shader_(std::move(other.shader_))
        , texture_list_(std::move(other.texture_list_))
        , current_tex_index_(std::move(other.current_tex_index_))
    {
        other.quad_count_        = 0;
        other.z_index_           = 0;
        other.has_room_          = true;
        other.vertices_          = nullptr;
        other.vertex_ptr_        = nullptr;
        other.current_tex_index_ = 0;
    }

    RenderBatch& RenderBatch::operator=(RenderBatch&& other) noexcept
    {
        RenderBatch{ std::move(other) }.swap(*this);
        return *this;
    }

    RenderBatch::~RenderBatch()
    {
        if (vertices_) {
            delete[] vertices_;
            vertices_ = nullptr;
        }
    }

    void RenderBatch::flush()
    {
        quad_count_        = 0;
        has_room_          = true;
        current_tex_index_ = 0;
        vertex_ptr_        = vertices_;
        std::fill(texture_list_.begin(), texture_list_.end(), nullptr);
    }

    bool RenderBatch::upload_quad(Texture2D* texture, const rect& src_rect, const mat4& transform, const vec4& colour,
                                  const i32 entity_id)
    {
        u16 tex_id     = 0;
        f32 tex_width  = 0.0f;
        f32 tex_height = 0.0f;
        if (texture) {
            const auto it = std::find(texture_list_.begin(), texture_list_.end(), texture);
            if (it == texture_list_.end()) {
                if (current_tex_index_ < max_texture_slot_count_) {
                    texture_list_[current_tex_index_++] = texture;
                    tex_id                              = current_tex_index_;
                } else
                    return false;
            } else
                tex_id = (u16)std::distance(texture_list_.begin(), it) + 1;
            tex_width  = (f32)texture->width();
            tex_height = (f32)texture->height();
        }

        vec4 quad_verticies[4] = { { 0.5f, 0.5f, 0.0f, 1.0f },
                                   { -0.5f, 0.5f, 0.0f, 1.0f },
                                   { -0.5f, -0.5f, 0.0f, 1.0f },
                                   { 0.5f, -0.5f, 0.0f, 1.0f } };
        vec2 tex_coords[4]     = { { src_rect.x + src_rect.w, src_rect.y + src_rect.h },
                                   { src_rect.x, src_rect.y + src_rect.h },
                                   { src_rect.x, src_rect.y },
                                   { src_rect.x + src_rect.w, src_rect.y } };

        for (usize i = 0; i < QUAD2D_VERTEX_COUNT; ++i) {
            vertex_ptr_->vertex    = transform * quad_verticies[i];
            vertex_ptr_->colour    = colour;
            vertex_ptr_->centre    = transform[3];
            vertex_ptr_->tex_coord = tex_coords[i];
            vertex_ptr_->tex_id    = tex_id;
            vertex_ptr_->tex_size  = { tex_width, tex_height };
            vertex_ptr_->entity_id = entity_id;
            ++vertex_ptr_;
        }

        if (++quad_count_ >= max_quad_count_)
            has_room_ = false;

        return true;
    }

    std::vector<u32> RenderBatch::generate_indices(u32& size)
    {
        size = 6 * max_quad_count_;
        std::vector<u32> index_buffer_data(size);

        for (usize i = 0; i < size; i += 6) {
            u32 offset = (i32)(4 * (i / 6));

            // TODO: Consider using std::memcpy here.
            index_buffer_data[i]     = 0 + offset;
            index_buffer_data[i + 1] = 1 + offset;
            index_buffer_data[i + 2] = 2 + offset;

            index_buffer_data[i + 3] = 2 + offset;
            index_buffer_data[i + 4] = 3 + offset;
            index_buffer_data[i + 5] = 0 + offset;
        }

        return index_buffer_data;
    }

    void RenderBatch::render()
    {
        shader_->bind();
        vao_->bind();
        ebo_->bind();
        vbo_->bind();
        vbo_->set_buffer_sub_data<QuadVertex>(vertices_, 0, sizeof(QuadVertex) * QUAD2D_VERTEX_COUNT * quad_count_);

        for (i32 i = 0; i < current_tex_index_; ++i)
            texture_list_[i]->bind(i);

        static bool texture_slots_initialized = false;
        if (!texture_slots_initialized) {
            std::vector<i32> textures(max_texture_slot_count_);
            for (u32 i = 0; i < (u32)textures.size(); ++i)
                textures[i] = i;

            shader_->set_uniform_1i_arr("u_Textures", max_texture_slot_count_, textures.data());
            texture_slots_initialized = false;
        }

        GL_Call(glDrawElements(GL_TRIANGLES, 6 * quad_count_, GL_UNSIGNED_INT, nullptr));

        shader_->unbind();
        vao_->unbind();
        vbo_->unbind();
        ebo_->unbind();

        for (i32 i = 0; i < current_tex_index_; ++i)
            texture_list_[i]->unbind();
    }
} // namespace codex::gfx
