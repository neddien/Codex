#include "public/file_system.h"

namespace codex::fs {
    namespace fs = std::filesystem;

    std::filesystem::path get_special_folder(const SpecialFolder folder) noexcept
    {
#if defined(CX_PLATFORM_UNIX)
        static const char* home_dir = std::getenv("HOME");
        if (!home_dir) {
            struct passwd* pw = getpwuid(getuid());
            if (pw)
                home_dir = pw->pw_dir;
            else
                return "";
        }

        switch (folder) {
            using enum SpecialFolder;

            case ApplicationFiles: return "/usr/local/share";
            case ApplicationData: return "/var/local";
            case UserApplicationData: return fs::path(home_dir) / ".config/";
            case Desktop: return fs::path(home_dir) / "/Desktop";
            case Fonts: return "/usr/share/fonts";
            case Temporary: return "/tmp";
            case User: return { home_dir };
            default: return {};
        }
#elif defined(CX_PLATFORM_OSX)

        // bruh...

#elif defined(CX_PLATFORM_WINDOWS)
        static char* home_dir = std::getenv("USERPROFILE");

        switch (folder) {
            using enum SpecialFolder;

            case ApplicationFiles: return { std::getenv("ProgramFiles") };
            case ApplicationData: return { std::getenv("APPDATA") };
            case UserApplicationData: return { std::getenv("LOCALAPPDATA") };
            case Desktop: return fs::path(home_dir) / "/Desktop";
            case Fonts: return { "C:/Windows/Fonts" };
            case Temporary: return { std::getenv("temp") };
            case User: return { home_dir };
            default: return {};
        }
#endif
        return "";
    }

    std::vector<std::filesystem::path> get_all_files_with_extensions(
        const std::filesystem::path& directory, const std::initializer_list<std::string_view> extensions) noexcept
    {
        std::vector<fs::path> matching_files;

        for (const auto& entry : fs::recursive_directory_iterator(directory)) {
            if (entry.is_regular_file()) {
                const auto& path = entry.path();
                for (const auto& ext : extensions) {
                    if (ext == "*" || path.extension() == ext) {
                        matching_files.push_back(path);
                        break;
                    }
                }
            }
        }

        return matching_files;
    }

    std::filesystem::path get_temp_file(const std::string_view prefix, const std::string_view ext) noexcept
    {
        static std::mt19937_64            rng{ std::random_device{}() };
        static std::mutex                 rng_mutex;
        static constexpr std::string_view hex = "0123456789abcdef";

        // 16 hex chars = 64 bits of randomness — collision probability negligible
        std::string name;
        name.reserve(prefix.size() + 16 + ext.size());
        name += prefix;
        {
            std::lock_guard lock{ rng_mutex };
            const u64       r = rng();
            for (int i = 0; i < 16; ++i)
                name += hex[(r >> (i * 4)) & 0xFu];
        }
        if (!ext.empty()) {
            if (ext[0] != '.')
                name += '.';
            name += ext;
        }

        return fs::temp_directory_path() / name;
    }

    [[nodiscard]] std::string normalize(const std::string& path) noexcept
    {
        auto parts = util::str::split(path, '/');
        parts.erase(std::remove_if(parts.begin(), parts.end(), [](const auto& s) { return s.empty(); }), parts.end());
        return util::str::join(parts.begin(), parts.end(), '/');
    }
} // namespace codex::fs
