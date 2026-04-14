#include "im_gui_layer.h"

#include <engine/core/engine.h>

// NOTE: Include GLAD before ImGui OpenGL backend!
#include <glad/glad.h>
#include <imgui_impl_opengl3.h>
#include <imgui_impl_sdl2.h>
#include <imgui_internal.h>
// #include<misc / cpp / imgui_stdlib.cpp>

namespace codex::imgui {
    using namespace codex::events;

    ImGuiLayer::ImGuiLayer()
    {
    }

    ImGuiLayer::~ImGuiLayer()
    {
    }

    void ImGuiLayer::on_attach()
    {
        auto& app           = Engine::get();
        auto& window        = app.window();
        auto* native_window = app.window().native_window();
        auto* gl_context    = app.window().gl_context();

        IMGUI_CHECKVERSION();
        if (!ImGui::CreateContext())
            throw CodexException("Failed to create ImGui context.");
        ImGuiIO& io = ImGui::GetIO();
        (void)io;
        io.DisplaySize = ImVec2((f32)window.width(), (f32)window.height()); // Set to your SDL2 window size
        io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;
        io.ConfigFlags |= ImGuiConfigFlags_DockingEnable;
        // NOTE: Viewports are very buggy when docking as of now and they
        // crash on OSX so I am disabling them for now.
        // io.ConfigFlags |= ImGuiConfigFlags_ViewportsEnable;
        current_context_ = ImGui::GetCurrentContext();

        ImGui::StyleColorsDark();

        ImGuiStyle& style = ImGui::GetStyle();
        if (io.ConfigFlags & ImGuiConfigFlags_ViewportsEnable) {
            style.WindowRounding              = 0.5f;
            style.Colors[ImGuiCol_WindowBg].w = 1.0f;
        }

        set_dark_theme_colours();

        ImGui_ImplSDL2_InitForOpenGL(native_window, gl_context);
        ImGui_ImplOpenGL3_Init();
    }

    void ImGuiLayer::on_detach()
    {
        ImGui_ImplOpenGL3_Shutdown();
        ImGui_ImplSDL2_Shutdown();
        ImGui::DestroyContext();
    }

    void ImGuiLayer::on_event(Event& event)
    {
        if (blocking_) {
            auto& io = ImGui::GetIO();
            event.handled |= event.is_in_category(EventCategoryMouse) & io.WantCaptureMouse;
            event.handled |= event.is_in_category(EventCategoryKeyboard) & io.WantCaptureKeyboard;
        }
    }

    void ImGuiLayer::begin()
    {
        ImGui_ImplOpenGL3_NewFrame();
        ImGui_ImplSDL2_NewFrame();
        ImGui::NewFrame();
    }

    void ImGuiLayer::end()
    {
        auto& io       = ImGui::GetIO();
        auto& app      = Engine::get();
        io.DisplaySize = ImVec2((f32)app.window().width(), (f32)app.window().height());

        ImGui::Render();
        glViewport(0, 0, (i32)io.DisplaySize.x, (i32)io.DisplaySize.y);
        ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());

        if (io.ConfigFlags & ImGuiConfigFlags_ViewportsEnable) {
            auto* window     = Engine::window().native_window();
            auto  gl_context = SDL_GL_GetCurrentContext();
            ImGui::UpdatePlatformWindows();
            ImGui::RenderPlatformWindowsDefault();
            SDL_GL_MakeCurrent(window, gl_context);
        }
    }

    void ImGuiLayer::set_dark_theme_colours()
    {
    }

    u32 ImGuiLayer::active_widget_id() const
    {
        return GImGui->ActiveId;
    }
} // namespace codex::imgui
