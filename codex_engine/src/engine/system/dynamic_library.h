#pragma once

#include <engine/core/public/exception.h>

namespace codex::sys {
    CX_CUSTOM_EXCEPTION(DynamicLibraryLoadException, "Failed to load dynamic library.")
    CX_CUSTOM_EXCEPTION(DynamicLibraryInvokeException, "Failed to invoke function")

    class CODEX_API DLib
    {
    private:
#ifdef CX_PLATFORM_WINDOWS
        using DLibInstance = HINSTANCE;
#elif defined(CX_PLATFORM_UNIX)
        using DLibInstance = void*;
#endif

    private:
        DLibInstance          handle = (DLibInstance) nullptr;
        std::filesystem::path file_path_;

    public:
        DLib() noexcept = default;
        DLib(std::filesystem::path file_path);
        DLib(const DLib& other) noexcept = delete;
        DLib(DLib&& other) noexcept;
        ~DLib() noexcept;

    public:
        DLib& operator=(const DLib& other) = delete;
        DLib& operator=(DLib&& other) noexcept;

    public:
        [[nodiscard]] inline std::filesystem::path get_path() const noexcept { return file_path_; }

    public:
        template <typename Fn, typename... TArgs>
        auto invoke(const char* func, TArgs&&... args) const -> decltype(auto)
        {
#if defined(CX_PLATFORM_UNIX)
            Fn* instance = (Fn*)dlsym(handle, func);
#elif defined(CX_PLATFORM_WINDOWS)
            Fn* instance = (Fn*)GetProcAddress(handle, func);
#endif
            if (!instance)
                throw DynamicLibraryInvokeException("Failed to invoke '{}' in '{}'", func, file_path_.string());
            return instance(std::forward<TArgs>(args)...);
        }
    };
} // namespace codex::sys
