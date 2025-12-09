#pragma once

#include <sdafx.h>

namespace codex::fs {
    enum class SpecialFolder
    {
        ApplicationFiles,
        ApplicationData,
        UserApplicationData,
        Desktop,
        Fonts,
        Temporary,
        User
    };

    [[nodiscard]] CODEX_API std::filesystem::path GetSpecialFolder(const SpecialFolder folder) noexcept;
    [[nodiscard]] CODEX_API std::vector<std::filesystem::path> GetAllFilesWithExtensions(
        const std::filesystem::path& directory, const std::initializer_list<std::string_view> extensions) noexcept;
    [[nodiscard]] inline std::vector<std::filesystem::path> GetAllFilesInDirectory(
        const std::filesystem::path& directory) noexcept
    {
        return GetAllFilesWithExtensions(directory, { "*" });
    }
} // namespace codex::fs
