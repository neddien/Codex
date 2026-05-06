#pragma once

#include <codex.h>

namespace codex::editor {
    class ConsoleMan : public Layer
    {
    private:
        static std::deque<std::string> s_output_;

    public:
        void on_attach() override;
        void on_update(const f32 deltaTime) override;
        void on_imgui_render() override;

    public:
        static void append_message(const std::string_view msg) noexcept;
    };
} // namespace codex::editor
