#pragma once

#include <codex.h>

namespace codex::editor {
    struct EditorProject : public ISerializable, public Loggable<"EditorProject">
    {
        std::string last_asset_path;
        AssetPath   last_open_scene;
        std::string vfs_root = "/editor";

        void serialize(ISerializationNode& node) const override;
        void deserialize(const ISerializationNode& node) override;
    };
} // namespace codex::editor
