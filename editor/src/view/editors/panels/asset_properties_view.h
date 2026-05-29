#pragma once

#include <codex.h>

#include "editor_panel.h"

namespace codex::editor {
    class AssetPropertiesView : public EditorPanel, public Loggable<"AssetPropertiesView">
    {
    public:
        using EditorPanel::EditorPanel;

    protected:
        void on_init() override;
        void on_imgui_render() override;

    private:
        void render_texture2d_properties(AssetMetadata& meta);

    private:
        std::unordered_map<UUID, Asset<gfx::Texture2D>> texture_cache_;
    };
} // namespace codex::editor
