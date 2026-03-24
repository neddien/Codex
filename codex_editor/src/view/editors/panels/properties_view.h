#pragma once

#include <codex.h>

#include "editor_panel.h"

namespace codex::editor {
    class PropertiesView : public EditorPanel
    {
    public:
        using EditorPanel::EditorPanel;

    protected:
        void on_init() override;
        void on_imgui_render() override;

    private:
        static void draw_physics_material_2d_control(phys::PhysicsMaterial2D& mat, const f32 columnWidth) noexcept;
    };
} // namespace codex::editor
