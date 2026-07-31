#include "project_settings_view.h"

#include "../scene_editor_view.h"

namespace codex::editor {
    void ProjectSettingsView::on_init()
    {
    }

    void ProjectSettingsView::on_imgui_render()
    {
        ImGui::Begin("Project settings");

        auto d = this->get_descriptor().lock();

        // Physics settings.
        {
            if (ImGui::CollapsingHeader("Physics")) {
                auto&           physics_props = d->active_scene.lock()->physics_properties();
                constexpr float label_width   = 300.0f;

                // Tick rate.
                {
                    ImGui::Columns(2);
                    ImGui::SetColumnWidth(0, label_width);
                    ImGui::Text("Tick Rate");
                    ImGui::NextColumn();
                    ImGui::SliderInt("###tick_rate", &physics_props.tick_rate, 0, 120);
                    ImGui::Columns(1);
                }

                // Velocity iterations.
                {
                    ImGui::Columns(2);
                    ImGui::SetColumnWidth(0, label_width);
                    ImGui::Text("Velocity iterations");
                    ImGui::NextColumn();
                    ImGui::SliderInt("###vel_iter", reinterpret_cast<i32*>(&physics_props.velocity_iterations), 0, 120);
                    ImGui::Columns(1);
                }

                // Position iterations.
                {
                    ImGui::Columns(2);
                    ImGui::SetColumnWidth(0, label_width);
                    ImGui::Text("Position iterations");
                    ImGui::NextColumn();
                    ImGui::SliderInt("###pos_iter", reinterpret_cast<i32*>(&physics_props.position_iterations), 0, 120);
                    ImGui::Columns(1);
                }

                // Scaling factor.
                {
                    static f32 scaling_factor;

                    scaling_factor = 1.0f / physics_props.scaling_factor;

                    ImGui::Columns(2);
                    ImGui::SetColumnWidth(0, label_width);
                    ImGui::Text("Scaling factor");
                    ImGui::NextColumn();
                    ImGui::DragFloat("###scale_conv", &scaling_factor);
                    ImGui::Columns(1);

                    physics_props.scaling_factor = 1.0f / scaling_factor;
                }

                // Gravity.
                {
                    SceneEditorView::draw_vec2_control("Gravity", physics_props.gravity, label_width);
                }

                // Collision layer names. Editor-only authoring labels for the 16 physics
                // bits; used for the layer/mask grid tooltips in the rigidbody inspector.
                {
                    ImGui::Dummy(ImVec2(0.0f, 6.0f));
                    if (ImGui::TreeNodeEx("Collision Layers", ImGuiTreeNodeFlags_DefaultOpen)) {
                        auto& names = d->project.physics_layer_names;
                        for (int i = 0; i < static_cast<int>(names.size()); ++i) {
                            ImGui::Columns(2);
                            ImGui::SetColumnWidth(0, label_width);
                            ImGui::Text("Layer %d", i + 1);
                            ImGui::NextColumn();
                            const auto id = fmt::format("###phys_layer_{}", i);
                            ImGui::InputTextWithHint(id.c_str(), fmt::format("Layer {}", i + 1).c_str(), &names[i]);
                            ImGui::Columns(1);
                        }
                        ImGui::TreePop();
                    }
                }
            }
        }

        ImGui::End();
    }
} // namespace codex::editor
