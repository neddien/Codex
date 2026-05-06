#include <codex.h>
#include <engine/core/entry_point.h>

#include "editor_application.h"

codex::Engine* codex::create_engine(codex::EngineArgs args)
{
    return new codex::editor::EditorApplication(
        codex::EngineProperties{ .name              = "CodexEditor",
                                 .cwd               = "./",
                                 .args              = std::move(args),
                                 .window_properties = {
                                     .title     = "Codex Editor",
                                     .width     = 1280,
                                     .height    = 720,
                                     .frame_cap = 300,
                                     .flags     = codex::WindowFlags::Visible | codex::WindowFlags::Resizable |
                                              codex::WindowFlags::PositionCentre | codex::WindowFlags::Maximized,
                                     .vsync = false,
                                 } });
}
