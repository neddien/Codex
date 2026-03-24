#pragma once

#include "editor_panel.h"

namespace codex::editor {
    class TilePalleteView : public EditorPanel
    {
    public:
        using EditorPanel::EditorPanel;

    private:
        Entity                        entity_ = Entity::none();
        scene::EditorCamera           camera_;
        Vector2f                      viewport_bounds_[2]{};
        Vector2f                      viewport_size_{};
        mem::Box<opengl::FrameBuffer> pallete_fb_ = nullptr;
        Scene                         pallete_scene_;
        Entity                        pallete_entity_   = Entity::none();
        bool                          viewport_focused_ = false;
        bool                          viewport_hovered_ = false;
        gfx::DebugDraw                debug_draw_;
        bool                          positioned_pallete_ = false;

    public:
        [[nodiscard]] inline Entity get_entity() const noexcept { return entity_; }
        void                        set_entity(Entity newEntity) noexcept;

    protected:
        void on_init() override;
        void on_update(const f32 deltaTime) override;
        void on_imgui_render() override;

    protected:
        // Events
        void on_event(events::Event& e) override;
        bool on_mouse_down_event(events::MouseDownEvent& e);
        bool on_mouse_move_event(events::MouseMoveEvent& e);
        bool on_mouse_scroll_event(events::MouseScrollEvent& e);
    };
} // namespace codex::editor
