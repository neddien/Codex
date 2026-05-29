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
        static void render_physics_material_2d_control(phys::PhysicsMaterial2D& mat, const f32 columnWidth) noexcept;
        static bool render_asset_path_box(const char* label, std::string_view accepted_type, AssetPath& path) noexcept;
    };
} // namespace codex::editor
