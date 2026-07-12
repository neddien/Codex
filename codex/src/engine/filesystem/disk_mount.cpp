#include "disk_mount.h"

#include "public/filesystem.h"

#if defined(CX_PLATFORM_LINUX)
#include <fcntl.h>
#include <platform/linux/linux_file_handle.h>
#elif defined(CX_PLATFORM_WINDOWS)
#include <platform/windows/nt_file_handle.h>
#endif

namespace codex::fs {
    DiskMount::DiskMount(std::filesystem::path root, const i32 priority)
        : root_{ std::move(root) }
        , priority_{ priority }
    {
    }

    std::filesystem::path DiskMount::abs(const std::string& rel_path) const noexcept
    {
        const auto norm = normalize(rel_path);
        return norm.empty() ? root_ : root_ / norm;
    }

    bool DiskMount::exists(const std::string& path) const noexcept
    {
        return std::filesystem::exists(abs(path));
    }

    i32 DiskMount::priority() const noexcept
    {
        return priority_;
    }

    bool DiskMount::mkdir(const std::string& rel_path) noexcept
    {
        std::error_code ec;
        std::filesystem::create_directories(abs(rel_path), ec);
        return !ec;
    }

    bool DiskMount::rm(const std::string& rel_path) noexcept
    {
        std::error_code ec;
        std::filesystem::remove(abs(rel_path), ec);
        return !ec;
    }

    bool DiskMount::cp(const std::string& src_rel_path, const std::string& dst_rel_path, const bool recursive) noexcept
    {
        std::error_code               ec;
        std::filesystem::copy_options opts = std::filesystem::copy_options::overwrite_existing;

        if (recursive)
            opts |= std::filesystem::copy_options::recursive;

        std::filesystem::copy(abs(src_rel_path), abs(dst_rel_path), opts, ec);

        if (ec)
            log(Error, "Failed to copy {} -> {}: {}", src_rel_path, dst_rel_path, ec.message());

        return !ec;
    }

    bool DiskMount::mv(const std::string& src_rel_path, const std::string& dst_rel_path) noexcept
    {
        std::error_code ec;
        std::filesystem::rename(abs(src_rel_path), abs(dst_rel_path), ec);
        return !ec;
    }

    std::vector<std::string> DiskMount::list(const std::string& rel_path, const ListOptions opts) const
    {
        std::vector<std::string> result;
        const bool               files_only = opts & ListOptions::FilesOnly;
        const bool               dirs_only  = opts & ListOptions::DirsOnly;
        const auto               base       = abs(rel_path);

        if (opts & ListOptions::Recursive) {
            for (const auto& entry : std::filesystem::recursive_directory_iterator(base)) {
                const bool is_dir = entry.is_directory();
                if ((is_dir && files_only) || (!is_dir && dirs_only))
                    continue;
                result.push_back(std::filesystem::relative(entry.path(), base).string());
            }
        } else {
            for (const auto& entry : std::filesystem::directory_iterator(base)) {
                const bool is_dir = entry.is_directory();
                if ((is_dir && files_only) || (!is_dir && dirs_only))
                    continue;
                result.push_back(entry.path().filename().string());
            }
        }

        return result;
    }

    bool DiskMount::directory(const std::string& rel_path) const noexcept
    {
        return std::filesystem::is_directory(abs(rel_path));
    }

    bool DiskMount::empty(const std::string& rel_path) const noexcept
    {
        return std::filesystem::is_empty(abs(rel_path));
    }

    std::string DiskMount::mount_src() const noexcept
    {
        return root_.generic_string();
    }

    Shared<FileHandle> DiskMount::open(const std::string& path, const FileProperties props) noexcept
    {
        const auto full_path = abs(path);

#if defined(CX_PLATFORM_LINUX)
        int flags = 0;

        const bool read  = props.mode & FileMode::Read;
        const bool write = props.mode & (FileMode::Write | FileMode::Append);

        if (read && write)
            flags = O_RDWR;
        else if (write)
            flags = O_WRONLY;
        else
            flags = O_RDONLY;

        if (props.mode & FileMode::Create)
            flags |= O_CREAT;
        if (props.mode & FileMode::Trunc)
            flags |= O_TRUNC;
        if (props.mode & FileMode::Append)
            flags |= O_APPEND;

        const int fd = ::open(full_path.c_str(), flags, 0644);
        if (fd < 0)
            return nullptr;

        return Shared<LinuxFileHandle>::make(fd, normalize(path), props);

#elif defined(CX_PLATFORM_WINDOWS)
        DWORD access = 0;
        if (props.mode & FileMode::Read)
            access |= GENERIC_READ;
        if (props.mode & (FileMode::Write | FileMode::Append))
            access |= GENERIC_WRITE;

        DWORD disposition = OPEN_EXISTING;
        if ((props.mode & FileMode::Create) && (props.mode & FileMode::Trunc))
            disposition = CREATE_ALWAYS;
        else if (props.mode & FileMode::Create)
            disposition = OPEN_ALWAYS;
        else if (props.mode & FileMode::Trunc)
            disposition = TRUNCATE_EXISTING;

        const HANDLE handle = CreateFileW(full_path.c_str(), access, FILE_SHARE_READ, nullptr, disposition,
                                          FILE_ATTRIBUTE_NORMAL, nullptr);
        if (handle == INVALID_HANDLE_VALUE)
            return nullptr;

        return Shared<NTFileHandle>::make(handle, normalize(path), props);

#else
        return nullptr;
#endif
    }
} // namespace codex::fs
