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

        // Deferred-commit edit buffers for the chroma key controls: only resynced from the
        // committed TextureProperties when the corresponding widget isn't actively being dragged,
        // otherwise the live in-progress edit gets stomped every frame by the not-yet-reimported value.
        vec3 chroma_colour_edit_    = {};
        bool chroma_colour_active_  = false;
        f32  chroma_inner_edit_     = 0.0f;
        bool chroma_inner_active_   = false;
        f32  chroma_outer_edit_     = 0.0f;
        bool chroma_outer_active_   = false;
    };
} // namespace codex::editor
