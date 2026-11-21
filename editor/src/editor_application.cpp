#include "editor_application.h"

#include <filesystem>

#include "editor.h"
#include "engine/core/public/exception.h"
// #include "title_bar.h"

#include <console_man.h>

namespace codex::editor {
    namespace stdfs = std::filesystem;

    stdfs::path           EditorApplication::s_app_data_path_{};
    stdfs::path           EditorApplication::s_var_app_data_path_{};
    stdfs::path           EditorApplication::s_install_path_;
    fs::VirtualFilesystem EditorApplication::s_vfs_{};

    stdfs::path EditorApplication::get_app_data_path() noexcept
    {
        return s_app_data_path_;
    }

    stdfs::path EditorApplication::get_var_app_data_path() noexcept
    {
        return s_var_app_data_path_;
    }

    stdfs::path EditorApplication::get_install_path() noexcept
    {
        return sys::module_path().parent_path().parent_path();
    }

    fs::VirtualFilesystem& EditorApplication::vfs() noexcept
    {
        return s_vfs_;
    }

    void EditorApplication::on_init()
    {
        s_install_path_ = get_install_path();

        info("Application install path: {}", s_install_path_.generic_string());
#ifdef CX_PLATFORM_UNIX
        s_app_data_path_ = stdfs::path(s_install_path_) / stdfs::path("share/CEditor");
#elif defined(CX_PLATFORM_WINDOWS)
        s_app_data_path_ = stdfs::path(s_install_path_) / stdfs::path("bin/data");
#endif
        s_var_app_data_path_ = fs::get_special_folder(fs::SpecialFolder::UserApplicationData) / stdfs::path("CEditor/");

        stdfs::current_path(s_app_data_path_);

        if (!stdfs::exists(s_var_app_data_path_)) {
            try {
                stdfs::create_directory(s_var_app_data_path_);
            }
            catch (const std::exception& ex) {
                warn("Failed to create application data folder!");
            }
        }

        if (s_install_path_.empty() || !stdfs::exists(s_app_data_path_ / "gl_shaders")) {
            log(Error, "{}: no such file or directory",
                (stdfs::path(s_app_data_path_) / "gl_shaders").generic_string());
            throw InvalidOperationException("Not running from an install");
        }

        s_vfs_.mount(Shared<fs::DiskMount>::make(s_app_data_path_, 0), "/edit/share", true);
        s_vfs_.mount(Shared<fs::DiskMount>::make(s_install_path_, 0), "/edit/install", true);
        s_vfs_.mount(Shared<fs::DiskMount>::make(s_var_app_data_path_, 0), "/edit/var", true);
        s_vfs_.mkdir("/edit/tmp");

        info("Application data path: '{}'", s_app_data_path_.string());
        info("Variable application data path: '{}'", s_var_app_data_path_.string());

        push_layer(new Editor);
        push_layer(new ConsoleMan);
    }
} // namespace codex::editor
