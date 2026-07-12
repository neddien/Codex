#include "batch_renderer2d.h"

#include <engine/core/engine.h>
#include <engine/debug/public/debug.h>
#include <engine/scene/public/components.inl>
#include <platform/open_gl/graphics_capabilities.h>

namespace codex::gfx {
    i32                      BatchRenderer2D::s_capacity_                 = BatchRenderer2D::INITIAL_CAPACITY;
    i32                      BatchRenderer2D::s_max_quad_count_per_batch_ = BatchRenderer2D::MAX_QUAD_COUNT_PER_BATCH;
    Shader*                  BatchRenderer2D::s_quad_shader_              = nullptr;
    const scene::Camera*     BatchRenderer2D::s_current_camera_           = nullptr;
    mat4                     BatchRenderer2D::s_current_camera_view_mat_;
    vec3                     BatchRenderer2D::s_current_camera_pos_;
    std::vector<RenderBatch> BatchRenderer2D::s_batches_;

    Shader* BatchRenderer2D::shader() noexcept
    {
        return s_quad_shader_;
    }

    usize BatchRenderer2D::batch_count() noexcept
    {
        return s_batches_.size();
    }

    usize BatchRenderer2D::quad_count() noexcept
    {
        usize quad_count = 0;
        for (const auto& b : s_batches_)
            quad_count += b.count();
        return quad_count;
    }

    std::vector<RenderBatch>& BatchRenderer2D::batches() noexcept
    {
        return s_batches_;
    }

    void BatchRenderer2D::init(fs::VirtualFilesystem& vfs, const std::string_view path)
    {
        if (!s_quad_shader_) {
            auto fh = vfs.open(std::string{ path });
            if (!fh)
                throw NotFoundException("{}: no such file or directory", path);

            std::string source(fh->size(), '\0');
            fh->read(source.data(), fh->size());

            s_quad_shader_ = new Shader(std::move(source));
            // TODO: Retrieve the maximum active texture slot count from the GPU instead of hard coding.
            s_quad_shader_->compile_shader(
                { { "CX_MAX_SLOT_COUNT", std::to_string(opengl::capabilities::max_texture_slot_count()) } });

            s_batches_.reserve(s_capacity_);
            for (i32 i = 0; i < s_capacity_; ++i)
                s_batches_.emplace_back(s_max_quad_count_per_batch_, s_quad_shader_);

            for (auto& batch : s_batches_)
                batch.bind_shader(s_quad_shader_);
        }
    }

    void BatchRenderer2D::dispose()
    {
        if (s_quad_shader_) {
            delete s_quad_shader_;

            s_quad_shader_    = nullptr;
            s_current_camera_ = nullptr;
        }
    }

    void BatchRenderer2D::begin(const scene::Camera& camera, const TransformComponent& transform)
    {
        s_current_camera_          = &camera;
        s_current_camera_view_mat_ = glm::inverse(transform.world_mat());
        s_current_camera_pos_      = transform.position;
        std::for_each(s_batches_.begin(), s_batches_.end(),
                      [](auto& b)
                      {
                          if (b.count() > 0)
                              b.flush();
                      });
    }

    void BatchRenderer2D::begin(const scene::EditorCamera& camera)
    {
        s_current_camera_          = &camera;
        s_current_camera_view_mat_ = camera.view_matrix();
        s_current_camera_pos_      = camera.pos();
        std::for_each(s_batches_.begin(), s_batches_.end(),
                      [](auto& b)
                      {
                          if (b.count() > 0)
                              b.flush();
                      });
    }

    void BatchRenderer2D::end(gfx::Shader* custom_end_shader)
    {
        CX_DEBUG_PROFILE_SCOPE("BatchRenderer2D::end")
        // std::vector<RenderBatch*> sorted_batch = s_batches_;
        // std::sort(sorted_batch.begin(), sorted_batch.end(), std::less{});

        Shader* cur_shader = (custom_end_shader) ? custom_end_shader : s_quad_shader_;

        cur_shader->bind();
        cur_shader->set_uniform_mat4f("u_View", s_current_camera_view_mat_);
        cur_shader->set_uniform_mat4f("u_Proj", s_current_camera_->projection_matrix());
        cur_shader->unbind();

        std::for_each(s_batches_.begin(), s_batches_.end(),
                      [cur_shader](RenderBatch& b)
                      {
                          if (b.count() > 0) {
                              b.bind_shader(cur_shader);
                              b.render();
                          }
                      });

        s_current_camera_ = nullptr;
    }

    void BatchRenderer2D::render_rect(Texture2D* texture, const rect& src_rect, const mat4& transform,
                                      const vec4& colour, const i32 z_index, const i32 entity_id)
    {
        // Check if we're in the camera viewport.
        const vec2  translation = transform[3];
        const ivec2 size{ glm::length(glm::vec3(transform[0])), glm::length(glm::vec3(transform[1])) };
        // Add ten extra pixels so fix the bug where there's a slight gap between the camera edge and the last
        // sprite inside the viewport of the camera.
        const auto camera_dim = ivec3{ s_current_camera_->width() * s_current_camera_->pan(),
                                       s_current_camera_->height() * s_current_camera_->pan(), 0 } +
                                100;
        const auto current_cam_pos = s_current_camera_pos_ - vec3{ camera_dim / 2 };

        if (translation.x < current_cam_pos.x + camera_dim.x && translation.x + size.x > current_cam_pos.x &&
            translation.y < current_cam_pos.y + camera_dim.y && translation.y + size.y > current_cam_pos.y) {
            for (auto& batch : s_batches_) {
                if (batch.has_room() && batch.z_index() == z_index) {
                    if (batch.upload_quad(texture, src_rect, transform, colour, entity_id))
                        return;
                    else
                        std::cerr << "failed to upload quad\n";
                }
            }

            // If there was no space then create a new batch
            auto new_batch = RenderBatch{ s_max_quad_count_per_batch_, s_quad_shader_ };
            new_batch.set_z_index(z_index);
            new_batch.flush();
            new_batch.upload_quad(texture, src_rect, transform, colour, entity_id);
            s_batches_.emplace_back(std::move(new_batch));
            std::sort(s_batches_.begin(), s_batches_.end(),
                      [](RenderBatch& a, RenderBatch& b) { return a.z_index() < b.z_index(); });
        }
    }
} // namespace codex::gfx
