#pragma once

namespace codex::fs {
    constexpr auto kMaxPathLength = 4096;

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

    [[nodiscard]] CODEX_API std::string normalize(const std::string& path) noexcept;

    // Returns a unique path inside the OS temp directory. The file is NOT created.
    // prefix: prepended to the random suffix (default "cx_")
    // ext:    file extension including the dot e.g. ".pak" (default: none)
    [[nodiscard]] CODEX_API std::filesystem::path get_temp_file(std::string_view prefix = "cx_",
                                                                std::string_view ext    = "") noexcept;
} // namespace codex::fs
