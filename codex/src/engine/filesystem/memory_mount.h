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
            struct Descriptor
            {
                std::string     path;
                std::vector<u8> buffer;
                u64             last_modif = 0;
                bool            read_only  = false;
            } desc;
            mutable std::shared_mutex mutex;

        public:
            FileEntry() noexcept = default;
            FileEntry(Descriptor descriptor) noexcept
                : desc{ std::move(descriptor) }
            {
            }
            FileEntry(const FileEntry& other) noexcept
                : desc{ other.desc }
            {
            }
            FileEntry(FileEntry&& other) noexcept
                : desc{ std::move(other.desc) }
            {
            }
            FileEntry& operator=(const FileEntry& other) noexcept { return FileEntry{ other }.swap(*this); }
            FileEntry& operator=(FileEntry&& other) noexcept { return FileEntry{ std::move(other) }.swap(*this); }

        public:
            FileEntry& swap(FileEntry& other) noexcept
            {
                std::swap(desc, other.desc);
                return *this;
            }
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
                                                    ListOptions        opts = ListOptions::None) const override;
        [[nodiscard]] bool                     directory(const std::string& rel_path) const noexcept override;
        [[nodiscard]] bool                     empty(const std::string& rel_path) const noexcept override;

    private:
        // TODO: FileEntry must be a shared ptr cuz we can have an open file handle on FileEntry but delete FileEntry
        // from here at the same time, not good
        absl::flat_hash_map<std::string, FileEntry> files_;
        absl::flat_hash_set<std::string>            dirs_;
        i32                                         priority_;
        mutable std::shared_mutex                   mutex_;
    };
} // namespace codex::fs
