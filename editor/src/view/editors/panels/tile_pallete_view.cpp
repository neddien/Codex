#include "tile_pallete_view.h"

#include "editor_application.h"

#include "icons_tabler.h"

namespace codex::editor {
    void TilePalleteView::set_entity(Entity newEntity) noexcept
    {
        if (newEntity != entity_) {
            entity_             = std::move(newEntity);
            positioned_pallete_ = false;
            camera_             = scene::EditorCamera{ 1280, 720 };
            if (entity_.has_component<TilemapComponent>()) {
                if (!pallete_entity_) {
                    pallete_entity_ = pallete_scene_.create_entity();
                    pallete_entity_.add_component<SpriteRendererComponent>();
                    pallete_entity_.add_component<GridRendererComponent>();
                }
            }
        }
    }

    void TilePalleteView::on_init()
    {
        camera_ = scene::EditorCamera{ 1280, 720 };

        opengl::FrameBufferProperties props;
        props.width       = 1280;
        props.height      = 720;
        props.attachments = { { .format = opengl::TextureFormat::RGBA8 } };

        // FIXME: Fix opengl::FrameBuffer's copy and move operators.
        pallete_fb_ = Box<opengl::FrameBuffer>::make(props);
    }

    void TilePalleteView::on_update(const f32 deltaTime)
    {
        pallete_fb_->bind();

        // Viewport resize
        {
            camera_.set_width(viewport_size_.x);
            camera_.set_height(viewport_size_.y);
            pallete_fb_->resize((u32)viewport_size_.x, (u32)viewport_size_.y);
        }

        gfx::Renderer::clear();

        pallete_scene_.on_editor_update(deltaTime, camera_);

        debug_draw_.begin(camera_);

        if (entity_.has_component<TilemapComponent>()) {
            auto& entity_tmc      = entity_.get_component<TilemapComponent>();
            auto& pallete_grc     = pallete_entity_.get_component<GridRendererComponent>();
            auto& pallete_src     = pallete_entity_.get_component<SpriteRendererComponent>();
            pallete_src.sprite()  = entity_tmc.sprite;
            pallete_grc.cell_size = entity_tmc.tile_size;

            SceneEditorView::render_grid(debug_draw_, camera_, pallete_grc);

            if (!positioned_pallete_) {
                auto pos = camera_.pos() - Vector3f{ pallete_src.sprite().size() / 2.0f, 0.0f };
                pos      = util::snap(pos, Vector3f{ entity_tmc.tile_size, 1.0f });
                pos += Vector3f{ pallete_src.sprite().size() / 2.0f, 0.0f };
                pallete_entity_.get_component<TransformComponent>().position = pos;
                positioned_pallete_                                          = true;
            }
        } else {
            entity_             = Entity::none();
            positioned_pallete_ = false;
        }

        debug_draw_.end();

        pallete_fb_->unbind();
    }

    void TilePalleteView::on_imgui_render()
    {
        static auto size = ImVec2{ 32, 32 };

        auto d = get_parent().get_descriptor().lock();

        if (focus_) {
            ImGui::SetNextWindowFocus();
            focus_ = false;
        }

        if (ImGui::Begin("Tile pallete", &show_)) {
            if (ImGui::Button(ICON_TI_BRUSH, size)) {
                if (entity_) {
                    auto& c         = entity_.get_component<TilemapComponent>();
                    c.current_state = TilemapComponent::State::Brush;
                    d->selected_entity.select(entity_);
                }
            }

            ImGui::SameLine();

            if (ImGui::Button(ICON_TI_ERASER, size)) {
                if (entity_) {
                    auto& c         = entity_.get_component<TilemapComponent>();
                    c.current_state = TilemapComponent::State::Erase;
                    d->selected_entity.select(entity_);
                }
            }

            if (entity_) {
                auto& c = entity_.get_component<TilemapComponent>();
                if (c.sprite) {
                    const auto viewport_min_region = ImGui::GetWindowContentRegionMin();
                    const auto viewport_max_region = ImGui::GetWindowContentRegionMax();
                    const auto viewport_offset     = ImGui::GetWindowPos();
                    viewport_bounds_[0]            = { viewport_min_region.x + viewport_offset.x,
                                                       viewport_min_region.y + viewport_offset.y };
                    viewport_bounds_[1]            = { viewport_max_region.x + viewport_offset.x,
                                                       viewport_max_region.y + viewport_offset.y };

                    auto current_viewport_window_size = ImGui::GetContentRegionAvail();
                    viewport_size_ = Vector2f{ current_viewport_window_size.x, current_viewport_window_size.y };
                    ImGui::Image((ImTextureID)(pallete_fb_->colour_attachment_id_at(0)), current_viewport_window_size,
                                 { 0, 1 }, { 1, 0 });

                    viewport_focused_ = ImGui::IsWindowFocused();
                    viewport_hovered_ = ImGui::IsWindowHovered();

                    set_imgui_block_events(!viewport_focused_);
                }
            }

            if (!show_)
                this->close();
        }
        ImGui::End();
    }

