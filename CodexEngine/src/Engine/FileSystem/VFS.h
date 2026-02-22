#pragma once

#include <sdafx.h>

#include "IVFSMount.h"

#include <Engine/Core/Public/UUID.h>
#include <Engine/FileSystem/IVFSMount.h>
#include <Engine/Memory/Public/Memory.h>

namespace codex::fs {
    class VFS
    {
    public:
        struct Node
        {
            bool                                       isDirectory;
            std::string                                path;
            IVFSMount*                                 owner;
            std::flat_map<std::string, mem::Box<Node>> children;
        };

    public:
        VFS() noexcept;

        // Mount management
        void Mount(mem::Shared<IVFSMount> mount, const std::string_view path);
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
        mem::Box<Node>                      m_Root;
        std::vector<mem::Shared<IVFSMount>> m_Mounts;
        IVFSMount*                          m_DefaultWriteMount;
    };
} // namespace codex::fs
