#include "asset_properties_view.h"

namespace codex::editor {
    void AssetPropertiesView::on_init()
    {
    }

    void AssetPropertiesView::on_imgui_render()
    {
        auto d = get_descriptor().lock();

        assert(d);

        ImGui::Begin("Asset Properties");

        if (d->registry_state == AssetRegistryState::Succeeded) {
            // TODO: .asset_registry() call on each frame is quite expesnive, just check if UUID is valid or not.
            AssetMetadata* selected_asset = AssetManager::registry().asset_metadata(d->selected_asset);
            if (selected_asset) {
                if (selected_asset->type == gfx::Texture2D::ktype_name())
                    render_texture2d_properties(*selected_asset);
            }
        }

        ImGui::End();
    }

    void AssetPropertiesView::render_texture2d_properties(AssetMetadata& meta)
    {
        auto d = get_descriptor().lock();
        assert(d);

        Asset<gfx::Texture2D> texture_asset;

        if (auto it = texture_cache_.find(meta.path.uuid()); it != texture_cache_.end()) {
            texture_asset = it->second;
        } else {
            texture_asset = AssetManager::load<gfx::Texture2D>(meta.path);
            if (texture_asset)
                texture_cache_.try_emplace(meta.path.uuid(), texture_asset);
        }

        // Texture prewview image
        {
            ImGui::Columns(2);
            ImGui::SetColumnWidth(0, d->column_width);
            ImGui::Text("Texture: ");
            ImGui::NextColumn();

            ImGui::BeginGroup();
            if (texture_asset) {
                //  TODO: Have like a default no-texture-loaded image.
                ImGui::Image(reinterpret_cast<ImTextureID>(texture_asset->gl_id()), { 100.0f, 100.0f }, { 0, 1 },
                             { 1, 0 });
            }

            static char tex_path_buf[256] = {};
            ImGui::SetNextItemWidth(100.0f);
            ImGui::InputText("##tex_path", tex_path_buf, sizeof(tex_path_buf));
            ImGui::SameLine();
            if (ImGui::Button("Load")) {
                // TODO: Check if AssetManager::load<T> returned a valid object, or maybe make load throw an
                // exception?

                cxassert(meta.import_settings, "Import settings isn't valid for asset");

                AssetManager::registry().repath(meta.path, tex_path_buf);
                AssetManager::reimport<gfx::Texture2D>(meta.path, *meta.import_settings);
            }
            ImGui::EndGroup();
            ImGui::Columns(1);
            ImGui::Dummy(ImVec2(0.0f, 10.0f));
        }

        // Texture filter mode
        {
            ImGui::Dummy(ImVec2(0.0f, 10.0f));
            ImGui::Columns(2);
            ImGui::SetColumnWidth(0, d->column_width);
            ImGui::Text("Texture filter mode: ");
            ImGui::NextColumn();
            const char*             preview_item = nullptr;
            const gfx::TextureProperties props =
                static_cast<const gfx::Texture2D::ImportSettings*>(meta.import_settings.get())->props;
            switch (props.filter_mode) {
                case gfx::TextureFilterMode::Linear: preview_item = "Linear"; break;
                case gfx::TextureFilterMode::Nearest: preview_item = "Nearest"; break;
            }
            if (ImGui::BeginCombo("##texture_filter_mode", preview_item)) {
                if (ImGui::Selectable("Nearest", props.filter_mode == gfx::TextureFilterMode::Nearest)) {
                    if (props.filter_mode != gfx::TextureFilterMode::Nearest) {
                        auto new_props        = props;
                        new_props.filter_mode = gfx::TextureFilterMode::Nearest;

                        // TODO: Proper re-import ?
                        auto path = texture_asset.path();
                        AssetManager::reimport<gfx::Texture2D>(path.uuid(),
                                                               gfx::Texture2D::ImportSettings{ new_props });
                    }
                }

                if (ImGui::Selectable("Linear", props.filter_mode == gfx::TextureFilterMode::Linear)) {
                    if (props.filter_mode != gfx::TextureFilterMode::Linear) {
                        auto new_props        = props;
                        new_props.filter_mode = gfx::TextureFilterMode::Linear;

                        // TODO: Proper re-import ?
                        auto path = texture_asset.path();
                        AssetManager::reimport<gfx::Texture2D>(path.uuid(),
                                                               gfx::Texture2D::ImportSettings{ new_props });
                    }
                }
                ImGui::EndCombo();
            }
            ImGui::Columns(1);
        }

        // Chroma key
        {
            ImGui::Dummy(ImVec2(0.0f, 10.0f));
            ImGui::Columns(2);
            ImGui::SetColumnWidth(0, d->column_width);
            ImGui::Text("Enable chroma key:");
            ImGui::NextColumn();

            const gfx::TextureProperties props =
                static_cast<const gfx::Texture2D::ImportSettings*>(meta.import_settings.get())->props;

            bool enabled = props.chroma_key.has_value();
            if (ImGui::Checkbox("###asset:txt2d:chrm_enbl", &enabled)) {
                auto new_props = props;
                new_props.chroma_key =
                    enabled ? opt<vec3>{ props.chroma_key.value_or(vec3{ 255.0f, 0.0f, 255.0f }) } : std::nullopt;

                auto path = texture_asset.path();
                AssetManager::reimport<gfx::Texture2D>(path.uuid(), gfx::Texture2D::ImportSettings{ new_props });
            }
            ImGui::Columns(1);

            if (props.chroma_key) {
                // Key colour
                ImGui::Columns(2);
                ImGui::SetColumnWidth(0, d->column_width);
                ImGui::Text("Key colour:");
                ImGui::NextColumn();

                if (!chroma_colour_active_)
                    chroma_colour_edit_ = *props.chroma_key;

                f32                 colour[3] = { chroma_colour_edit_.x / 255.0f, chroma_colour_edit_.y / 255.0f,
                                                   chroma_colour_edit_.z / 255.0f };
                ImGuiColorEditFlags flags     = 0;
                flags |= ImGuiColorEditFlags_DisplayRGB;
                ImGui::ColorPicker3("##asset:txt2d:chrm_clr", colour, flags);
                chroma_colour_edit_   = vec3{ colour[0] * 255.0f, colour[1] * 255.0f, colour[2] * 255.0f };
                chroma_colour_active_ = ImGui::IsItemActive();
                if (ImGui::IsItemDeactivatedAfterEdit()) {
                    auto new_props       = props;
                    new_props.chroma_key = chroma_colour_edit_;

                    auto path = texture_asset.path();
                    AssetManager::reimport<gfx::Texture2D>(path.uuid(), gfx::Texture2D::ImportSettings{ new_props });
                }
                ImGui::Columns(1);

                // Inner tolerance
                ImGui::Columns(2);
                ImGui::SetColumnWidth(0, d->column_width);
                ImGui::Text("Inner tolerance:");
                ImGui::NextColumn();

                if (!chroma_inner_active_)
                    chroma_inner_edit_ = props.chroma_inner_tolerance;

                ImGui::DragFloat("##asset:txt2d:chrm_itol", &chroma_inner_edit_, 1.0f, 0.0f,
                                 props.chroma_outer_tolerance);
                chroma_inner_active_ = ImGui::IsItemActive();
                if (ImGui::IsItemDeactivatedAfterEdit()) {
                    auto new_props                   = props;
                    new_props.chroma_inner_tolerance = chroma_inner_edit_;

                    auto path = texture_asset.path();
                    AssetManager::reimport<gfx::Texture2D>(path.uuid(), gfx::Texture2D::ImportSettings{ new_props });
                }
                ImGui::Columns(1);

                // Outer tolerance
                ImGui::Columns(2);
                ImGui::SetColumnWidth(0, d->column_width);
                ImGui::Text("Outer tolerance:");
                ImGui::NextColumn();

                if (!chroma_outer_active_)
                    chroma_outer_edit_ = props.chroma_outer_tolerance;

                ImGui::DragFloat("##asset:txt2d:chrm_otol", &chroma_outer_edit_, 1.0f, props.chroma_inner_tolerance,
                                 442.0f);
                chroma_outer_active_ = ImGui::IsItemActive();
                if (ImGui::IsItemDeactivatedAfterEdit()) {
                    auto new_props                   = props;
                    new_props.chroma_outer_tolerance = chroma_outer_edit_;

                    auto path = texture_asset.path();
                    AssetManager::reimport<gfx::Texture2D>(path.uuid(), gfx::Texture2D::ImportSettings{ new_props });
                }
                ImGui::Columns(1);
            }
        }
    }
} // namespace codex::editor
