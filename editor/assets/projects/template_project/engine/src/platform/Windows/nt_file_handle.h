#pragma once

#include <engine/filesystem/public/file_handle.h>

namespace codex::fs {
    class NTFileHandle : public FileHandle
    {
    public:
        NTFileHandle(HANDLE handle, std::string path, FileProperties props) noexcept;
        ~NTFileHandle() noexcept override;

    public:
        [[nodiscard]] usize       read(void* dest, usize len) noexcept override;
        [[nodiscard]] usize       write(const void* src, usize len) noexcept override;
        void                      seek(usize offset) noexcept override;
        [[nodiscard]] usize       tell() const noexcept override;
        [[nodiscard]] usize       size() const noexcept override;
        [[nodiscard]] std::string path() const noexcept override;
        void                      flush() override;

    private:
        HANDLE         handle_;
        std::string    path_;
        FileProperties props_;
    };
} // namespace codex::fs
