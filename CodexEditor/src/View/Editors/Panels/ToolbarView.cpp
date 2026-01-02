#include "ToolbarView.h"

#include "../SceneEditorView.h"

#include <EditorApplication.h>

namespace codex::editor {
    void ToolbarView::OnInit()
    {
        const auto cwd      = EditorApplication::GetAppDataPath();
        const auto tex_spec = opengl::TextureProperties{ .format     = opengl::TextureFormat::RGBA8,
                                                      .mipmapMode = opengl::TextureMipmapMode::LinearNearest };

        m_PlayIcon     = gfx::Texture2D(cwd / "Resources/play.png", tex_spec);
        m_StopIcon     = gfx::Texture2D(cwd / "Resources/stop.png", tex_spec);
        m_SimulateIcon = gfx::Texture2D(cwd / "Resources/gear.png", tex_spec);
    }

    void ToolbarView::OnImGuiRender()
    {
        auto       d     = this->GetDescriptor().Lock();
        const auto scene = d->activeScene.Lock();

        ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(5, 5));
        ImGui::PushStyleVar(ImGuiStyleVar_ItemInnerSpacing, ImVec2(0, 0));

        ImGuiWindowClass toolbar_win_class;
        // toolbar_win_class.DockNodeFlagsOverrideSet = ImGuiDockNodeFlags_NoTabBar;
        ImGui::SetNextWindowClass(&toolbar_win_class);
        ImGui::Begin("##toolbar", nullptr,
                     ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoScrollWithMouse |
                         ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoResize);

        if (scene->GetState() == Scene::State::Simulate)
            ImGui::BeginDisabled();

        const auto  size      = ImGui::GetWindowHeight() - 15.0f;
        const auto& play_icon = (scene->GetState() == Scene::State::Play) ? m_StopIcon : m_PlayIcon;
        if (ImGui::ImageButton("#play", reinterpret_cast<ImTextureID>(play_icon.GetGlId()), ImVec2(size, size)))
        {
            if (scene->GetState() == Scene::State::Edit)
            {
                this->GetParent().OnScenePlay();
            }
            else if (scene->GetState() == Scene::State::Play)
            {
                this->GetParent().OnSceneStop();
            }
        }

        if (scene->GetState() == Scene::State::Simulate)
            ImGui::EndDisabled();

        ImGui::SameLine();

        if (scene->GetState() == Scene::State::Play)
            ImGui::BeginDisabled();

        const auto& simulate_icon = (scene->GetState() == Scene::State::Simulate) ? m_StopIcon : m_SimulateIcon;
        if (ImGui::ImageButton("#sim", reinterpret_cast<ImTextureID>(simulate_icon.GetGlId()), ImVec2(size, size)))
        {
            if (scene->GetState() == Scene::State::Edit)
            {
                this->GetParent().OnSceneSimulate();
            }
            else if (scene->GetState() == Scene::State::Simulate)
            {
                this->GetParent().OnSceneStop();
            }
        }

        if (scene->GetState() == Scene::State::Play)
            ImGui::EndDisabled();

        ImGui::PopStyleVar(2);
        ImGui::End();
    }
} // namespace codex::editor
