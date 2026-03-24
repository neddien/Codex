#pragma once

#include <engine/file_system/file_handle.h>

namespace codex::fs {
    class LinuxFileHandle : public FileHandle
    {
    public:
        LinuxFileHandle(const i32 fd, std::string path, const FileProperties props) noexcept;
        ~LinuxFileHandle() noexcept override;

    public:
        [[nodiscard]] usize       read(void* dest, usize len) noexcept override;
        [[nodiscard]] usize       write(const void* src, usize len) noexcept override;
        [[nodiscard]] usize       read_at(void* dest, usize len, usize offset) noexcept override;
        [[nodiscard]] usize       write_at(const void* src, usize len, usize offset) noexcept override;
        void                      seek(usize offset) noexcept override;
        [[nodiscard]] usize       tell() const noexcept override;
        [[nodiscard]] usize       size() const noexcept override;
        [[nodiscard]] std::string path() const noexcept override;
        void                      flush() override;

    private:
        int                       fd_;
        std::string               path_;
        FileProperties            props_;
        mutable std::shared_mutex mutex_;
    };
} // namespace codex::fs
