#pragma once

#include "cxpak.h"
#include "ivfs_mount.h"

#include <engine/concurrency/public/task.h>
#include <engine/core/public/uuid.h>
#include <engine/memory/public/memory.h>

namespace codex::fs {
    class VFS
    {
    private:
        struct Node
        {
            std::string                                     path;
            std::vector<mem::Shared<IVFSMount>>             mounts;
            std::unordered_map<std::string, mem::Box<Node>> children;
            bool                                            dir;
        };

    public:
        VFS(mem::Shared<IVFSMount> default_mount = nullptr) noexcept;

        // Mount management
        bool mount(mem::Shared<IVFSMount> mount, const std::string& path, const bool mkdir = false) noexcept;
        bool unmount(const std::string& path, mem::Shared<IVFSMount> mount = nullptr) noexcept;
        // void set_priority(const std::filesystem::path& mount_point, const i32 priority);

        // File operations (tries mounts in priority order)
        [[nodiscard]] mem::Shared<FileHandle> open(const std::string& path, const FileProperties props = {}) noexcept;
        [[nodiscard]] bool                    exists(const std::string& path) const noexcept;
        [[nodiscard]] usize                   get_size(const std::string& path) const noexcept;
        bool                                  mkdir(const std::string& path, const bool recursive = false) noexcept;

        // Directory operations
        [[nodiscard]] std::vector<std::string> list(const std::string& dir,
                                                    ListOptions        opts = ListOptions::None) const noexcept;
        [[nodiscard]] bool                     is_directory(const std::string& path) const noexcept;

        // Async file operations
        [[nodiscard]] cc::Task<mem::Shared<FileHandle>>  open_async(std::string    path,
                                                                    FileProperties props = {}) noexcept;
        [[nodiscard]] cc::Task<bool>                     exists_async(std::string path) const noexcept;
        [[nodiscard]] cc::Task<bool>                     mkdir_async(std::string path, bool recursive = false) noexcept;
        [[nodiscard]] cc::Task<std::vector<std::string>> list_async(std::string dir) const noexcept;
        [[nodiscard]] cc::Task<bool>                     is_directory_async(std::string path) const noexcept;
        bool export_to_pak(mem::Shared<FileHandle> out, const PakProperties props = {}) noexcept;

        // Debugging
        // std::vector<std::string> get_mount_points() const;
        // IVFSMount*               find_mount_for_path(const std::string& path);

    private:
        Node* walk_to(const std::string& path, Node** const previous_node = nullptr) noexcept;
        bool  mkdir_nolock(const std::string& path, const bool recursive) noexcept;

        [[nodiscard]] std::vector<std::string> list_nolock(const std::string& dir) const noexcept;
        [[nodiscard]] bool                     is_directory_nolock(const std::string& path) const noexcept;

    private:
        mem::Box<Node>            root_;
        mutable std::shared_mutex mutex_;
    };
} // namespace codex::fs
