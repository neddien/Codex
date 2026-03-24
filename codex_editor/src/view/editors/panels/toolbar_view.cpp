#include "toolbar_view.h"

#include <editor.h>
#include <view/editors/scene_editor_view.h>

#include <editor_application.h>

#include "IconsTabler.h"

namespace codex::editor {
    void ToolbarView::on_init()
    {
    }

    void ToolbarView::on_imgui_render()
    {
        auto       d         = this->get_descriptor().lock();
        const auto scene     = d->active_scene.lock();
        const bool compiling = (d->compilation_state.load() == CompilationState::Compiling);

        ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(5, 5));
        ImGui::PushStyleVar(ImGuiStyleVar_ItemInnerSpacing, ImVec2(0, 0));

        ImGuiWindowClass toolbar_win_class;
        ImGui::SetNextWindowClass(&toolbar_win_class);
        ImGui::Begin("##toolbar", nullptr,
                     ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoScrollWithMouse |
                         ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoResize);

        if (compiling || scene->state() == Scene::State::Simulate)
            ImGui::BeginDisabled();

        const auto  size      = ImGui::GetWindowHeight() - 10.0f;
        const char* play_icon = (scene->state() == Scene::State::Play) ? ICON_TI_PLAYER_STOP : ICON_TI_PLAYER_PLAY;

        ImGui::PushFont(Editor::get_large_icon_font());
        if (ImGui::Button(play_icon, ImVec2(size, size))) {
            if (scene->state() == Scene::State::Edit) {
                this->get_parent().on_scene_play();
            } else if (scene->state() == Scene::State::Play) {
                this->get_parent().on_scene_stop();
            }
        }
        ImGui::PopFont();

        if (compiling || scene->state() == Scene::State::Simulate)
            ImGui::EndDisabled();

        ImGui::SameLine();

        if (compiling || scene->state() == Scene::State::Play)
            ImGui::BeginDisabled();

        const char* simulate_icon = (scene->state() == Scene::State::Simulate) ? ICON_TI_PLAYER_STOP : ICON_TI_SETTINGS;

        ImGui::PushFont(Editor::get_large_icon_font());
        if (ImGui::Button(simulate_icon, ImVec2(size, size))) {
            if (scene->state() == Scene::State::Edit) {
                this->get_parent().on_scene_simulate();
            } else if (scene->state() == Scene::State::Simulate) {
                this->get_parent().on_scene_stop();
            }
        }
        ImGui::PopFont();

        if (compiling || scene->state() == Scene::State::Play)
            ImGui::EndDisabled();

        ImGui::PopStyleVar(2);
        ImGui::End();
    }
} // namespace codex::editor
