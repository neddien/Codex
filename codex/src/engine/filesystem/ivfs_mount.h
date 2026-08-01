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
        virtual bool mkdir([[maybe_unused]] const std::string& rel_path) noexcept { return false; }
        virtual bool rm([[maybe_unused]] const std::string& rel_path) noexcept { return false; }
        virtual bool cp([[maybe_unused]] const std::string& src_rel_path,
                        [[maybe_unused]] const std::string& dst_rel_path,
                        [[maybe_unused]] const bool         recursive = false) noexcept
        { return false; }
        virtual bool mv([[maybe_unused]] const std::string& src_rel_path,
                        [[maybe_unused]] const std::string& dst_rel_path) noexcept
        { return false; }
        [[nodiscard]] virtual std::vector<std::string> list(
            [[maybe_unused]] const std::string& rel_path,
            [[maybe_unused]] const ListOptions  opts = ListOptions::None) const
        { return {}; }
        [[nodiscard]] virtual bool directory([[maybe_unused]] const std::string& rel_path) const noexcept
        { return false; }
        [[nodiscard]] virtual bool empty([[maybe_unused]] const std::string& rel_path) const noexcept { return false; }
        [[nodiscard]] virtual std::string mount_src() const noexcept
        { return ""; } // Virtual mounts (like MemoryMount) don't need this.
    };
} // namespace codex::fs
