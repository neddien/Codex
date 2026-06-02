#pragma once

#include "public/file_handle.h"

#include <engine/memory/public/memory.h>
#include <engine/utils/public/bitwise_enum.h>

namespace codex::fs {
    enum class ListOptions : u8
    {
        None      = 0,
        Recursive = bit(0),
        FilesOnly = bit(1),
        DirsOnly  = bit(2),
    };
    CX_ENABLE_BITWISE_ENUM(ListOptions)

    class IVFSMount
    {
    public:
        virtual ~IVFSMount() = default;

        [[nodiscard]] virtual bool               exists(const std::string& path) const noexcept = 0;
        [[nodiscard]] virtual Shared<FileHandle> open(const std::string&   path,
                                                      const FileProperties props = {}) noexcept = 0;
        [[nodiscard]] virtual i32                priority() const noexcept                      = 0;
        virtual bool                             mkdir(const std::string& rel_path) noexcept { return false; }
        virtual bool                             rm(const std::string& rel_path) noexcept { return false; }
        virtual bool                             cp(const std::string& src_rel_path, const std::string& dst_rel_path,
                                                    const bool recursive = false) noexcept
        {
            return false;
        }
        virtual bool mv(const std::string& src_rel_path, const std::string& dst_rel_path) noexcept { return false; }
        [[nodiscard]] virtual std::vector<std::string> list(const std::string& rel_path,
                                                            const ListOptions  opts = ListOptions::None) const noexcept
        {
            return {};
        }
        [[nodiscard]] virtual bool        is_directory(const std::string& rel_path) const noexcept { return false; }
        [[nodiscard]] virtual std::string mount_src() const noexcept
        {
            return "";
        } // Virtual mounts (like MemoryMount) don't need this.

        // Async wrappers, non-virtual, dispatch through the virtual sync methods.
        // Override the sync methods for custom behaviour; override these for native async (e.g. io_uring).
        [[nodiscard]] cc::Task<Shared<FileHandle>> open_async(std::string path, FileProperties props = {}) noexcept;
        [[nodiscard]] cc::Task<bool>               exists_async(std::string path) const noexcept;
        [[nodiscard]] cc::Task<bool>               mkdir_async(std::string path) noexcept;
        [[nodiscard]] cc::Task<bool>               rm_async(std::string path) noexcept;
        [[nodiscard]] cc::Task<std::vector<std::string>> list_async(std::string path) const noexcept;
        [[nodiscard]] cc::Task<bool>                     is_directory_async(std::string path) const noexcept;
    };
} // namespace codex::fs
