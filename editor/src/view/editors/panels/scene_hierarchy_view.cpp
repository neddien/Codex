#include "scene_hierarchy_view.h"

#include <codex.h>

#include "../scene_editor_view.h"

namespace codex::editor {
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
            static auto action_delete = false;
            static auto action_rename = false;
            auto        entities      = scene->get_all_entities_with_component<TagComponent>();
            for (auto& e : entities) {
                auto& tag_component = e.get_component<TagComponent>();
                if (ImGui::Selectable((tag_component.tag + "##entity").c_str(), d->selected_entity.entity == e,
                                      ImGuiSelectableFlags_DontClosePopups)) {
                    d->selected_entity.select(e);
                }

                if (ImGui::BeginPopupContextItem()) {
                    d->selected_entity.select(e);

                    if (ImGui::MenuItem("Rename"))
                        action_rename = true;
                    if (ImGui::MenuItem("Delete"))
                        action_delete = true;

                    ImGui::EndPopup();
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