    void TilePalleteView::on_event(events::Event& e)
    {
        events::EventDispatcher d{ e };
        d.dispatch<events::MouseDownEvent>(bind_event_delegate(this, &TilePalleteView::on_mouse_down_event));
        d.dispatch<events::MouseMoveEvent>(bind_event_delegate(this, &TilePalleteView::on_mouse_move_event));
        d.dispatch<events::MouseScrollEvent>(bind_event_delegate(this, &TilePalleteView::on_mouse_scroll_event));
    }

    bool TilePalleteView::on_mouse_down_event(events::MouseDownEvent& e)
    {
        auto mouse_pos = Vector2{ ImGui::GetMousePos().x, ImGui::GetMousePos().y };
        mouse_pos.x -= viewport_bounds_[0].x;
        mouse_pos.y -= viewport_bounds_[0].y;
        mouse_pos.y = (viewport_bounds_[1] - viewport_bounds_[0]).y - mouse_pos.y;

        if (entity_ && mouse_pos.x >= 0 && mouse_pos.y >= 0 && mouse_pos.x <= viewport_size_.x &&
            mouse_pos.y <= viewport_size_.y) {
            auto& c = entity_.get_component<TilemapComponent>();

            const auto mouse_in_scene =
                util::snap(scene::Camera::screen_coordinates_to_world(camera_, mouse_pos, camera_.pos()),
                           Vector3f{ c.tile_size, 1.0f });
            const auto pallete_entity_size = pallete_entity_.get_component<SpriteRendererComponent>().sprite().size();
            const auto pallete_entity_top_left = pallete_entity_.get_component<TransformComponent>().position -
                                                 util::to_vec3f(pallete_entity_size / 2.0f);
            if (mouse_in_scene.x >= pallete_entity_top_left.x && mouse_in_scene.y >= pallete_entity_top_left.y &&
                mouse_in_scene.x <= pallete_entity_top_left.x + pallete_entity_size.x &&
                mouse_in_scene.y <= pallete_entity_top_left.y + pallete_entity_size.y) {
                c.current_tile = Vector2f{ mouse_in_scene } - Vector2f{ pallete_entity_top_left };
            }
            return true;
        }
        return false;
    }

    bool TilePalleteView::on_mouse_move_event(events::MouseMoveEvent& e)
    {
        auto mouse_pos = Vector2{ ImGui::GetMousePos().x, ImGui::GetMousePos().y };
        mouse_pos.x -= viewport_bounds_[0].x;
        mouse_pos.y -= viewport_bounds_[0].y;
        mouse_pos.y = (viewport_bounds_[1] - viewport_bounds_[0]).y - mouse_pos.y;
        if (mouse_pos.x >= 0 && mouse_pos.y >= 0 && mouse_pos.x <= viewport_size_.x &&
            mouse_pos.y <= viewport_size_.y) {
            if (Input::is_mouse_down(Mouse::MiddleMouse)) {
                if (Input::is_mouse_dragging()) {
                    const auto vec = Vector2f{ Input::mouse_delta_x(), Input::mouse_delta_y() * -1.0f };
                    camera_.set_pos(camera_.pos() + util::to_vec3f(vec) * camera_.pan());
                }
                return true;
            }
        }
        return false;
    }

    bool TilePalleteView::on_mouse_scroll_event(events::MouseScrollEvent& e)
    {
        auto mouse_pos = Vector2{ ImGui::GetMousePos().x, ImGui::GetMousePos().y };
        mouse_pos.x -= viewport_bounds_[0].x;
        mouse_pos.y -= viewport_bounds_[0].y;
        mouse_pos.y = (viewport_bounds_[1] - viewport_bounds_[0]).y - mouse_pos.y;
        if (mouse_pos.x >= 0 && mouse_pos.y >= 0 && mouse_pos.x <= viewport_size_.x &&
            mouse_pos.y <= viewport_size_.y) {
            camera_.set_pan(camera_.pan() + e.offset_y() * -0.05f);
            return true;
        }
        return false;
    }
} // namespace codex::editor
