#pragma once

#include <codex.h>

#include <ImGuizmo.h>

#include "view/editors/scene_editor_view.h"

namespace codex::editor {
    class Editor : public Layer
    {
    private:
        mem::Box<SceneEditorView> scene_editor_view_ = nullptr;

    private:
        static std::optional<scene::EditorCamera> s_camera_;
        static ImFont*                            s_large_icon_font_;

    public:
        [[nodiscard]] static inline scene::EditorCamera& get_viewport_camera() noexcept { return *s_camera_; }
        [[nodiscard]] static inline ImFont*              get_large_icon_font() noexcept { return s_large_icon_font_; }

    public:
        void on_attach() override;
        void on_detach() override;
        void on_update(const f32 deltaTime) override;
        void on_imgui_render() override;

    public:
        void on_event(events::Event& e) override;
        bool on_key_down_event(events::KeyDownEvent& e);
    };
} // namespace codex::editor
