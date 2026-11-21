#pragma once

#include <engine/filesystem/public/filesystem.h>

namespace codex::sys {
    [[nodiscard]] u32 get_engine_thread_count() noexcept;
    [[nodiscard]] u64 get_current_thread_id() noexcept;

    [[nodiscard]] inline std::filesystem::path module_path() noexcept
    {
#if defined(CX_PLATFORM_WINDOWS)
        std::wstring path(fs::kMaxPathLength, 0);
        path.resize(GetModuleFileNameW(nullptr, path.data(), (DWORD)path.size()));
        return path;
#elif defined(CX_PLATFORM_LINUX)
        std::string path(fs::kMaxPathLength, 0);
        const auto  len = readlink("/proc/self/exe", path.data(), path.size());
        path.resize(len > 0 ? (usize)len : 0);
        return path;
#endif
    }
} // namespace codex::sys
