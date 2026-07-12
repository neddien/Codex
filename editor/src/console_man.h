#pragma once

#include <codex.h>

namespace codex::editor {
    class ConsoleMan : public Layer
    {
    private:
        // Appended from process-output threads while the UI thread renders, so
        // every access goes through the mutex.
        static std::deque<std::string> s_output_;
        static std::mutex              s_mutex_;
        static bool                    s_autoscroll_;

    public:
        void on_attach() override;
        void on_update(const f32 deltaTime) override;
        void on_imgui_render() override;

    public:
        static void append_message(const std::string_view msg) noexcept;
    };
} // namespace codex::editor
