#include "editor_application.h"

#include <filesystem>

#include "editor.h"
// #include "title_bar.h"

#include <console_man.h>

namespace codex::editor {
    namespace stdfs = std::filesystem;

    stdfs::path           EditorApplication::s_app_data_path_{};
    stdfs::path           EditorApplication::s_var_app_data_path_{};
    fs::VirtualFilesystem EditorApplication::s_vfs_{};

    stdfs::path EditorApplication::get_app_data_path() noexcept
    {
        return s_app_data_path_;
    }

    stdfs::path EditorApplication::get_var_app_data_path() noexcept
    {
        return s_var_app_data_path_;
    }

    fs::VirtualFilesystem& EditorApplication::vfs() noexcept
    {
        return s_vfs_;
    }

    void EditorApplication::on_init()
    {
#ifdef CX_PLATFORM_UNIX
        s_app_data_path_ = stdfs::path(CE_INSTALL_DIR) / stdfs::path("share/CEditor");
#elif defined(CX_PLATFORM_WINDOWS)
        s_app_data_path_ = stdfs::path(CE_INSTALL_DIR) / stdfs::path("bin");
#endif
        s_var_app_data_path_ =
            fs::get_special_folder(fs::SpecialFolder::UserApplicationData) / stdfs::path("CEditor/");

        stdfs::current_path(s_app_data_path_);

        if (!stdfs::exists(s_var_app_data_path_)) {
            try {
                stdfs::create_directory(s_var_app_data_path_);
            }
            catch (const std::exception& ex) {
                warn("Failed to create application data folder!");
            }
        }

        s_vfs_.mount(Shared<fs::DiskMount>::make(s_app_data_path_, 0), "/edit/share", true);
        s_vfs_.mount(Shared<fs::DiskMount>::make(CE_INSTALL_DIR, 0), "/edit/install", true);
        s_vfs_.mount(Shared<fs::DiskMount>::make(s_var_app_data_path_, 0), "/edit/var", true);
        s_vfs_.mkdir("/edit/tmp");

        info("Application data path: '{}'", s_app_data_path_.string());
        info("Variable application data path: '{}'", s_var_app_data_path_.string());

        push_layer(new Editor);
        push_layer(new ConsoleMan);
    }
} // namespace codex::editor
