#pragma once

#include "ivfs_mount.h"

namespace codex::fs {
    class DiskMount : public IVFSMount
    {
    public:
        DiskMount(std::filesystem::path root, const i32 priority);

    public:
        [[nodiscard]] bool                    exists(const std::string& path) const noexcept override;
        [[nodiscard]] Shared<FileHandle> open(const std::string& path,
                                                   FileProperties     props = {}) noexcept override;
        [[nodiscard]] i32                     priority() const noexcept override;

        bool                                   mkdir(const std::string& rel_path) noexcept override;
        [[nodiscard]] std::vector<std::string> list(const std::string& rel_path,
                                                    ListOptions        opts = ListOptions::None) const noexcept override;
        [[nodiscard]] bool                     is_directory(const std::string& rel_path) const noexcept override;

    private:
        [[nodiscard]] std::filesystem::path abs(const std::string& rel_path) const noexcept;

    private:
        std::filesystem::path root_;
        i32                   priority_;
    };
} // namespace codex::fs
