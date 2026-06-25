#include "editor_project.h"

namespace codex::editor {
    EditorProject::EditorProject() noexcept
    {
        engine_project.author = "Codex Editor";

        // TODO: This should contian the UUID for the empty scene
        // which will ship with all editor projects
        engine_project.boot_scene        = AssetPath{};
        engine_project.engine_properties = EngineProperties{
            .flags = EngineFlags::InitAll,
            .window_properties =
                WindowProperties{
                    .title  = "Codex Application",
                    .width  = 1920,
                    .height = 1080,
                    .vsync  = true,
                },
        };
    }

    void EditorProject::archive(Archive& ar)
    {
        ar("engine", engine_project);
        ar("vfs_root", vfs_root);
        ar("last_asset_path", last_asset_path);
        ar("last_open_scene", last_open_scene);
    }
} // namespace codex::editor
