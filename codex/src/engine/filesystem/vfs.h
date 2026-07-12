#pragma once

#include <shared_mutex>

#include "cxpak.h"
#include "ivfs_mount.h"

#include <engine/concurrency/public/task.h>
#include <engine/core/public/uuid.h>
#include <engine/memory/public/memory.h>

namespace codex::fs {
    class VirtualFilesystem : public Loggable<"VFS">
    {
    private:
        struct Node
        {
            std::string                                path;
            std::vector<Shared<IVFSMount>>             mounts;
            std::unordered_map<std::string, Box<Node>> children;
            bool                                       dir;
        };

    public:
        VirtualFilesystem(Shared<IVFSMount> default_mount = nullptr) noexcept;

        // Mount management
        bool mount(Shared<IVFSMount> mount, const std::string& path, const bool mkdir = false) noexcept;
        bool unmount(const std::string& path, Shared<IVFSMount> mount = nullptr) noexcept;
        // void set_priority(const std::filesystem::path& mount_point, const i32 priority);

        // File operations (tries mounts in priority order)
        [[nodiscard]] Shared<FileHandle> open(const std::string& path, FileProperties props = {}) noexcept;
        [[nodiscard]] bool               exists(const std::string& path) const noexcept;
        [[nodiscard]] usize              get_size(const std::string& path) const noexcept;
        bool                             mkdir(const std::string& path, const bool recursive = false) noexcept;
        bool ensure_mount_point(const std::string& path, const bool recursive = false) noexcept;
        bool rm(const std::string& path, const bool recursive = false) noexcept;
        bool cp(const std::string& src_path, const std::string& dst_path, const bool recursive = false) noexcept;
        bool mv(const std::string& src_path, const std::string& dst_path) noexcept;

        // Directory operations
        [[nodiscard]] std::vector<std::string> list(const std::string& dir, ListOptions opts = ListOptions::None) const;
        [[nodiscard]] bool                     is_directory(const std::string& path) const noexcept;

        // Async file operations
        [[nodiscard]] cc::task<Shared<FileHandle>> open_async(std::string path, FileProperties props = {}) noexcept;
        [[nodiscard]] cc::task<bool>               exists_async(std::string path) const noexcept;
        [[nodiscard]] cc::task<bool>               mkdir_async(std::string path, bool recursive = false) noexcept;
        [[nodiscard]] cc::task<std::vector<std::string>> list_async(std::string dir) const;
        [[nodiscard]] cc::task<bool>                     is_directory_async(std::string path) const noexcept;
        bool                                 export_to_pak(Shared<FileHandle> out, const PakProperties props = {},
                                                           const std::string& root = "/") noexcept;
        std::optional<std::filesystem::path> materialize(const std::string&           vfs_path,
                                                         const std::filesystem::path& cache_dir);

    private:
        struct resolved_node
        {
            IVFSMount*  mount;
            Node*       node;
            std::string rel_path;
            usize       depth;
        };
        resolved_node resolve_mount_nolock(const std::string& path, bool existing_only = false) const noexcept;
        Node*         walk_to(const std::string& path, Node** const previous_node = nullptr) noexcept;
        bool          mkdir_nolock(const std::string& path, const bool recursive) noexcept;
        bool          ensure_mount_point_nolock(const std::string& path, const bool recursive = false) noexcept;
        bool          rm_nolock(const std::string& path, const bool recursive) noexcept;
        bool cp_nolock(const std::string& src_path, const std::string& dst_path, const bool recursive = false) noexcept;
        bool mv_nolock(const std::string& src_path, const std::string& dst_path) noexcept;
        [[nodiscard]] std::vector<std::string> list_nolock(const std::string& dir,
                                                           ListOptions        opts = ListOptions::None) const;
        [[nodiscard]] std::vector<std::string> list_children_nolock(const std::string& dir) const;
        [[nodiscard]] bool                     is_directory_nolock(const std::string& path) const noexcept;
        [[nodiscard]] Shared<FileHandle>       open_nolock(const std::string& path, FileProperties props = {}) noexcept;

    private:
        Box<Node>                 root_;
        mutable std::shared_mutex mutex_;
    };
} // namespace codex::fs
