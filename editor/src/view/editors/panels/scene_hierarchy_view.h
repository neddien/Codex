#pragma once

#include <codex.h>

#include "editor_panel.h"

namespace codex::editor {
    class SceneHierarchyView : public EditorPanel, private Loggable<"SceneHierarchyView">
    {
    public:
        using EditorPanel::EditorPanel;

    protected:
        void on_init() override;
        void on_imgui_render() override;
    };
} // namespace codex::editor
