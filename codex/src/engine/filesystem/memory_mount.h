#pragma once

#include "ivfs_mount.h"

#include <engine/core/public/common_third_party_libs.h>

namespace codex::fs {
    class MemoryMount : public IVFSMount, public std::enable_shared_from_this<MemoryMount>
    {
        friend class MemoryFileHandle;

    private:
        struct FileEntry
        {
            std::string       path; // normalized path; the map key is its fnv1a hash
            std::vector<u8>   buffer;
            std::shared_mutex mutex;
            u64               last_modif = 0;
            bool              read_only  = false;
        };

    public:
        MemoryMount(const i32 priority);

    public:
        [[nodiscard]] bool               exists(const std::string& path) const noexcept override;
        [[nodiscard]] Shared<FileHandle> open(const std::string&   path,
                                              const FileProperties props = {}) noexcept override;
        [[nodiscard]] i32                priority() const noexcept override;

        bool mkdir(const std::string& rel_path) noexcept override;
        bool rm(const std::string& rel_path) noexcept override;
        bool cp(const std::string& src_rel_path, const std::string& dst_rel_path,
                const bool recursive = false) noexcept override;
        bool mv(const std::string& src_rel_path, const std::string& dst_rel_path) noexcept override;
        [[nodiscard]] std::vector<std::string> list(const std::string& rel_path,
                                                    ListOptions opts = ListOptions::None) const noexcept override;
        [[nodiscard]] bool                     is_directory(const std::string& rel_path) const noexcept override;

    private:
        // All helpers below assume `mutex_` is already held and operate on normalized paths.

        // True if `npath` names a directory: either an explicit entry in `dirs_`, or the
        // implicit parent of some existing file. The empty path is the (always-present) root.
        [[nodiscard]] bool is_dir_unlocked(const std::string& npath) const noexcept;

        // Resolves the final destination path for moving/copying `nspath` to `ndpath`.
        // If `ndpath` is an existing directory, the item lands inside it as <ndpath>/<filename>.
        // Otherwise `ndpath` is the destination name and its parent must be an existing directory.
        // Returns an empty string if the destination is invalid.
        [[nodiscard]] std::string resolve_dest(const std::string& nspath, const std::string& ndpath) const noexcept;

        // Copies (move_ == false) or moves (move_ == true) the whole subtree rooted at `src_dir`
        // to `dst_dir`, recreating the directory structure. `dst_dir` must not live inside `src_dir`.
        void transfer_tree(const std::string& src_dir, const std::string& dst_dir, const bool move_) noexcept;

    private:
        absl::flat_hash_map<usize, Box<FileEntry>> files_; // fnv1a(path) -> entry (Box keeps it pointer-stable)
        absl::flat_hash_map<usize, std::string>    dirs_;  // fnv1a(path) -> path
        i32                                        priority_;
        mutable std::shared_mutex                  mutex_;
    };
} // namespace codex::fs
