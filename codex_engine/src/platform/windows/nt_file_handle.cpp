#include "nt_file_handle.h"

#include <engine/filesystem/public/filesystem.h>

namespace codex::fs {
    NTFileHandle::NTFileHandle(HANDLE handle, std::string path, const FileProperties props) noexcept
        : handle_{ handle }
        , path_{ std::move(path) }
        , props_{ props }
    {
        if (props_.mode & FileMode::Append) {
            LARGE_INTEGER zero{};
            SetFilePointerEx(handle_, zero, nullptr, FILE_END);
        }
    }

    NTFileHandle::~NTFileHandle() noexcept
    {
        flush();
        if (handle_ != INVALID_HANDLE_VALUE)
            CloseHandle(handle_);
    }

    usize NTFileHandle::read(void* dest, const usize len) noexcept
    {
        std::scoped_lock lock{ mutex_ };

        if (!(props_.mode & FileMode::Read))
            return 0;

        DWORD bytes_read = 0;
        ReadFile(handle_, dest, static_cast<DWORD>(len), &bytes_read, nullptr);
        return static_cast<usize>(bytes_read);
    }

    usize NTFileHandle::write(const void* src, const usize len) noexcept
    {
        std::scoped_lock lock{ mutex_ };

        if (!(props_.mode & (FileMode::Write | FileMode::Append)))
            return 0;

        DWORD bytes_written = 0;
        WriteFile(handle_, src, static_cast<DWORD>(len), &bytes_written, nullptr);
        return static_cast<usize>(bytes_written);
    }

    usize NTFileHandle::read_at(void* dest, const usize len, const usize offset) noexcept
    {
        if (!(props_.mode & FileMode::Read))
            return 0;

        OVERLAPPED ov{};
        ov.Offset     = static_cast<DWORD>(offset & 0xFFFFFFFF);
        ov.OffsetHigh = static_cast<DWORD>(offset >> 32);

        DWORD bytes_read = 0;
        ReadFile(handle_, dest, static_cast<DWORD>(len), &bytes_read, &ov);
        return static_cast<usize>(bytes_read);
    }

    usize NTFileHandle::write_at(const void* src, const usize len, const usize offset) noexcept
    {
        if (!(props_.mode & (FileMode::Write | FileMode::Append)))
            return 0;

        OVERLAPPED ov{};
        ov.Offset     = static_cast<DWORD>(offset & 0xFFFFFFFF);
        ov.OffsetHigh = static_cast<DWORD>(offset >> 32);

        DWORD bytes_written = 0;
        WriteFile(handle_, src, static_cast<DWORD>(len), &bytes_written, &ov);
        return static_cast<usize>(bytes_written);
    }

    void NTFileHandle::seek(const usize offset) noexcept
    {
        std::scoped_lock lock{ mutex_ };
        LARGE_INTEGER    li;
        li.QuadPart = static_cast<LONGLONG>(offset);
        SetFilePointerEx(handle_, li, nullptr, FILE_BEGIN);
    }

    usize NTFileHandle::tell() const noexcept
    {
        std::shared_lock lock{ mutex_ };
        LARGE_INTEGER    zero{}, pos{};
        SetFilePointerEx(handle_, zero, &pos, FILE_CURRENT);
        return static_cast<usize>(pos.QuadPart);
    }

    usize NTFileHandle::size() const noexcept
    {
        std::shared_lock lock{ mutex_ };
        LARGE_INTEGER    size{};
        GetFileSizeEx(handle_, &size);
        return static_cast<usize>(size.QuadPart);
    }

    std::string NTFileHandle::path() const noexcept
    {
        std::shared_lock lock{ mutex_ };
        return path_;
    }

    void NTFileHandle::flush()
    {
        std::scoped_lock lock{ mutex_ };
        if (handle_ != INVALID_HANDLE_VALUE && (props_.mode & (FileMode::Write | FileMode::Append)))
            FlushFileBuffers(handle_);
    }
} // namespace codex::fs
