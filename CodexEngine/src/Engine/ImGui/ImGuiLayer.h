#ifndef CODEX_IMGUI_LAYER_H
#define CODEX_IMGUI_LAYER_H

#include <sdafx.h>

#include <Engine/Core/Layer.h>
#include <Engine/Events/Event.h>

#include <imgui.h>

namespace codex::imgui {
    class CODEX_API ImGuiLayer : public Layer
    {
    private:
        bool          m_Blocking       = true;
        ImGuiContext* m_CurrentContext = nullptr;

    public:
        ImGuiLayer();
        ~ImGuiLayer();

    public:
        inline void               BlockEvents(const bool block) noexcept { m_Blocking = block; }
        [[nodiscard]] inline auto GetImGuiContext() const noexcept -> ImGuiContext* { return m_CurrentContext; }

    public:
        auto OnAttach() -> void override;
        auto OnDetach() -> void override;
        auto OnEvent(events::Event& event) -> void override;

    public:
        auto Begin() -> void;
        auto End() -> void;
        auto SetDarkThemeColours() -> void;
        auto GetActiveWidgetID() const -> u32;
    };
} // namespace codex::imgui

#endif // CODEX_IMGUI_LAYER_H
