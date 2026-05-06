#include "dynamic_library.h"

namespace codex::sys {
    DLib::DLib(std::filesystem::path file_path)
        : file_path_(std::move(file_path))
    {
        const auto& file_path_str = file_path_.string();
#if defined(CX_PLATFORM_UNIX)
        handle = dlopen(file_path_str.c_str(), RTLD_LAZY);
        // printf("Err: %s\n", dlerror());
#elif defined(CX_PLATFORM_WINDOWS)
        handle = LoadLibraryA(file_path_str.c_str());
#endif
        if (!handle)
            throw DynamicLibraryLoadException("Failed to load '{}'.", file_path_str);
    }

    DLib::DLib(DLib&& other) noexcept
    {
        if ((uintptr)other.handle) {
            handle     = other.handle;
            file_path_ = other.file_path_;

            other.handle     = (DLibInstance) nullptr;
            other.file_path_ = "";
        }
    }

    DLib& DLib::operator=(DLib&& other) noexcept
    {
        if ((uintptr)other.handle) {
            handle     = other.handle;
            file_path_ = other.file_path_;

            other.handle     = (DLibInstance) nullptr;
            other.file_path_ = "";
        }
        return *this;
    }

    DLib::~DLib() noexcept
    {
#if defined(CX_PLATFORM_UNIX)
        dlclose(handle);
#elif defined(CX_PLATFORM_WINDOWS)
        FreeLibrary(handle);
#endif

        // FIXME: There's a bug where a DLib instance might have static storage and
        // might get freed after the logger so this line right here will cause a crash.
        log(Info, "~DLib(): {}", file_path_.string());

        handle     = (DLibInstance) nullptr;
        file_path_ = std::filesystem::path{};
    }
} // namespace codex::sys
