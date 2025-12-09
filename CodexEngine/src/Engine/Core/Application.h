#ifndef CODEX_CORE_APPLICATION_H
#define CODEX_CORE_APPLICATION_H

#include <sdafx.h>

#include <Engine/Core/Public/Exception.h>
#include <Engine/Core/Window.h>
#include <Engine/Core/LayerStack.h>

int main(int argc, char** argv);

namespace codex {
    // Forward declarations.
    class Input;
    namespace events {
        class WindowResizeEvent;
    } // namespace events

    CX_CUSTOM_EXCEPTION(InvalidPathException, "The path supplied is invalid.");

    struct ApplicationCLIArgs
    {
    public:
        int    count = 0;
        char** args  = nullptr;

    public:
        auto operator[](const usize index) const noexcept -> const char*
        {
            CX_ASSERT(index > count, "Index out of bounds.");
            return args[index];
        }
    };

    struct ApplicationProperties
    {
        std::string           name = "Codex Application";
        std::filesystem::path cwd;
        ApplicationCLIArgs    args;
        WindowProperties      windowProperties;
    };

    class CODEX_API Application
    {
        friend int ::main(int argc, char** argv);
        friend Application* CreateApplication(ApplicationCLIArgs args);

    protected:
        ApplicationProperties m_Properties;
        Window::Box           m_Window    = nullptr;
        bool                  m_Running   = true;
        bool                  m_Minimized = false;
        LayerStack            m_LayerStack;
        f32                   m_DeltaTime  = 0.0f;
        imgui::ImGuiLayer*    m_ImGuiLayer = nullptr;
        Input*                m_Input      = nullptr;
        lgx::Logger*          m_Logger     = nullptr;

    private:
        static Application* s_Instance;

    public:
        Application(ApplicationProperties props);
        Application(const Application& other) = delete;
        virtual ~Application();

    public:
        // FIXME: Throw a NullReferenceException when an Application instance hasn't yet been created.
        [[nodiscard]] static inline auto GetLogger() noexcept -> lgx::Logger& { return *s_Instance->m_Logger; }
        [[nodiscard]] static inline auto GetWindow() noexcept -> Window& { return *s_Instance->m_Window; }
        [[nodiscard]] static inline auto Get() noexcept -> Application& { return *s_Instance; }
        [[nodiscard]] static inline auto GetFps() noexcept -> u32
        {
            return static_cast<u32>(1.0f / s_Instance->m_DeltaTime);
        }
        [[nodiscard]] static inline auto GetFrameCap() noexcept -> u32
        {
            return s_Instance->m_Properties.windowProperties.frameCap;
        }
        [[nodiscard]] static inline auto GetDelta() noexcept -> f32 { return s_Instance->m_DeltaTime; }
        [[nodiscard]] static inline auto GetImGuiLayer() noexcept -> imgui::ImGuiLayer*
        {
            return s_Instance->m_ImGuiLayer;
        }
        [[nodiscard]] static inline auto GetCurrentWorkingDirectory() noexcept -> std::filesystem::path
        {
            return std::filesystem::current_path();
        }
        static inline void SetCurrentWorkingDirectory(const std::filesystem::path& newCwd)
        {
            if (std::filesystem::exists(newCwd) && std::filesystem::is_directory(newCwd))
            {
                std::filesystem::current_path(newCwd);
                s_Instance->m_Properties.cwd = newCwd;
            }
            else
            {
                cx_throw(InvalidPathException, "The path supplied '{}' as the current working directory is invalid.",
                         newCwd.string());
            }
        }

    public:
        auto OnWindowResize_Event(const events::WindowResizeEvent& event) -> bool;

    public:
        virtual auto OnInit() -> void{};

    public:
        auto Run() -> void;
        auto Stop() -> void;
        auto OnEvent(events::Event& e) -> void;
        auto PushLayer(Layer* layer) -> void;
        auto PushOverlay(Layer* overlay) -> void;
    };

    auto CreateApplication(const ApplicationCLIArgs args) -> Application*;
} // namespace codex

#endif // CODEX_CORE_APPLICATION_H
