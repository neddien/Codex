#pragma once

#include <codex.h>

namespace codex::editor {
    struct EditorProject : public ISerializable, public Loggable<"EditorProject">
    {
        EngineProject engine_project;
        std::string   vfs_root = "/editor";
        std::string   last_asset_path;
        std::string   last_open_scene;

        EditorProject() noexcept;

        void archive(Archive& archive) override;
    };
} // namespace codex::editor
