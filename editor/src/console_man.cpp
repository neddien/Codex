#include "console_man.h"

namespace codex::editor {
    std::deque<std::string> ConsoleMan::s_output_;

    void ConsoleMan::on_attach()
    {
    }

    void ConsoleMan::on_update(const f32 deltaTime)
    {
    }

    void ConsoleMan::on_imgui_render()
    {
        ImGui::Begin("Console");
        const float footer_height = ImGui::GetStyle().ItemSpacing.y + ImGui::GetFrameHeightWithSpacing();
        if (ImGui::BeginChild("scroll_reg")) {
            try {
                for (const auto& e : s_output_)
                    ImGui::Text("%s", e.c_str());
            }
            catch (...) {
            }
        }
        ImGui::EndChild();

        ImGui::End();
    }

    void ConsoleMan::append_message(const std::string_view msg) noexcept
    {
        s_output_.emplace_back(msg);
    }

} // namespace codex::editor
