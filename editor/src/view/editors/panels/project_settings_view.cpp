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
                auto& physics_props = d->active_scene.lock()->physics_properties();

                // Tick rate.
                {
                    ImGui::Columns(2);
                    ImGui::SetColumnWidth(0, 300);
                    ImGui::Text("Tick Rate");
                    ImGui::NextColumn();
                    ImGui::SliderInt("###tick_rate", &physics_props.tick_rate, 0, 120);
                    ImGui::Columns(1);
                }

                // Velocity iterations.
                {
                    ImGui::Columns(2);
                    ImGui::SetColumnWidth(0, 300);
                    ImGui::Text("Velocity iterations");
                    ImGui::NextColumn();
                    ImGui::SliderInt("###vel_iter", reinterpret_cast<i32*>(&physics_props.velocity_iterations), 0, 120);
                    ImGui::Columns(1);
                }

                // Position iterations.
                {
                    ImGui::Columns(2);
                    ImGui::SetColumnWidth(0, 300);
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
                    ImGui::SetColumnWidth(0, 300);
                    ImGui::Text("Scaling factor");
                    ImGui::NextColumn();
                    ImGui::DragFloat("###scale_conv", &scaling_factor);
                    ImGui::Columns(1);

                    physics_props.scaling_factor = 1.0f / scaling_factor;
                }

                // Gravity.
                {
                    SceneEditorView::draw_vec2_control("Gravity", physics_props.gravity, d->column_width);
                }
            }
        }

        ImGui::End();
    }
} // namespace codex::editor
