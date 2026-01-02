#include "ImGuiLayer.h"

#include <Engine/Core/Application.h>

// NOTE: Include GLAD before ImGui OpenGL backend!
#include <glad.h>
#include <imgui_impl_opengl3.h>
#include <imgui_impl_sdl2.h>
#include <imgui_internal.h>
#include <misc/cpp/imgui_stdlib.cpp>

namespace codex::imgui {
    using namespace codex::events;

    ImGuiLayer::ImGuiLayer()
    {
    }

    ImGuiLayer::~ImGuiLayer()
    {
    }

    void ImGuiLayer::OnAttach()
    {
        auto& app           = Application::Get();
        auto& window        = app.GetWindow();
        auto* native_window = app.GetWindow().GetNativeWindow();
        auto* gl_context    = app.GetWindow().GetGlContext();

        IMGUI_CHECKVERSION();
        if (!ImGui::CreateContext())
            cx_throw(CodexException, "Failed to create ImGui context.");
        ImGuiIO& io = ImGui::GetIO();
        (void)io;
        io.DisplaySize = ImVec2((f32)window.GetWidth(), (f32)window.GetHeight()); // Set to your SDL2 window size
        io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;
        io.ConfigFlags |= ImGuiConfigFlags_DockingEnable;
        // NOTE: Viewports are very buggy when docking as of now and they
        // crash on OSX so I am disabling them for now.
        // io.ConfigFlags |= ImGuiConfigFlags_ViewportsEnable;
        m_CurrentContext = ImGui::GetCurrentContext();

        ImGui::StyleColorsDark();

        ImGuiStyle& style = ImGui::GetStyle();
        if (io.ConfigFlags & ImGuiConfigFlags_ViewportsEnable)
        {
            style.WindowRounding              = 0.5f;
            style.Colors[ImGuiCol_WindowBg].w = 1.0f;
        }

        SetDarkThemeColours();

        ImGui_ImplSDL2_InitForOpenGL(native_window, gl_context);
        ImGui_ImplOpenGL3_Init();
    }

    void ImGuiLayer::OnDetach()
    {
        ImGui_ImplOpenGL3_Shutdown();
        ImGui_ImplSDL2_Shutdown();
        ImGui::DestroyContext();
    }

    void ImGuiLayer::OnEvent(Event& event)
    {
        if (m_Blocking)
        {
            auto& io = ImGui::GetIO();
            event.handled |= event.IsInCategory(EventCategoryMouse) & io.WantCaptureMouse;
            event.handled |= event.IsInCategory(EventCategoryKeyboard) & io.WantCaptureKeyboard;
        }
    }

    void ImGuiLayer::Begin()
    {
        ImGui_ImplOpenGL3_NewFrame();
        ImGui_ImplSDL2_NewFrame();
        ImGui::NewFrame();
    }

    void ImGuiLayer::End()
    {
        auto& io       = ImGui::GetIO();
        auto& app      = Application::Get();
        io.DisplaySize = ImVec2((f32)app.GetWindow().GetWidth(), (f32)app.GetWindow().GetHeight());

        ImGui::Render();
        glViewport(0, 0, (i32)io.DisplaySize.x, (i32)io.DisplaySize.y);
        ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());

        if (io.ConfigFlags & ImGuiConfigFlags_ViewportsEnable)
        {
            auto* window     = Application::GetWindow().GetNativeWindow();
            auto  gl_context = SDL_GL_GetCurrentContext();
            ImGui::UpdatePlatformWindows();
            ImGui::RenderPlatformWindowsDefault();
            SDL_GL_MakeCurrent(window, gl_context);
        }
    }

    void ImGuiLayer::SetDarkThemeColours()
    {
    }

    u32 ImGuiLayer::GetActiveWidgetID() const
    {
        return GImGui->ActiveId;
    }
} // namespace codex::imgui
