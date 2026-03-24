#pragma once

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

    [[nodiscard]] CODEX_API std::filesystem::path get_special_folder(const SpecialFolder folder) noexcept;
    [[nodiscard]] CODEX_API std::vector<std::filesystem::path> get_all_files_with_extensions(
        const std::filesystem::path& directory, const std::initializer_list<std::string_view> extensions) noexcept;
    [[nodiscard]] inline std::vector<std::filesystem::path> get_all_files_in_directory(
        const std::filesystem::path& directory) noexcept
    {
        return get_all_files_with_extensions(directory, { "*" });
    }
} // namespace codex::fs
