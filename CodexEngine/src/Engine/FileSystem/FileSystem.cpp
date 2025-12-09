#include "Public/FileSystem.h"

namespace codex::fs {
    namespace fs = std::filesystem;

    std::filesystem::path GetSpecialFolder(const SpecialFolder folder) noexcept
    {
#if defined(CX_PLATFORM_UNIX)
        static const char* home_dir = std::getenv("HOME");
        if (!home_dir)
        {
            struct passwd* pw = getpwuid(getuid());
            if (pw)
                home_dir = pw->pw_dir;
            else
                return "";
        }

        switch (folder)
        {
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

        switch (folder)
        {
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

    std::vector<std::filesystem::path> GetAllFilesWithExtensions(
        const std::filesystem::path& directory, const std::initializer_list<std::string_view> extensions) noexcept
    {
        std::vector<fs::path> matching_files;

        for (const auto& entry : fs::recursive_directory_iterator(directory))
        {
            if (entry.is_regular_file())
            {
                const auto& path = entry.path();
                for (const auto& ext : extensions)
                {
                    if (ext == "*" || path.extension() == ext)
                    {
                        matching_files.push_back(path);
                        break;
                    }
                }
            }
        }

        return matching_files;
    }
} // namespace codex::fs
