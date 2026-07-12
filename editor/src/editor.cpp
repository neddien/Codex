#include "editor.h"

#include <ImGuizmo.h>
#include <editor_application.h>
#include <nfd.h>

#include "icons_tabler.h"

namespace codex::editor {
    namespace stdfs = std::filesystem;

    std::optional<scene::EditorCamera> Editor::s_camera_          = std::nullopt;
    ImFont*                            Editor::s_large_icon_font_ = nullptr;
    ImFont*                            Editor::s_xl_icon_font_    = nullptr;
    ImFont*                            Editor::s_console_font_    = nullptr;

    void Editor::on_attach()
    {
        // TODO: Consider splitting these into their own separate init functions.
        // Setup ImGUI.
        // FIXME: Causes a crash when the given font is missing.
        auto&              io            = ImGui::GetIO();
        static std::string ini_file_path = (EditorApplication::get_var_app_data_path() / "imgui.ini").string();
        static std::string font_file_path =
            (EditorApplication::get_app_data_path() / "fonts/roboto/Roboto-Regular.ttf").string();
        static std::string icon_font_path =
            (EditorApplication::get_app_data_path() / "fonts/tabler-icons.ttf").string();

        if (!stdfs::exists(ini_file_path)) {
            try {
                stdfs::copy_file(EditorApplication::get_app_data_path() / "imgui.ini",
                                 EditorApplication::get_var_app_data_path() / "imgui.ini");
            }
            catch (const std::exception& ex) {
                warn("Failed to create variable application data folder! Some data will be lost "
                     "after closing the application.\n\tInner Exception: {}",
                     ex.what());
            }
        }

        const f32 font_size       = 14.0f;
        const f32 icon_large_size = 20.0f;
        io.IniFilename            = ini_file_path.c_str();
        io.FontDefault            = io.Fonts->AddFontFromFileTTF(font_file_path.c_str(), font_size);

        // Load and merge Tabler Icons
        ImFontConfig config;
        config.MergeMode                   = true;
        config.PixelSnapH                  = true;
        static const ImWchar icon_ranges[] = { ICON_MIN_TI, ICON_MAX_TI, 0 };
        io.Fonts->AddFontFromFileTTF(icon_font_path.c_str(), font_size, &config, icon_ranges);

        // Load Large Icon Font (Standalone)
        ImFontConfig large_config;
        large_config.PixelSnapH = true;
        s_large_icon_font_ =
            io.Fonts->AddFontFromFileTTF(icon_font_path.c_str(), icon_large_size, &large_config, icon_ranges);

        // Load XL Icon Font for content browser grid (rasterized at 64px for crisp rendering)
        ImFontConfig xl_config;
        xl_config.PixelSnapH = true;
        s_xl_icon_font_      = io.Fonts->AddFontFromFileTTF(icon_font_path.c_str(), 64.0f, &xl_config, icon_ranges);

        // Monospace font for the console panel.
        static std::string console_font_path =
            (EditorApplication::get_app_data_path() / "fonts/cascadia-code-nfm/CaskaydiaCoveNerdFontMono-Regular.ttf")
                .string();
        if (stdfs::exists(console_font_path))
            s_console_font_ = io.Fonts->AddFontFromFileTTF(console_font_path.c_str(), font_size);
        else
            warn("Console font not found: {}", console_font_path);

        io.Fonts->Build();

        // Init the renderer.
        const auto width  = Engine::window().width();
        const auto height = Engine::window().height();
        gfx::Renderer::init(width, height);
        gfx::BatchRenderer2D::init(EditorApplication::vfs(), "/edit/share/gl_shaders/batch_renderer2d_quad.glsl");
        gfx::DebugDraw::init(EditorApplication::vfs(), "/edit/share/gl_shaders/debug_draw_line2d.glsl");

        NFD_Init();

        s_camera_ = scene::EditorCamera(1920, 1080);

        scene_editor_view_ = Box<SceneEditorView>::make();
        scene_editor_view_->on_attach();
    }

    void Editor::on_detach()
    {
        scene_editor_view_->on_detach();

        s_camera_ = std::nullopt;

        gfx::BatchRenderer2D::dispose();
        gfx::Renderer::dispose();
        gfx::DebugDraw::dispose();

        NFD_Quit();
    }

    void Editor::on_update(const f32 deltaTime)
    {
        scene_editor_view_->on_update(deltaTime);
    }

    void Editor::on_imgui_render()
    {
        scene_editor_view_->on_imgui_render();
    }

    void Editor::on_event(events::Event& e)
    {
        // EventDispatcher d(e);
        // d.Dispatch<KeyDownEvent>(BindEventDelegate(this, &Editor::on_key_down_event));
        scene_editor_view_->on_event(e);
    }

    bool Editor::on_key_down_event(events::KeyDownEvent& e)
    {
        /*
        switch (e.GetKey())
        {
            case Key::Num1: m_GizmoMode = GizmoMode::Translation; return true;
            case Key::Num2: m_GizmoMode = GizmoMode::Rotation; return true;
            case Key::Num3: m_GizmoMode = GizmoMode::Scale; return true;
            default: break;
        }
        */
        return false;
    }
} // namespace codex::editor
