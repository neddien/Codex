#include <codex.h>
#include <engine/core/entry_point.h>

#include "editor_application.h"

codex::Engine* codex::create_engine(codex::EngineArgs args)
{
    EngineProject project;
    project.name              = "CodexEditor";
    project.engine_ver        = { CX_VER_MAJ, CX_VER_MIN, CX_VER_REV, CX_BUILD_COUNT };
    project.boot_scene        = AssetPath{ "/editor/scenes/default.cxscene" };
    project.engine_properties = { .name              = "CodexEditor",
                                  .cwd               = "./",
                                  .args              = std::move(args),
                                  .window_properties = {
                                      .title     = "Codex Editor",
                                      .width     = 1920,
                                      .height    = 1080,
                                      .frame_cap = 300,
                                      .flags     = codex::WindowFlags::Visible | codex::WindowFlags::Resizable |
                                               codex::WindowFlags::PositionCentre | codex::WindowFlags::Maximized,
                                      .vsync = false,
                                  } };

    return new codex::editor::EditorApplication{ project };
}
