#pragma once

#include <sdafx.h>

#include <Engine/Core/Public/Exception.h>

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
        DLibInstance          m_Handle = (DLibInstance) nullptr;
        std::filesystem::path m_FilePath;

    public:
        DLib() noexcept = default;
        DLib(std::filesystem::path filePath);
        DLib(const DLib& other) noexcept = delete;
        DLib(DLib&& other) noexcept;
        ~DLib() noexcept;

    public:
        DLib& operator=(const DLib& other) = delete;
        DLib& operator=(DLib&& other) noexcept;

    public:
        [[nodiscard]] inline std::filesystem::path GetPath() const noexcept { return m_FilePath; }

    public:
        template <typename Fn, typename... TArgs>
        auto Invoke(const char* func, TArgs&&... args) const -> decltype(auto)
        {
#if defined(CX_PLATFORM_UNIX)
            Fn* instance = (Fn*)dlsym(m_Handle, func);
#elif defined(CX_PLATFORM_WINDOWS)
            Fn* instance = (Fn*)GetProcAddress(m_Handle, func);
#endif
            if (!instance)
                cx_throw(DynamicLibraryInvokeException, "Failed to invoke '{}' in '{}'", func, m_FilePath.string());
            return instance(std::forward<TArgs>(args)...);
        }
    };
} // namespace codex::sys
