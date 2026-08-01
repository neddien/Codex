#include "console_man.h"

#include "editor.h"

#include <imgui.h>
#include <imgui_internal.h>

namespace codex::editor {
    std::deque<std::string> ConsoleMan::s_output_;
    std::mutex              ConsoleMan::s_mutex_;
    bool                    ConsoleMan::s_autoscroll_ = true;

    void ConsoleMan::on_attach()
    {
    }

    void ConsoleMan::on_update([[maybe_unused]] const f32 deltaTime)
    {
    }

    void ConsoleMan::on_imgui_render()
    {
        ImGui::Begin("Console");
        if (ImGui::BeginChild("scroll_reg", ImVec2{ 0.0f, 0.0f }, 0, ImGuiWindowFlags_HorizontalScrollbar)) {
            {
                std::scoped_lock guard{ s_mutex_ };
                ImGui::PushFont(Editor::get_console_font());
                for (const auto& e : s_output_)
                    ImGui::TextUnformatted(e.data(), e.data() + e.size());
                ImGui::PopFont();
            }

            // Terminal-style autoscroll: follow the output until the user scrolls away
            // (mouse wheel up or grabbing the scrollbar), resume once the scroll is
            // brought back to the very end.
            if (ImGui::GetScrollY() >= ImGui::GetScrollMaxY() - 1.0f)
                s_autoscroll_ = true;

            ImGuiWindow* window = ImGui::GetCurrentWindow();
            const bool   grabbed_scrollbar =
                ImGui::GetActiveID() == ImGui::GetWindowScrollbarID(window, ImGuiAxis_Y);
            const bool wheeled_up = ImGui::IsWindowHovered() && ImGui::GetIO().MouseWheel > 0.0f;
            if (grabbed_scrollbar || wheeled_up)
                s_autoscroll_ = false;

            if (s_autoscroll_)
                ImGui::SetScrollHereY(1.0f);
        }
        ImGui::EndChild();

        ImGui::End();
    }

    void ConsoleMan::append_message(const std::string_view msg) noexcept
    {
        std::scoped_lock guard{ s_mutex_ };
        s_output_.emplace_back(msg);
    }

} // namespace codex::editor
