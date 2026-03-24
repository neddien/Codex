#pragma once

#include <engine/core/layer.h>
#include <engine/events/event.h>

#include <imgui.h>
#include <imgui_stdlib.h>

namespace codex::imgui {
    class CODEX_API ImGuiLayer : public Layer
    {
    public:
        ImGuiLayer();
        ~ImGuiLayer();

    public:
        inline void               block_events(const bool block) noexcept { blocking_ = block; }
        [[nodiscard]] inline auto imgui_context() const noexcept -> ImGuiContext* { return current_context_; }

    public:
        auto on_attach() -> void override;
        auto on_detach() -> void override;
        auto on_event(events::Event& event) -> void override;

    public:
        auto begin() -> void;
        auto end() -> void;
        auto set_dark_theme_colours() -> void;
        auto active_widget_id() const -> u32;

    private:
        bool          blocking_        = true;
        ImGuiContext* current_context_ = nullptr;
    };
} // namespace codex::imgui
