#pragma once

#include "IVFSMount.h"

#include <Engine/Core/Public/UUID.h>
#include <Engine/FileSystem/IVFSMount.h>
#include <Engine/Memory/Public/Box.h>

namespace codex::fs {
    class VFS
    {
    public:
        struct Entry
        {
            std::filesystem::path virtual_path;
            IVFSMount*            mount;
            std::filesystem::path mount_path;
            i32                   priority;
        };

    public:
        VFS();

        // Mount management
        void Mount(mem::Box<IVFSMount> mount);
        // void SetPriority(const std::filesystem::path& mountPoint, const i32 priority);

        // File operations (tries mounts in priority order)
        bool            Exists(const std::filesystem::path& path);
        std::vector<u8> Read(const std::filesystem::path& path);
        usize           GetSize(const std::filesystem::path& path);

        // Directory operations
        std::vector<std::string> List(const std::filesystem::path& dir);
        bool                     IsDirectory(const std::filesystem::path& path);

        // Debugging
        std::vector<std::string> GetMountPoints() const;
        IVFSMount*               FindMountForPath(const std::filesystem::path& path);

    private:
        std::unordered_map<UUID, std::vector<Entry>>                 m_Files;
        std::unordered_map<UUID, std::vector<std::filesystem::path>> m_Dirs;
        std::vector<IVFSMount>                                       m_Mounts;
    };
} // namespace codex::fs
