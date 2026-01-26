#include "EditorApplication.h"

#include <filesystem>

#include "Editor.h"
// #include "TitleBar.h"

#include <ConsoleMan.h>

namespace codex::editor {
    namespace stdfs = std::filesystem;

    stdfs::path EditorApplication::m_ApplicationDataPath{};
    stdfs::path EditorApplication::m_VariableApplicationDataPath{};

    stdfs::path EditorApplication::GetAppDataPath() noexcept
    {
        return m_ApplicationDataPath;
    }

    stdfs::path EditorApplication::GetVarAppDataPath() noexcept
    {
        return m_VariableApplicationDataPath;
    }

    void EditorApplication::OnInit()
    {
        // Create the editor logger.
        auto& logger = lgx::Get("editor");
        logger.SetDefaultPrefix("CEditor");
        logger.SetDefaultInfoStyle(fmt::fg(fmt::color::lime_green));
        logger.SetDefaultWarnStyle(fmt::fg(fmt::color::yellow));
        logger.SetDefaultErrorStyle(fmt::fg(fmt::color::red) | fmt::emphasis::italic);
        logger.SetDefaultFatalStyle(fmt::fg(fmt::color::dark_red) | fmt::emphasis::italic);

#ifdef CX_PLATFORM_UNIX
        m_ApplicationDataPath = stdfs::path(CE_INSTALL_DIR) / stdfs::path("share/CEditor");
#elif defined(CX_PLATFORM_WINDOWS)
        m_ApplicationDataPath = stdfs::path(CE_INSTALL_DIR) / stdfs::path("bin");
#endif
        m_VariableApplicationDataPath =
            stdfs::path(fs::GetSpecialFolder(fs::SpecialFolder::UserApplicationData)) / stdfs::path("CEditor/");

        stdfs::current_path(m_ApplicationDataPath);

        if (!stdfs::exists(m_VariableApplicationDataPath))
        {
            try
            {
                stdfs::create_directory(m_VariableApplicationDataPath);
            }
            catch (const std::exception& ex)
            {
                lgx::Get("editor").Log(lgx::Warn, "Failed to create application data folder!");
            }
        }

        logger.Log(lgx::Info, "Application data path: '{}'", m_ApplicationDataPath.string());
        logger.Log(lgx::Info, "Variable application data path: '{}'", m_VariableApplicationDataPath.string());

        PushLayer(new Editor);
        PushLayer(new ConsoleMan);
    }
} // namespace codex::editor
