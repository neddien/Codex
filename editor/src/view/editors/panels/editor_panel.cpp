#include "editor_panel.h"

#include "../scene_editor_view.h"

namespace codex::editor {
    EditorPanel::EditorPanel(SceneEditorView& view) noexcept
        : editor_view_(view)
        , desc_(editor_view_.descriptor_)
        , show_(true)
    {
    }

    EditorPanel::~EditorPanel() noexcept
    {
    }

    void EditorPanel::on_pre_update(const f32 deltaTime)
    {
        on_update(deltaTime);

        // Update our panels.
        for (auto& panel : view_panels_) {
            panel->on_pre_update(deltaTime);
        }
    }

    void EditorPanel::on_pre_imgui_render()
    {
        on_imgui_render();

        auto block_events = imgui_block_events();

        // Render our panels.
        {
            auto it = view_panels_.begin();
            while (it != view_panels_.end()) {
                auto& panel = *it;
                if (!panel->show_)
                    it = view_panels_.erase(it);
                else {
                    block_events = block_events && panel->imgui_block_events();
                    panel->on_pre_imgui_render();
                    ++it;
                }
            }
        }

        set_imgui_block_events(block_events);
    }

    void EditorPanel::on_pre_event(events::Event& e)
    {
        // Child panels should be handled first because of painter's rule.
        for (auto& panel : view_panels_) {
            if (e.handled)
                return;

            panel->on_pre_event(e);
        }

        on_event(e);
    }
} // namespace codex::editor
