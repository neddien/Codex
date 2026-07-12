#pragma once

#include <codex.h>
#include <engine/filesystem/vfs.h>
#include <filesystem>

namespace codex::editor {
    class EditorApplication : public Engine, private Loggable<"EditorApplication">
    {
        CX_DEFAULT_LOGGER("EditorApplication")

    public:
        using Engine::Engine;
        ~EditorApplication() override = default;

    public:
        [[nodiscard]] static std::filesystem::path  get_app_data_path() noexcept;
        [[nodiscard]] static std::filesystem::path  get_var_app_data_path() noexcept;
        [[nodiscard]] static fs::VirtualFilesystem& vfs() noexcept;

    public:
        void on_init() override;

    private:
        static std::filesystem::path s_app_data_path_;
        static std::filesystem::path s_var_app_data_path_;
        static fs::VirtualFilesystem s_vfs_;
    };
} // namespace codex::editor
