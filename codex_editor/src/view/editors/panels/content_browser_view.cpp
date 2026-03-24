#include "content_browser_view.h"

namespace codex::editor {
    void ContentBrowserView::on_init()
    {
        auto desc = get_descriptor().lock();
    }

    void ContentBrowserView::on_imgui_render()
    {
        ImGui::Begin("Content Browser");
        ImGui::End();
    }
} // namespace codex::editor
