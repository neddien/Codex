#pragma once

#include "ivfs_mount.h"

namespace codex::fs {
    class MemoryMount : public IVFSMount, public std::enable_shared_from_this<MemoryMount>
    {
        friend class MemoryFileHandle;

    private:
        struct FileEntry
        {
            std::vector<u8>   buffer;
            std::shared_mutex mutex;
            u64               last_modif;
            bool              read_only;
        };

    public:
        MemoryMount(const i32 priority);

    public:
        [[nodiscard]] bool               exists(const std::string& path) const noexcept override;
        [[nodiscard]] Shared<FileHandle> open(const std::string&   path,
                                              const FileProperties props = {}) noexcept override;
        [[nodiscard]] i32                priority() const noexcept override;

        bool                                   mkdir(const std::string& rel_path) noexcept override;
        bool                                   rm(const std::string& rel_path) noexcept override;
        [[nodiscard]] std::vector<std::string> list(const std::string& rel_path,
                                                    ListOptions opts = ListOptions::None) const noexcept override;
        [[nodiscard]] bool                     is_directory(const std::string& rel_path) const noexcept override;

    private:
        std::unordered_map<std::string, FileEntry> files_;
        std::unordered_set<std::string>            dirs_;
        i32                                        priority_;
        mutable std::shared_mutex                  mutex_;
    };
} // namespace codex::fs
