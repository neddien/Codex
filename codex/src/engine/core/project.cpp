#include "project.h"

namespace codex {
    void EngineProject::serialize(ISerializationNode& node) const
    {
    }

    void EngineProject::deserialize(const ISerializationNode& node)
    {
    }

    void EngineProject::save_to_disk(const std::filesystem::path& proj_path)
    {
    }

    void EngineProject::save_to_vfs(fs::VirtualFilesystem& vfs, const std::filesystem::path& proj_path)
    {
    }

    Box<EngineProject> EngineProject::load_from_disk(const std::filesystem::path& proj_path)
    {
    }

    Box<EngineProject> EngineProject::load_from_vfs(fs::VirtualFilesystem& vfs, const std::filesystem::path& proj_path)
    {
    }

    void EngineUserProject::serialize(ISerializationNode& node) const
    {
    }

    void EngineUserProject::deserialize(const ISerializationNode& node)
    {
    }
} // namespace codex
