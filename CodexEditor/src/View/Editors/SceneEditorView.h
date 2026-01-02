#pragma once

#include <Codex.h>
#include <ImGuizmo.h>

namespace codex::editor {
    // Forward declarations.
    class EditorPanel;

    struct SelectedEntityDescriptor
    {
    public:
        Entity entity = Entity::None();

    public:
        inline void Select(Entity entity) noexcept
        {
            if (entity)
            {
                Deselect();
                this->entity = std::move(entity);
            }
        }
        inline void Deselect() noexcept { entity = Entity::None(); }
    };

    enum class GizmoMode
    {
        Translation = ImGuizmo::OPERATION::TRANSLATE,
        Rotation    = ImGuizmo::OPERATION::ROTATE,
        Scale       = ImGuizmo::OPERATION::SCALE
    };

    struct SceneEditorDescriptor
    {
        mem::Ref<Scene>           activeScene;
        mem::Shared<Scene>        editorScene;
        mem::Shared<Scene>        runtimeScene;
        std::filesystem::path     scriptModulePath;
        SelectedEntityDescriptor  selectedEntity;
        f32                       columnWidth  = 140.0f;
        Vector4f                  selectColour = { 0.5f, 1.0f, 0.5f, 1.0f };
        std::filesystem::path     currentProjectPath;
        std::vector<rf::RFScript> scripts;
    };

    // SceneEditorView is technically a layer but it is not part of Codex's layer
    // stack and is actually being proxied by EditorLayer (an actual layer).
    class SceneEditorView : public Layer
    {
        friend class EditorPanel;

    private:
        mem::Box<opengl::FrameBuffer>         m_Framebuffer = nullptr;
        Vector2f                           m_ViewportBounds[2]{};
        Vector2f                           m_ViewportSize{};
        bool                               m_GizmoActive = false;
        GizmoMode                          m_GizmoMode   = GizmoMode::Translation;
        NativeBehaviour*                   m_Script      = nullptr;
        mem::Shared<SceneEditorDescriptor> m_Descriptor  = nullptr;
        std::vector<mem::Box<EditorPanel>> m_ViewPanels;
        bool                               m_ViewportHovered = false;
        bool                               m_ViewportFocused = false;
        mutable gfx::DebugDraw             m_DebugDraw;

    public:
        SceneEditorView() = default;

    public:
        [[nodiscard]] mem::Ref<SceneEditorDescriptor>       GetDescriptor() noexcept { return m_Descriptor; }
        [[nodiscard]] const mem::Ref<SceneEditorDescriptor> GetDescriptor() const noexcept { return m_Descriptor; }

    public:
        void OnAttach() override;
        void OnDetach() override;
        void OnUpdate(const f32 deltaTime) override;
        void OnImGuiRender() override;

    public:
        // Events
        void OnEvent(events::Event& e) override;
        bool OnKeyDown_Event(events::KeyDownEvent& e);
        bool OnMouseDown_Event(events::MouseDownEvent& e);
        bool OnMouseMove_Event(events::MouseMoveEvent& e);
        bool OnMouseScroll_Event(events::MouseScrollEvent& e);

    public:
        i32 CompileProject(const bool wait = false);
        void OnScenePlay() noexcept;
        void OnSceneSimulate() noexcept;
        void OnSceneStop() noexcept;
        void VisualizeColliders() const noexcept;
        void LoadProject(const std::filesystem::path cxproj);
        void UnloadProject();

    public:
        // Stupid method that does one thing and it's panel.OnInit() cause
        // EditorPanel is forward declared here so I can't use it as a complete type.
        void InitializePanel(EditorPanel& panel) const noexcept;
        template <typename T>
            requires(std::is_base_of_v<EditorPanel, T>)
        T& AttachPanel() noexcept
        {
            for (auto& e : m_ViewPanels)
            {
                if (auto* ptr = dynamic_cast<T*>(e.Get()); ptr != nullptr)
                    return *ptr;
            }

            m_ViewPanels.emplace_back(new T(*this));
            InitializePanel(*m_ViewPanels.back());
            return *static_cast<T*>(m_ViewPanels.back().Get());
        }
        template <typename T>
            requires(std::is_base_of_v<EditorPanel, T>)
        void DetachPanel() noexcept
        {
            for (auto it = m_ViewPanels.begin(); it != m_ViewPanels.end(); ++it)
            {
                if (dynamic_cast<T*>(it->Get()) != nullptr)
                {
                    m_ViewPanels.erase(it);
                    break;
                }
            }
        }

    public:
        static void RenderGrid(gfx::DebugDraw& renderer, const scene::EditorCamera& camera,
                               const GridRendererComponent& c) noexcept;
        static void DrawVec3Control(const char* label, Vector3f& values, const f32 columnWidth = 100.0f,
                                    const f32 speed = 1.0f, const f32 resetValue = 0.0f);
        static void DrawVec2Control(const char* label, Vector2f& values, const f32 columnWidth = 100.0,
                                    const f32 speed = 1.0f, const f32 resetValue = 0.0);
    };
} // namespace codex::editor
