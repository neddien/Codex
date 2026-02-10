#include "Editor.h"

#include <EditorApplication.h>
#include <ImGuizmo.h>
#include <nfd.h>

namespace codex::editor {
    namespace stdfs = std::filesystem;

    std::optional<scene::EditorCamera> Editor::s_Camera = std::nullopt;

    void Editor::OnAttach()
    {
        // TODO: Consider splitting these into their own separate init functions.
        // Setup ImGUI.
        auto&              io            = ImGui::GetIO();
        static std::string ini_file_path = (EditorApplication::GetVarAppDataPath() / "imgui.ini").string();
        static std::string font_file_path =
            (EditorApplication::GetAppDataPath() / "Fonts/roboto/Roboto-Regular.ttf").string();

        if (!stdfs::exists(ini_file_path))
        {
            try
            {
                stdfs::copy_file(EditorApplication::GetAppDataPath() / "imgui.ini",
                                 EditorApplication::GetVarAppDataPath() / "imgui.ini");
            }
            catch (const std::exception& ex)
            {
                lgx::Get("editor").Log(lgx::Warn,
                                       "Failed to create variable application data folder! Some data will be lost "
                             "after closing the application.\n\tInner Exception: {}",
                             ex.what());
            }
        }

        const f32 font_size = 14.0f;
        io.IniFilename      = ini_file_path.c_str();
        io.FontDefault      = io.Fonts->AddFontFromFileTTF(font_file_path.c_str(), font_size);

        // Init the renderer.
        const auto width  = EditorApplication::GetWindow().GetWidth();
        const auto height = EditorApplication::GetWindow().GetHeight();
        gfx::Renderer::Init(width, height);
        gfx::BatchRenderer2D::Init(EditorApplication::GetAppDataPath() / "GL Shaders/BatchRenderer2D_Quad.glsl");
        gfx::DebugDraw::Init(EditorApplication::GetAppDataPath() / "GL Shaders/DebugDraw_Line2D.glsl");

        NFD_Init();

        s_Camera = scene::EditorCamera(1920, 1080);

        m_SceneEditorView = mem::Box<SceneEditorView>::New();
        m_SceneEditorView->OnAttach();
    }

    void Editor::OnDetach()
    {
        m_SceneEditorView->OnDetach();

        s_Camera = std::nullopt;

        gfx::BatchRenderer2D::Dispose();
        gfx::Renderer::Dispose();
        gfx::DebugDraw::Dispose();

        NFD_Quit();
    }

    void Editor::OnUpdate(const f32 deltaTime)
    {
        m_SceneEditorView->OnUpdate(deltaTime);
    }

    void Editor::OnImGuiRender()
    {
        m_SceneEditorView->OnImGuiRender();
    }

    void Editor::OnEvent(events::Event& e)
    {
        // EventDispatcher d(e);
        // d.Dispatch<KeyDownEvent>(BindEventDelegate(this, &Editor::OnKeyDown_Event));
        m_SceneEditorView->OnEvent(e);
    }

    bool Editor::OnKeyDown_Event(events::KeyDownEvent& e)
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
