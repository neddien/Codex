#include "ConsoleMan.h"

namespace codex::editor {
    std::deque<std::string> ConsoleMan::m_Output;

    void ConsoleMan::OnAttach()
    {
    }

    void ConsoleMan::OnUpdate(const f32 deltaTime)
    {
    }

    void ConsoleMan::OnImGuiRender()
    {
        ImGui::Begin("Console");
        const float footer_height = ImGui::GetStyle().ItemSpacing.y + ImGui::GetFrameHeightWithSpacing();
        if (ImGui::BeginChild("scroll_reg"))
        {
            try
            {
                for (const auto& e : m_Output)
                    ImGui::Text("%s", e.c_str());
            }
            catch (...)
            {
            }
        }
        ImGui::EndChild();

        ImGui::End();
    }

    void ConsoleMan::AppendMessage(const std::string_view msg) noexcept
    {
        m_Output.emplace_back(msg);
    }

} // namespace codex::editor