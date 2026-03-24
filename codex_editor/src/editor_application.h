#pragma once

#include <codex.h>
#include <filesystem>

namespace codex::editor {
    class EditorApplication : public Engine
    {
    public:
        using Engine::Engine;
        ~EditorApplication() override = default;

    public:
        [[nodiscard]] static std::filesystem::path get_app_data_path() noexcept;
        [[nodiscard]] static std::filesystem::path get_var_app_data_path() noexcept;

    public:
        void on_init() override;

    private:
        static std::filesystem::path s_app_data_path_;
        static std::filesystem::path s_var_app_data_path_;
    };
} // namespace codex::editor
