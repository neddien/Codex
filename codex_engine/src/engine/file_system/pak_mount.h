#pragma once

#include "cxpak.h"
#include "ivfs_mount.h"

namespace codex::fs {
    class PakMount : public IVFSMount, public mem::SharedManagable<PakMount>
    {
    public:
        PakMount(mem::Shared<FileHandle> handle, const i32 priority);

    public:
        [[nodiscard]] bool                     exists(const std::string& path) const noexcept override;
        [[nodiscard]] mem::Shared<FileHandle>  open(const std::string&   path,
                                                    const FileProperties props = {}) noexcept override;
        [[nodiscard]] i32                      priority() const noexcept override;
        bool                                   mkdir(const std::string& rel_path) noexcept override;
        [[nodiscard]] std::vector<std::string> list(const std::string& rel_path,
                                                    const ListOptions opts = ListOptions::None) const noexcept override;
        [[nodiscard]] bool                     is_directory(const std::string& rel_path) const noexcept override;
        [[nodiscard]] u64                      chunk_size() const noexcept;

        // Reads every entry, recomputes its CRC, and returns paths that fail.
        // Returns an empty vector immediately if the PAK carries PakFlags::Insecure.
        [[nodiscard]] std::vector<std::string> verify_integrity() const;

    private:
        mutable mem::Shared<FileHandle> handle_;
        PakHeader               header_;
        std::vector<PakEntry>   entries_; // sorted by path_hash for O(log N) lookup
        std::vector<std::string> dirs_;   // sorted for O(log N) is_directory / exists
        i32                     priority_;
    };
} // namespace codex::fs
