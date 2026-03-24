#pragma once

#include <codex.h>

#include "editor_panel.h"

namespace codex::editor {
    class ToolbarView : public EditorPanel
    {
    public:
        using EditorPanel::EditorPanel;

    private:

    protected:
        void on_init() override;
        void on_imgui_render() override;
    };
} // namespace codex::editor
