#include "scene_hierarchy_view.h"

#include <codex.h>
#include <engine/scene/public/prefab.h>
#include <nfd.h>

#include "../scene_editor_view.h"

namespace codex::editor {
    namespace stdfs = std::filesystem;

    void SceneHierarchyView::on_init()
    {
    }

    void SceneHierarchyView::on_imgui_render()
    {
        {
            auto d     = this->get_descriptor().lock();
            auto scene = d->active_scene.lock();

            ImGui::Begin("Scene hierarchy");
            if (ImGui::Button("New entity"))
                ImGui::OpenPopup("new_entity_popup");
            if (ImGui::BeginPopup("new_entity_popup")) {
                auto name = fmt::format("Entity {}", scene->entity_count() + 1);
                ImGui::Text("Entity Name");
                ImGui::SameLine();
                ImGui::InputText("##entity_name", &name);
                ImGui::SameLine();
                if (ImGui::IsKeyPressed(ImGuiKey_Escape) || ImGui::IsKeyPressed(ImGuiKey_Enter) ||
                    ImGui::Button("Add")) // Check for Enter key press
                {
                    // Close the popup
                    ImGui::CloseCurrentPopup();
                    d->selected_entity.select(scene->create_entity(name));
                }
                ImGui::EndPopup();
            }

            ImGui::Text("Entities");
            static auto action_delete        = false;
            static auto action_rename        = false;
            static auto action_create_prefab = false;

            // Hierarchy mutations are deferred until after the tree is drawn so we never
            // mutate a children list we are currently iterating.
            std::optional<std::pair<Entity, Entity>> pending_attach; // { new parent, child }
            std::optional<Entity>                    pending_detach;

            auto draw_entity_node = [&](auto&& self, Entity e) -> void {
                auto& tag_component = e.get_component<TagComponent>();
                auto& idc           = e.get_component<IDComponent>();
                auto* hc = e.has_component<HierarchyComponent>() ? &e.get_component<HierarchyComponent>() : nullptr;

                auto flags = ImGuiTreeNodeFlags_OpenOnArrow | ImGuiTreeNodeFlags_OpenOnDoubleClick |
                             ImGuiTreeNodeFlags_SpanAvailWidth;
                if (!hc || hc->children.empty())
                    flags |= ImGuiTreeNodeFlags_Leaf;
                if (d->selected_entity.entity == e)
                    flags |= ImGuiTreeNodeFlags_Selected;

                const bool open =
                    ImGui::TreeNodeEx((tag_component.tag + "##" + idc.uuid.to_string()).c_str(), flags);

                if (ImGui::IsItemClicked(ImGuiMouseButton_Left) && !ImGui::IsItemToggledOpen())
                    d->selected_entity.select(e);

                if (ImGui::BeginPopupContextItem()) {
                    d->selected_entity.select(e);

                    if (ImGui::MenuItem("Rename"))
                        action_rename = true;
                    if (ImGui::MenuItem("Create Prefab"))
                        action_create_prefab = true;
                    if (hc && hc->parent && ImGui::MenuItem("Detach"))
                        pending_detach = e;
                    if (ImGui::MenuItem("Delete"))
                        action_delete = true;

                    ImGui::EndPopup();
                }

                if (ImGui::BeginDragDropSource()) {
                    ImGui::SetDragDropPayload("CX_ENTITY", &e, sizeof(Entity));
                    ImGui::TextUnformatted(tag_component.tag.c_str());
                    ImGui::EndDragDropSource();
                }

                if (ImGui::BeginDragDropTarget()) {
                    if (const auto* payload = ImGui::AcceptDragDropPayload("CX_ENTITY"))
                        pending_attach = { { e, *static_cast<const Entity*>(payload->Data) } };
                    ImGui::EndDragDropTarget();
                }

                if (open) {
                    if (hc)
                        for (Entity c : hc->children)
                            self(self, c);
                    ImGui::TreePop();
                }
            };

            auto entities = scene->entities_with_component<TagComponent>();
            for (auto& e : entities) {
                // Children get drawn by their parents.
                if (e.has_component<HierarchyComponent>() && e.get_component<HierarchyComponent>().parent)
                    continue;
                draw_entity_node(draw_entity_node, e);
            }

            // Dropping onto the empty space below the tree detaches the entity from its parent.
            const auto avail = ImGui::GetContentRegionAvail();
            ImGui::Dummy({ std::max(avail.x, 1.0f), std::max(avail.y, 40.0f) });
            if (ImGui::BeginDragDropTarget()) {
                if (const auto* payload = ImGui::AcceptDragDropPayload("CX_ENTITY"))
                    pending_detach = *static_cast<const Entity*>(payload->Data);
                ImGui::EndDragDropTarget();
            }

            if (pending_attach && !scene->attach_parent(pending_attach->first, pending_attach->second))
                log(Warn, "Cannot parent '{}' to '{}'.",
                    pending_attach->second.get_component<TagComponent>().tag,
                    pending_attach->first.get_component<TagComponent>().tag);
            if (pending_detach && pending_detach->has_component<HierarchyComponent>())
                scene->detach_parent(pending_detach->get_component<HierarchyComponent>().parent, *pending_detach);

            if (action_create_prefab) {
                action_create_prefab = false;

                nfdu8char_t*      out_path    = nullptr;
                nfdu8filteritem_t filters[]   = { { "Codex Prefab", "cxprefab" } };
                const std::string assets_path = (d->current_project_path / "assets").generic_string();
                const auto result = NFD_SaveDialogU8(&out_path, filters, 1, assets_path.c_str(), "prefab.cxprefab");
                if (result == NFD_OKAY) {
                    stdfs::path save_path{ out_path };
                    NFD_FreePathU8(out_path);
                    if (save_path.extension() != ".cxprefab")
                        save_path.replace_extension(".cxprefab");

                    std::error_code ec;
                    const auto      assets_root = stdfs::weakly_canonical(d->current_project_path / "assets", ec);
                    const auto      canonical_save_path = stdfs::weakly_canonical(save_path, ec);
                    const auto      relative            = stdfs::relative(canonical_save_path, assets_root, ec);
                    if (ec || relative.empty() || relative.is_absolute() || *relative.begin() == "..") {
                        log(Error, "Prefabs must be saved inside the project's assets directory");
                    } else {
                        auto prefab = scene::Prefab::from_entity(d->selected_entity.entity);
                        if (!SerializationManager::save_to_file(prefab, canonical_save_path,
                                                                SerializationManager::Format::Binary)) {
                            log(Error, "Failed to save prefab to {}", canonical_save_path.generic_string());
                        } else {
                            AssetManager::registry().scan_async(AssetManager::root_dir()).await_sync();
                            ++d->asset_registry_revision;
                        }
                    }
                } else if (result == NFD_ERROR) {
                    log(Error, "Failed to open prefab save dialog: {}", NFD_GetError());
                }
            }

            if (action_rename)
                ImGui::OpenPopup("rename_popup");
            if (ImGui::BeginPopup("rename_popup")) {
                ImGui::Text("New Name");
                ImGui::SameLine();
                ImGui::InputText("##entity_name", &d->selected_entity.entity.get_component<TagComponent>().tag);
                ImGui::SameLine();
                if (ImGui::IsKeyPressed(ImGuiKey_Escape) || ImGui::IsKeyPressed(ImGuiKey_Enter) ||
                    ImGui::Button("Enter")) // Check for Enter key
                {
                    ImGui::CloseCurrentPopup();
                    action_rename = false;
                }
                ImGui::EndPopup();
            } else if (action_delete) {
                scene->remove_entity(d->selected_entity.entity);
                d->selected_entity.deselect();
                ImGui::CloseCurrentPopup();
                action_delete = false;
            }

            ImGui::End();
        }
    }
} // namespace codex::editor
