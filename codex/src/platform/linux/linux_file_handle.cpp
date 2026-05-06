#include "linux_file_handle.h"

#include <engine/filesystem/public/filesystem.h>

#include <sys/stat.h>

namespace codex::fs {
    LinuxFileHandle::LinuxFileHandle(const i32 fd, std::string path, const FileProperties props) noexcept
        : fd_{ fd }
        , path_{ std::move(path) }
        , props_{ props }
    {
        if (props_.mode & FileMode::Append)
            ::lseek(fd_, 0, SEEK_END);
    }

    LinuxFileHandle::~LinuxFileHandle() noexcept
    {
        flush();
        if (fd_ >= 0)
            ::close(fd_);
    }

    usize LinuxFileHandle::read(void* dest, const usize len) noexcept
    {
        std::scoped_lock lock{ mutex_ };

        if (!(props_.mode & FileMode::Read))
            return 0;

        const auto result = ::read(fd_, dest, len);
        return result < 0 ? 0 : static_cast<usize>(result);
    }

    usize LinuxFileHandle::write(const void* src, const usize len) noexcept
    {
        std::scoped_lock lock{ mutex_ };

        if (!(props_.mode & (FileMode::Write | FileMode::Append)))
            return 0;

        const auto result = ::write(fd_, src, len);
        return result < 0 ? 0 : static_cast<usize>(result);
    }

    usize LinuxFileHandle::read_at(void* dest, const usize len, const usize offset) noexcept
    {
        if (!(props_.mode & FileMode::Read))
            return 0;

        const auto result = ::pread(fd_, dest, len, static_cast<off_t>(offset));
        return result < 0 ? 0 : static_cast<usize>(result);
    }

    usize LinuxFileHandle::write_at(const void* src, const usize len, const usize offset) noexcept
    {
        if (!(props_.mode & (FileMode::Write | FileMode::Append)))
            return 0;

        const auto result = ::pwrite(fd_, src, len, static_cast<off_t>(offset));
        return result < 0 ? 0 : static_cast<usize>(result);
    }

    void LinuxFileHandle::seek(const usize offset) noexcept
    {
        std::scoped_lock lock{ mutex_ };
        ::lseek(fd_, static_cast<off_t>(offset), SEEK_SET);
    }

    usize LinuxFileHandle::tell() const noexcept
    {
        std::shared_lock lock{ mutex_ };
        const auto       pos = ::lseek(fd_, 0, SEEK_CUR);
        return pos < 0 ? 0 : static_cast<usize>(pos);
    }

    usize LinuxFileHandle::size() const noexcept
    {
        std::shared_lock lock{ mutex_ };
        struct stat      st;
        if (::fstat(fd_, &st) < 0)
            return 0;
        return static_cast<usize>(st.st_size);
    }

    std::string LinuxFileHandle::path() const noexcept
    {
        std::shared_lock lock{ mutex_ };
        return path_;
    }

    u64 LinuxFileHandle::last_modified() const noexcept
    {
        std::shared_lock lock{ mutex_ };

        struct stat sb;
        if (fstat(fd_, &sb) < 0)
            return 0;

        return static_cast<u64>(sb.st_mtim.tv_sec);
    }

    void LinuxFileHandle::flush()
    {
        std::scoped_lock lock{ mutex_ };
        if (fd_ >= 0 && (props_.mode & (FileMode::Write | FileMode::Append)))
            ::fsync(fd_);
    }
} // namespace codex::fs
