#pragma once

#include <codex.h>

#include <imgui.h>

#include <ImGuizmo.h>

namespace codex::editor {
    // Forward declarations.
    class EditorPanel;

    struct EditorPanelDeleter
    {
        void operator()(EditorPanel* panel) noexcept;
    };

    struct SelectedEntityDescriptor
    {
    public:
        inline void select(Entity entity) noexcept
        {
            if (entity) {
                deselect();
                this->entity = std::move(entity);
            }
        }
        inline void deselect() noexcept { entity = Entity::none(); }

    public:
        Entity entity = Entity::none();
    };

    enum class GizmoMode
    {
        Translation = ImGuizmo::OPERATION::TRANSLATE,
        Rotation    = ImGuizmo::OPERATION::ROTATE,
        Scale       = ImGuizmo::OPERATION::SCALE,
    };

    enum class CompilationState
    {
        Idle,
        Compiling,
        Succeeded,
        Failed,
    };

    enum class AssetRegistryState
    {
        Idle,
        Scanning,
        Succeeded,
        Failed,
    };

    struct SceneEditorDescriptor
    {
        Ref<Scene>                    active_scene;
        Shared<Scene>                 editor_scene;
        Shared<Scene>                 runtime_scene;
        std::filesystem::path         script_module_path;
        SelectedEntityDescriptor      selected_entity;
        f32                           column_width  = 140.0f;
        Vector4f                      select_colour = { 0.5f, 1.0f, 0.5f, 1.0f };
        std::filesystem::path         current_project_path;
        std::atomic<CompilationState> compilation_state{ CompilationState::Idle };
        std::atomic<bool>             pending_nb_load{ false };
        f64                           compilation_finish_time = 0.0;
        Shared<fs::VirtualFilesystem> vfs                     = Shared<fs::VirtualFilesystem>::make();
        AssetRegistryState            registry_state{ AssetRegistryState::Idle };
        std::mutex                    registry_state_mutex;
        std::condition_variable       registry_state_cv;
        bool                          registry_state_ready{ false };
        UUID                          selected_asset{};

    public:
        void reset() noexcept
        {
            editor_scene = Shared<Scene>::make();
            runtime_scene.reset();
            script_module_path      = std::filesystem::path{};
            selected_entity         = SelectedEntityDescriptor{};
            column_width            = 140.0f;
            select_colour           = { .5f, 1.0f, .5f, 1.0f };
            current_project_path    = std::filesystem::path{};
            compilation_state       = CompilationState::Idle;
            pending_nb_load         = false;
            compilation_finish_time = .0f;

            active_scene = editor_scene;
        }

    public:
        SceneEditorDescriptor& swap(SceneEditorDescriptor& other) noexcept
        {
            active_scene.swap(other.active_scene);
            editor_scene.swap(other.editor_scene);
            runtime_scene.swap(other.runtime_scene);
            std::swap(script_module_path, other.script_module_path);
            std::swap(selected_entity, other.selected_entity);
            std::swap(column_width, other.column_width);
            std::swap(select_colour, other.select_colour);
            std::swap(current_project_path, other.current_project_path);
            std::swap(compilation_finish_time, other.compilation_finish_time);
            vfs.swap(other.vfs);
        }
    };

    // SceneEditorView is technically a layer but it is not part of Codex's layer
    // stack and is actually being proxied by EditorLayer (an actual layer).
    class SceneEditorView : public Layer, Loggable<"Editor::SceneEditorView">
    {
        friend class EditorPanel;

    private:
        Box<opengl::FrameBuffer>                          framebuffer_ = nullptr;
        Vector2f                                          viewport_bounds_[2]{};
        Vector2f                                          viewport_size_{};
        bool                                              gizmo_active_ = false;
        GizmoMode                                         gizmo_mode_   = GizmoMode::Translation;
        NativeBehaviour*                                  script_       = nullptr;
        Shared<SceneEditorDescriptor>                     descriptor_   = nullptr;
        std::vector<Box<EditorPanel, EditorPanelDeleter>> view_panels_;
        bool                                              viewport_hovered_ = false;
        bool                                              viewport_focused_ = false;
        bool                                              is_shutting_down_ = false;
        mutable gfx::DebugDraw                            debug_draw_;

    public:
        SceneEditorView() = default;
        ~SceneEditorView() override { is_shutting_down_ = true; }

    public:
        [[nodiscard]] Ref<SceneEditorDescriptor>       get_descriptor() noexcept { return descriptor_; }
        [[nodiscard]] const Ref<SceneEditorDescriptor> get_descriptor() const noexcept { return descriptor_; }

    public:
        void on_attach() override;
        void on_detach() override;
        void on_update(const f32 deltaTime) override;
        void on_imgui_render() override;

    public:
        // Events
        void on_event(events::Event& e) override;
        bool on_key_down_event(events::KeyDownEvent& e);
        bool on_mouse_down_event(events::MouseDownEvent& e);
        bool on_mouse_move_event(events::MouseMoveEvent& e);
        bool on_mouse_scroll_event(events::MouseScrollEvent& e);

    public:
        void compile_project();
        void on_scene_play() noexcept;
        void on_scene_simulate() noexcept;
        void on_scene_stop() noexcept;
        void visualize_lines() const noexcept;
        void load_project(const std::filesystem::path cxproj);
        void unload_project();

    public:
        // Stupid method that does one thing and it's panel.on_init() cause
        // EditorPanel is forward declared here so I can't use it as a complete type.
        void initialize_panel(EditorPanel& panel) const noexcept;
        template <typename T>
            requires(std::is_base_of_v<EditorPanel, T>)
        T& attach_panel() noexcept
        {
            for (auto& e : view_panels_) {
                if (auto* ptr = dynamic_cast<T*>(e.get()); ptr != nullptr)
                    return *ptr;
            }

            view_panels_.emplace_back(new T(*this));
            initialize_panel(*view_panels_.back());
            return *static_cast<T*>(view_panels_.back().get());
        }
        template <typename T>
            requires(std::is_base_of_v<EditorPanel, T>)
        void detach_panel() noexcept
        {
            for (auto it = view_panels_.begin(); it != view_panels_.end(); ++it) {
                if (dynamic_cast<T*>(it->get()) != nullptr) {
                    view_panels_.erase(it);
                    break;
                }
            }
        }

    public:
        static void render_grid(gfx::DebugDraw& renderer, const scene::EditorCamera& camera,
                                const GridRendererComponent& c) noexcept;
        static void draw_vec3_control(const char* label, Vector3f& values, const f32 column_width = 100.0f,
                                      const f32 speed = 1.0f, const f32 reset_value = 0.0f);
        static void draw_vec2_control(const char* label, Vector2f& values, const f32 column_width = 100.0f,
                                      const f32 speed = 1.0f, const f32 reset_value = 0.0);
        static void draw_asset_path(const AssetPath& path, const f32 column_width = 100.0f);
    };
} // namespace codex::editor
