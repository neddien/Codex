#include "SceneEditorView.h"

#include <ConsoleMan.h>
#include <Editor.h>
#include <EditorApplication.h>
#include <tinyfiledialogs.h>

#include "Panels/ProjectSettingsView.h"
#include "Panels/PropertiesView.h"
#include "Panels/SceneHierarchyView.h"
#include "Panels/ToolbarView.h"

#include <imgui.h>
#include <imgui_internal.h>

namespace codex::editor {
    namespace stdfs = std::filesystem;

    void SceneEditorView::OnAttach()
    {
        // TODO: Fix Process stdout not working on Linux.
        /*
        sys::ProcessInfo inf;
        inf.command        = "ls";
        inf.redirectStdOut = true;
        inf.redirectStdErr = true;
        inf.onExit = [](i32) { ConsoleMan::AppendMessage("Exited."); };

        const auto ldb = [](const char* buffer, usize len) { ConsoleMan::AppendMessage(std::string(buffer, len)); };

        auto handle                     = sys::Process::New(inf);
        handle->Event_OnOutDataReceived = ldb;
        handle->Event_OnOutDataReceived = ldb;
        handle->Event_OnErrDataReceived = ldb;

        ConsoleMan::AppendMessage("Started.");
        handle->Launch();
        */

        m_Descriptor = mem::Shared<SceneEditorDescriptor>::From(
            new SceneEditorDescriptor{ .editorScene = mem::Shared<Scene>::New() });
        m_Descriptor->activeScene = m_Descriptor->editorScene;

        // Panels
        this->AttachPanel<SceneHierarchyView>();
        this->AttachPanel<PropertiesView>();
        this->AttachPanel<ToolbarView>();

        opengl::FrameBufferProperties props;
        props.attachments = { { .format = opengl::TextureFormat::RGBA8 },
                              { .format = opengl::TextureFormat::RedInt32 } };

        // TODO: This is the scene render resolution so you should not hard code this.
        props.width   = 1920;
        props.height  = 1080;
        m_Framebuffer = mem::Box<opengl::FrameBuffer>::New(props);

        // EditorLayer::GetCamera().SetProjectionType(scene::Camera::ProjectionType::Perspective);

        // TODO: We should z-index using the depth buffer in the future so that we can also have 3d elements
        // instead of disabling the depth buffer and z-index'ing by sorting our render batches.
        // glEnable(GL_DEPTH_TEST);
        // glDepthFunc(GL_LESS);
    }

    void SceneEditorView::OnDetach()
    {
        auto& d = m_Descriptor;
        d->activeScene.Reset();
    }

    void SceneEditorView::OnUpdate(const f32 deltaTime)
    {
        auto& d     = m_Descriptor;
        auto  scene = d->activeScene.Lock();

        m_Framebuffer->Bind();

        // Viewport resize
        {
            CX_DEBUG_PROFILE_SCOPE("OnUpdate::ViewportResize")

            auto&       camera          = Editor::GetViewportCamera();
            static auto prev_viewport   = m_ViewportSize;
            static auto prev_camera_pan = camera.GetPan();
            if (m_ViewportSize != prev_viewport || camera.GetPan() != prev_camera_pan)
            {
                camera.SetWidth(m_ViewportSize.x);
                camera.SetHeight(m_ViewportSize.y);
                prev_viewport   = m_ViewportSize;
                prev_camera_pan = camera.GetPan();
                m_Framebuffer->Resize((u32)m_ViewportSize.x, (u32)m_ViewportSize.y);
            }
        }

        gfx::Renderer::SetClearColour(0.2f, 0.2f, 0.2f, 1.0f);
        gfx::Renderer::Clear();

        m_DebugDraw.Begin(Editor::GetViewportCamera());

        switch (scene->GetState())
        {
            case Scene::State::Edit: {
                CX_DEBUG_PROFILE_SCOPE("OnUpdate::OnEditorUpdate")

                if (d->selectedEntity.entity)
                {
                    if (d->selectedEntity.entity.HasComponent<GridRendererComponent>())
                    {
                        RenderGrid(m_DebugDraw, Editor::GetViewportCamera(),
                                   d->selectedEntity.entity.GetComponent<GridRendererComponent>());
                    }
                }

                VisualizeColliders();

                scene->OnEditorUpdate(deltaTime, Editor::GetViewportCamera());
                break;
            }
            case Scene::State::Play: {
                CX_DEBUG_PROFILE_SCOPE("OnUpdate::OnRuntimeUpdate")
                scene->OnRuntimeUpdate(deltaTime);
                break;
            }
            case Scene::State::Simulate: {
                CX_DEBUG_PROFILE_SCOPE("OnUpdate::OnSimulationUpdate")
                VisualizeColliders();
                scene->OnSimulationUpdate(deltaTime, Editor::GetViewportCamera());
                break;
            }
        }

        auto [mx, my] = ImGui::GetMousePos();
        mx -= m_ViewportBounds[0].x;
        my -= m_ViewportBounds[0].y;
        const Vector2f viewport_size = m_ViewportBounds[1] - m_ViewportBounds[0];
        const i32      mouse_x       = (i32)mx;
        const i32      mouse_y       = (i32)my;

        if (Input::IsMouseDown(Mouse::LeftMouse) && mouse_x >= 0 && mouse_y >= 0 && mouse_x <= (i32)viewport_size.x &&
            mouse_y <= (i32)viewport_size.y && !m_GizmoActive)
        {
            if (d->activeScene.Lock()->GetState() != Scene::State::Play && !m_GizmoActive &&
                (!d->selectedEntity.entity || !d->selectedEntity.entity.HasComponent<TilemapComponent>()))
            {
                Vector2f scale = { m_Framebuffer->GetProperties().width / viewport_size.x,
                                   m_Framebuffer->GetProperties().height / viewport_size.y };
                Vector2f pos   = { mouse_x, viewport_size.y - mouse_y };
                pos *= scale;
                pos          = glm::round(pos);
                const i32 id = m_Framebuffer->ReadPixel(1, (i32)pos.x, (i32)pos.y);
                auto      e  = Entity((entt::entity)id, d->activeScene.Get());
                if (e)
                    d->selectedEntity.Select(e);
            }
        }

        m_DebugDraw.End();

        m_Framebuffer->Unbind();

        // Update our panels.
        for (auto& panel : m_ViewPanels)
            panel->OnPreUpdate(deltaTime);
    }

    void SceneEditorView::OnImGuiRender()
    {
        CX_DEBUG_PROFILE_SCOPE("OnImGuiRender")

        auto& d  = m_Descriptor;
        auto& io = ImGui::GetIO();

        // Dockspace, viewport and gizmo.
        {
            static auto dockspace_open  = true;
            static auto dockspace_flags = ImGuiDockNodeFlags_None;
            static auto fullscreen      = true;
            auto        window_flags    = ImGuiWindowFlags_MenuBar | ImGuiWindowFlags_NoDocking;
            if (fullscreen)
            {
                auto* viewport = ImGui::GetMainViewport();
                ImGui::SetNextWindowPos(viewport->Pos);
                ImGui::SetNextWindowSize(viewport->Size);
                ImGui::SetNextWindowViewport(viewport->ID);
                ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 0.0f);
                ImGui::PushStyleVar(ImGuiStyleVar_WindowBorderSize, 0.0f);
                window_flags |= ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoResize |
                                ImGuiWindowFlags_NoMove;
                window_flags |= ImGuiWindowFlags_NoBringToFrontOnFocus | ImGuiWindowFlags_NoNavFocus;
            }
            if (dockspace_flags & ImGuiDockNodeFlags_PassthruCentralNode)
                window_flags |= ImGuiWindowFlags_NoBackground;

            ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0.0f, 0.0f));
            ImGui::Begin("Dockspace Demo", &dockspace_open, window_flags);
            ImGui::PopStyleVar();
            if (fullscreen)
                ImGui::PopStyleVar(2);

            if (io.ConfigFlags & ImGuiConfigFlags_DockingEnable)
            {
                auto dockspace_id = ImGui::GetID("MyDockSpace");
                ImGui::DockSpace(dockspace_id, ImVec2(0.0f, 0.0f), dockspace_flags);
            }

            ImGui::SetNextWindowPos(ImVec2(0, 20), ImGuiCond_Always);

            // ImGuizmo
            ImGuizmo::BeginFrame();

            // Enable docking on the main window.
            ImGui::DockSpaceOverViewport(0, ImGui::GetMainViewport());

            static bool show_demo_window = true;
            ImGui::ShowDemoWindow(&show_demo_window);
        }

        // Profiler window.
        {
            ImGui::Begin("Profiler");
            for (const auto& e : dbg::Profiler::GetProfilers())
            {
                const auto info = e.second.GetInfo();
                const auto dur  = e.second.ElapsedAs<std::milli, f32>().count();
                ImGui::Text("%s: %fms", info.name.c_str(), dur);
            }
            ImGui::End();
        }

        // File menu
        {
            if (ImGui::BeginMainMenuBar())
            {
                if (ImGui::BeginMenu("File"))
                {
                    // Add File menu items here
                    if (ImGui::MenuItem("Create new project", "Ctrl+N")) {}
                    if (ImGui::MenuItem("Open", "Ctrl+O"))
                    {
                        const char* filters[]{ "*.cxproj" };
                        const char* file = tinyfd_openFileDialog("Load a Project.", nullptr, 1, filters, nullptr, 0);
                        if (file)
                        {
                            LoadProject(stdfs::path(file));
                        }
                    }
                    if (ImGui::MenuItem("Compile project"))
                    {
                        CompileProject();
                    }
                    if (ImGui::MenuItem("Clear build files"))
                    {
                        auto& d = m_Descriptor;
                        // d->activeScene.Lock()->UnloadScriptModule();

                        /*
                        const auto files =
                            fs::GetAllFilesWithExtensions(d->currentProjectPath / "Assets/", { ".h", ".hpp", ".hh" });
                        std::vector<rf::RFScript> rf_files;
                        rf_files.reserve(files.size());

                        const auto output_path = stdfs::absolute(d->currentProjectPath / "int/");
                        for (const auto& f : files)
                            rf_files.emplace_back(f).EmitMetadata(output_path);
                        rf::RFScript::EmitBaseClass(output_path, rf_files);
                        */

                        sys::ProcessInfo p_info;

#ifdef CX_PLATFORM_WINDOWS
                        p_info.command = "cmake --preset=windows-llvm-any-debug --clear";
#elif defined(CX_PLATFORM_LINUX)
                        p_info.command = "./build.py --preset=linux-any-debug --clear";
#elif defined(CX_PLATFORM_OSX)
                        p_info.command = "./build.py --preset=osx-any-debug --clear";
#endif
                        p_info.onExit = [this](i32 exitCode)
                        {
                            auto& d = m_Descriptor;
                            // TODO: Scene should also be thread safe since this callback is being called from a
                            // different thread.
                            // Scene::LoadScriptModule(d->scriptModulePath); // TODO: COME BACK
                            ConsoleMan::AppendMessage("-- Clear finished.");
                        };
                        p_info.redirectStdOut = true;
                        p_info.redirectStdErr = true;

                        const auto redirector = [](const char* buffer, usize len)
                        { ConsoleMan::AppendMessage(std::string(buffer, len)); };

                        ConsoleMan::AppendMessage("-- Clear started.");
                        auto proc                     = sys::Process::New(std::move(p_info));
                        proc->Event_OnOutDataReceived = redirector;
                        proc->Event_OnErrDataReceived = redirector;
                        proc->Launch();
                    }
                    if (ImGui::MenuItem("Save", "Ctrl+S"))
                    {
                        // Handle the "Save" action
                        static const char* save_dir = nullptr;
                        if (!save_dir)
                        {
                            const char* filter_patterns[] = { "*.cxproj" };
                            save_dir =
                                tinyfd_saveFileDialog("Save Project", "default.cxproj", 1, filter_patterns, NULL);
                            if (save_dir)
                            {
                                // TODO: project->Save(path);
                                d->selectedEntity.Deselect();
                                SerializationManager::SaveToFile(*d->activeScene.Lock(), save_dir);
                            }
                        }
                        else
                        {
                            d->selectedEntity.Deselect();
                            SerializationManager::SaveToFile(*d->activeScene.Lock(), save_dir);
                        }
                    }
                    if (ImGui::MenuItem("Exit", "Alt+F4"))
                    {
                        Application::Get().Stop();
                    }

                    ImGui::EndMenu();
                }
                if (ImGui::BeginMenu("Edit"))
                {
                    if (ImGui::MenuItem("Project Settings"))
                    {
                        this->AttachPanel<ProjectSettingsView>();
                    }
                    ImGui::EndMenu();
                }
                ImGui::EndMainMenuBar();
            }
        }

        // Engine viewport
        {
            auto& camera = Editor::GetViewportCamera();

            ImGui::Begin("Viewport");
            const auto viewport_min_region = ImGui::GetWindowContentRegionMin();
            const auto viewport_max_region = ImGui::GetWindowContentRegionMax();
            const auto viewport_offset     = ImGui::GetWindowPos();
            m_ViewportBounds[0]            = { viewport_min_region.x + viewport_offset.x,
                                               viewport_min_region.y + viewport_offset.y };
            m_ViewportBounds[1]            = { viewport_max_region.x + viewport_offset.x,
                                               viewport_max_region.y + viewport_offset.y };

            auto current_viewport_window_size = ImGui::GetContentRegionAvail();
            m_ViewportSize = Vector2f{ current_viewport_window_size.x, current_viewport_window_size.y };
            ImGui::Image(reinterpret_cast<ImTextureID>(m_Framebuffer->GetColourAttachmentIdAt(0)),
                         current_viewport_window_size, { 0, 1 }, { 1, 0 });

            m_ViewportFocused = ImGui::IsWindowFocused();
            m_ViewportHovered = ImGui::IsWindowHovered();

            // Application::Get().GetImGuiLayer()->BlockEvents(!m_ViewportFocused);

            // Guizmo
            {
                if (d->selectedEntity.entity)
                {
                    ImGuizmo::SetOrthographic(true);
                    ImGuizmo::SetDrawlist();

                    ImGuizmo::SetRect(m_ViewportBounds[0].x, m_ViewportBounds[0].y,
                                      m_ViewportBounds[1].x - m_ViewportBounds[0].x,
                                      m_ViewportBounds[1].y - m_ViewportBounds[0].y);

                    // Camera
                    auto proj_mat = camera.GetProjectionMatrix();
                    auto view_mat = camera.GetViewMatrix();

                    auto& tc        = d->selectedEntity.entity.GetComponent<TransformComponent>();
                    auto  transform = tc.ToMatrix();

                    ImGuizmo::Manipulate(glm::value_ptr(view_mat), glm::value_ptr(proj_mat),
                                         (ImGuizmo::OPERATION)m_GizmoMode, ImGuizmo::MODE::LOCAL,
                                         glm::value_ptr(transform));

                    m_GizmoActive = ImGuizmo::IsOver();

                    if (m_GizmoActive && ImGuizmo::IsUsing())
                    {
                        Vector3f rotation;
                        codex::math::TransformDecompose(transform, tc.position, rotation, tc.scale);
                        tc.rotation += glm::degrees(rotation) - tc.rotation;
                    }
                }
            }

            ImGui::End();
        }

        // Render info
        {
            ImGui::Begin("Render Info");
            ImGui::Text("Renderer information");
            ImGui::Text("FPS: %u", Application::GetFps());
            ImGui::Text("Delta time: %f", Application::GetDelta());
            ImGui::Text("Batch count: %zu", gfx::BatchRenderer2D::GeBatchCount());
            ImGui::Text("Total quad count: %zu", gfx::BatchRenderer2D::GetQuadCount());
            ImGui::End();
        }

        auto block_events = !m_ViewportFocused;

        // Render our panels.
        {
            CX_DEBUG_PROFILE_SCOPE("PanelRender")

            auto it = m_ViewPanels.begin();
            while (it != m_ViewPanels.end())
            {
                auto& panel = *it;
                if (!panel->m_Show)
                    it = m_ViewPanels.erase(it);
                else
                {
                    block_events = block_events && panel->GetImGuiBlockEvents();
                    panel->OnPreImGuiRender();
                    ++it;
                }
            }
        }

        Application::Get().GetImGuiLayer()->BlockEvents(block_events);

        ImGui::End();
    }

    void SceneEditorView::OnEvent(events::Event& e)
    {
        // Child panel events should be handled first because of painter's rule.
        for (auto& panel : m_ViewPanels)
        {
            if (e.handled)
                return;
            panel->OnPreEvent(e);
        }

        events::EventDispatcher d{ e };
        d.Dispatch<events::KeyDownEvent>(BindEventDelegate(this, &SceneEditorView::OnKeyDown_Event));
        d.Dispatch<events::MouseDownEvent>(BindEventDelegate(this, &SceneEditorView::OnMouseDown_Event));
        d.Dispatch<events::MouseMoveEvent>(BindEventDelegate(this, &SceneEditorView::OnMouseMove_Event));
        d.Dispatch<events::MouseScrollEvent>(BindEventDelegate(this, &SceneEditorView::OnMouseScroll_Event));
    }

    bool SceneEditorView::OnKeyDown_Event(events::KeyDownEvent& e)
    {
        switch (e.GetKey())
        {
            using enum codex::Key;
            using enum codex::editor::GizmoMode;

            case Num1: m_GizmoMode = Translation; return true;
            case Num2: m_GizmoMode = Rotation; return true;
            case Num3: m_GizmoMode = Scale; return true;
            default: break;
        }
        return false;
    }

    bool SceneEditorView::OnMouseDown_Event(events::MouseDownEvent& e)
    {
        auto mouse_pos = Vector2{ ImGui::GetMousePos().x, ImGui::GetMousePos().y };
        mouse_pos.x -= m_ViewportBounds[0].x;
        mouse_pos.y -= m_ViewportBounds[0].y;
        mouse_pos.y = (m_ViewportBounds[1] - m_ViewportBounds[0]).y - mouse_pos.y;
        if (mouse_pos.x >= 0 && mouse_pos.y >= 0 && mouse_pos.x <= m_ViewportSize.x && mouse_pos.y <= m_ViewportSize.y)
        {
            auto& d = m_Descriptor;
            if (d->selectedEntity.entity && d->selectedEntity.entity.HasComponent<TilemapComponent>())
            {
                if (Input::IsMouseDown(Mouse::LeftMouse))
                {
                    auto& tmc    = d->selectedEntity.entity.GetComponent<TilemapComponent>();
                    auto& camera = Editor::GetViewportCamera();

                    // Vector conversion fiesta
                    const auto camera_dim =
                        Vector3f{ camera.GetWidth() * camera.GetPan(), camera.GetHeight() * camera.GetPan(), 0.0f };
                    auto tile_pos = scene::Camera::ScreenCoordinatesToWorld(camera, mouse_pos, camera.GetPos());
                    tile_pos =
                        utils::Snap(tile_pos, Vector3f{ tmc.gridSize, 1.0f }) + Vector3f{ tmc.gridSize / 2.0f, 0.0f };
                    if (tmc.currentState == TilemapComponent::State::Brush)
                    {
                        tmc.AddTile(tile_pos, tmc.currentTile);
                    }
                    else if (tmc.currentState == TilemapComponent::State::Erase)
                    {
                        tmc.RemoveTile(tile_pos);
                    }
                    return true;
                }
            }
        }
        return false;
    }

    bool SceneEditorView::OnMouseMove_Event(events::MouseMoveEvent& e)
    {
        auto mouse_pos = Vector2{ ImGui::GetMousePos().x, ImGui::GetMousePos().y };
        mouse_pos.x -= m_ViewportBounds[0].x;
        mouse_pos.y -= m_ViewportBounds[0].y;
        mouse_pos.y = (m_ViewportBounds[1] - m_ViewportBounds[0]).y - mouse_pos.y;
        if (mouse_pos.x >= 0 && mouse_pos.y >= 0 && mouse_pos.x <= m_ViewportSize.x && mouse_pos.y <= m_ViewportSize.y)
        {
            auto& d = m_Descriptor;

            if (Input::IsMouseDown(Mouse::MiddleMouse))
            {
                if (Input::IsMouseDragging())
                {
                    auto&      camera = Editor::GetViewportCamera();
                    const auto vec    = Vector2f{ Input::GetMouseDeltaX(), Input::GetMouseDeltaY() * -1.0f };
                    camera.SetPos(camera.GetPos() + utils::ToVec3f(vec) * camera.GetPan());
                    return true;
                }
            }
            else if (d->selectedEntity.entity && d->selectedEntity.entity.HasComponent<TilemapComponent>())
            {
                if (Input::IsMouseDown(Mouse::LeftMouse))
                {
                    auto& tmc    = d->selectedEntity.entity.GetComponent<TilemapComponent>();
                    auto& camera = Editor::GetViewportCamera();

                    // Vector conversion fiesta
                    const auto camera_dim =
                        Vector3f{ camera.GetWidth() * camera.GetPan(), camera.GetHeight() * camera.GetPan(), 0.0f };
                    auto tile_pos = scene::Camera::ScreenCoordinatesToWorld(camera, mouse_pos, camera.GetPos());
                    tile_pos =
                        utils::Snap(tile_pos, Vector3f{ tmc.gridSize, 1.0f }) + Vector3f{ tmc.gridSize / 2.0f, 0.0f };
                    if (tmc.currentState == TilemapComponent::State::Brush)
                    {
                        tmc.AddTile(tile_pos, tmc.currentTile);
                    }
                    else if (tmc.currentState == TilemapComponent::State::Erase)
                    {
                        tmc.RemoveTile(tile_pos);
                    }
                    return true;
                }
            }
        }
        return false;
    }

    bool SceneEditorView::OnMouseScroll_Event(events::MouseScrollEvent& e)
    {
        auto mouse_pos = Vector2{ ImGui::GetMousePos().x, ImGui::GetMousePos().y };
        mouse_pos.x -= m_ViewportBounds[0].x;
        mouse_pos.y -= m_ViewportBounds[0].y;
        mouse_pos.y = (m_ViewportBounds[1] - m_ViewportBounds[0]).y - mouse_pos.y;
        if (mouse_pos.x >= 0 && mouse_pos.y >= 0 && mouse_pos.x <= m_ViewportSize.x && mouse_pos.y <= m_ViewportSize.y)
        {
            auto& camera = Editor::GetViewportCamera();
            camera.SetPan(camera.GetPan() + e.GetOffsetY() * -0.05f);
            return true;
        }
        return false;
    }

    i32 SceneEditorView::CompileProject(const bool wait)
    {
        auto& d = m_Descriptor;

        if (NBMan::InstanceLoaded())
            NBMan::Unload();

        sys::ProcessInfo p_info;

#ifdef CX_PLATFORM_WINDOWS
        // p_info.command = "cmake ./ -G \"Visual Studio 17\" -B builds/vs2022 && cmake --build builds/vs2022";
        p_info.command = "python3 Scripts/build.py --preset=windows-llvm-any-debug --build";
#elif defined(CX_PLATFORM_LINUX)
        p_info.command = "python3 Scripts/build.py --preset=linux-any-debug --build";
#elif defined(CX_PLATFORM_OSX)
        p_info.command = "python3 Scripts/build.py --preset=osx-any-debug --build";
#endif
        p_info.onExit = [this](i32 exitCode)
        {
            if (exitCode == 0)
            {
                auto& d = m_Descriptor;
                NBMan::Load(d->scriptModulePath);
                ConsoleMan::AppendMessage("-- Script build finished.");
            }
            else
            {
                throw new InvalidOperationException("Project failed to compile!");
            }
        };

#ifndef CX_PLATFORM_UNIX
        p_info.redirectStdOut = true;
        p_info.redirectStdErr = true;
#endif

        const auto redirector = [](const char* buffer, usize len)
        { ConsoleMan::AppendMessage(std::string(buffer, len)); };

        ConsoleMan::AppendMessage("-- Script build started.");
        auto proc                     = sys::Process::New(std::move(p_info));
        proc->Event_OnOutDataReceived = redirector;
        proc->Event_OnErrDataReceived = redirector;
        proc->Launch();

        if (wait)
            return proc->WaitForExit();
        return 0;
    }

    void SceneEditorView::OnScenePlay() noexcept
    {
        auto& d = m_Descriptor;
        d->selectedEntity.Deselect();
        d->runtimeScene = mem::Shared<Scene>::New();
        d->editorScene->CopyTo(*d->runtimeScene);
        d->activeScene = d->runtimeScene;
        d->activeScene.Lock()->SetState(Scene::State::Play);
        d->activeScene.Lock()->OnRuntimeStart();
    }

    void SceneEditorView::OnSceneSimulate() noexcept
    {
        auto& d = m_Descriptor;
        d->selectedEntity.Deselect();
        d->runtimeScene = mem::Shared<Scene>::New();
        d->editorScene->CopyTo(*d->runtimeScene);
        d->activeScene = d->runtimeScene;
        d->activeScene.Lock()->SetState(Scene::State::Simulate);
        d->activeScene.Lock()->OnSimulationStart();
    }

    void SceneEditorView::OnSceneStop() noexcept
    {
        auto& d     = m_Descriptor;
        auto  scene = d->activeScene.Lock();
        if (scene->GetState() == Scene::State::Play)
            d->activeScene.Lock()->OnRuntimeStop();
        else
            scene->OnSimulationStop();
        d->runtimeScene.Reset();
        d->activeScene = d->editorScene;

        // Just in case when we select an entity during runtime.
        d->selectedEntity.entity = Entity::None();
    }

    void SceneEditorView::InitializePanel(EditorPanel& panel) const noexcept
    {
        panel.OnInit();
    }

    void SceneEditorView::VisualizeColliders() const noexcept
    {
        CX_DEBUG_PROFILE_SCOPE("VisualizeColliders")

        const auto& d = m_Descriptor;

        const auto box_colliders = d->activeScene.Lock()->GetAllEntitiesWithComponent<BoxCollider2DComponent>();
        for (const auto& e : box_colliders)
        {
            const auto& bc = e.GetComponent<BoxCollider2DComponent>();
            const auto& tc = e.GetComponent<TransformComponent>();
            m_DebugDraw.DrawRect2D({ bc.offset.x + tc.position.x, bc.offset.y + tc.position.y,
                                     bc.size.x * tc.scale.x * 2.0f, bc.size.y * tc.scale.y * 2.0f },
                                   tc.rotation.z);
        }

        const auto circle_colliders = d->activeScene.Lock()->GetAllEntitiesWithComponent<CircleCollider2DComponent>();
        for (const auto& e : circle_colliders)
        {
            const auto& cc = e.GetComponent<CircleCollider2DComponent>();
            const auto& tc = e.GetComponent<TransformComponent>();
            m_DebugDraw.DrawCircle2D(Vector3f{ cc.offset, 0.0f } + tc.position, cc.radius * tc.scale.x * tc.scale.y,
                                     tc.rotation.z);
        }
    }

    void SceneEditorView::LoadProject(const std::filesystem::path cxproj)
    {
        auto& d = m_Descriptor;

        UnloadProject();

        d->currentProjectPath = cxproj;
        d->currentProjectPath = d->currentProjectPath.parent_path();

        // NOTE: I do not like this.
        stdfs::current_path(d->currentProjectPath);

        // TODO: Regarding to UnloadScriptModule(), LoadScriptModule() and CompileProject() here:
        // Some projects might just not use C++ for scripting (in the future when add C#
        // support) so make sure to not always force load the C++ script module. For now I will
        // leave this as is because there's only C++ support.

        d->editorScene.Reset(new Scene);
        d->scriptModulePath = d->currentProjectPath / stdfs::path("lib/libNBMan.dll");
        d->activeScene      = d->editorScene;

        try
        {
            if (true || CompileProject(true) == 0)
            {
                NBMan::Load(d->scriptModulePath);
                ConsoleMan::AppendMessage("-- Script module load finished.");
            }
            else
            {
                cx_throw(InvalidOperationException, "Project failed to compile");
            }
        }
        catch (const CodexException& ex)
        {
            lgx::Get("editor").Log(lgx::Error, "Failed to load compile and load NBMan: {}", ex.to_string());
        }

        SerializationManager::LoadFromFile(*d->editorScene, cxproj);

        // FIXME: Cannot unload the script module while we have attached scripts, invalidates the
        // vptr of our class effetively invalidating the whole thing really.
        CompileProject(); // NOTE
    }

    void SceneEditorView::UnloadProject()
    {
        auto& d = m_Descriptor;
        d->selectedEntity.Deselect();

        if (NBMan::InstanceLoaded())
            NBMan::Unload();
    }

    void SceneEditorView::RenderGrid(gfx::DebugDraw& renderer, const scene::EditorCamera& camera,
                                     const GridRendererComponent& c) noexcept
    {
        CX_DEBUG_PROFILE_SCOPE("RenderGrid")

        const auto camera_dim = Vector2{ camera.GetWidth() * camera.GetPan(), camera.GetHeight() * camera.GetPan() };
        const auto camera_pos = camera.GetPos() - Vector3f{ camera_dim / 2, 0.0f };
        const auto start_pos  = glm::ceil(camera_pos / Vector3f{ c.cellSize, 1.0f }) * Vector3f{ c.cellSize, 0.0f };
        const auto count      = camera_dim / Vector2{ c.cellSize } + 1;

        for (auto i = 0; i < count.x; ++i)
        {
            renderer.DrawLine2D({ start_pos.x + i * c.cellSize.x, camera_pos.y },
                                { start_pos.x + i * c.cellSize.x, camera_pos.y + camera_dim.y }, c.colour);
        }

        for (auto i = 0; i < count.y; ++i)
        {
            renderer.DrawLine2D({ camera_pos.x, start_pos.y + i * c.cellSize.y },
                                { camera_pos.x + camera_dim.x, start_pos.y + i * c.cellSize.y }, c.colour);
        }
    }

    void SceneEditorView::DrawVec3Control(const char* label, Vector3f& values, const f32 columnWidth, const f32 speed,
                                          const f32 resetValue)
    {
        ImGuiIO& io        = ImGui::GetIO();
        auto     bold_font = io.Fonts->Fonts[0];

        ImGui::PushID(label);

        if (label[0] != '#' && label[1] != '#')
        {
            ImGui::Columns(2);
            ImGui::SetColumnWidth(0, columnWidth);
            ImGui::Text("%s", label);
            ImGui::NextColumn();
        }

        ImGui::PushMultiItemsWidths(3, ImGui::CalcItemWidth());
        ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2{ 0, 0 });

        float  lineHeight = GImGui->Font->FontSize + GImGui->Style.FramePadding.y * 2.0f;
        ImVec2 buttonSize = { lineHeight + 3.0f, lineHeight };

        ImGui::PushStyleColor(ImGuiCol_Button, ImVec4{ 0.8f, 0.1f, 0.15f, 1.0f });
        ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4{ 0.9f, 0.2f, 0.2f, 1.0f });
        ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4{ 0.8f, 0.1f, 0.15f, 1.0f });
        ImGui::PushFont(bold_font);
        if (ImGui::Button("X", buttonSize))
            values.x = resetValue;
        ImGui::PopFont();
        ImGui::PopStyleColor(3);

        ImGui::SameLine();
        ImGui::DragFloat("##X", &values.x, speed, 0.0f, 0.0f, "%.2f");
        ImGui::PopItemWidth();
        ImGui::SameLine();

        ImGui::PushStyleColor(ImGuiCol_Button, ImVec4{ 0.2f, 0.7f, 0.2f, 1.0f });
        ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4{ 0.3f, 0.8f, 0.3f, 1.0f });
        ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4{ 0.2f, 0.7f, 0.2f, 1.0f });
        ImGui::PushFont(bold_font);
        if (ImGui::Button("Y", buttonSize))
            values.y = resetValue;
        ImGui::PopFont();
        ImGui::PopStyleColor(3);

        ImGui::SameLine();
        ImGui::DragFloat("##Y", &values.y, speed, 0.0f, 0.0f, "%.2f");
        ImGui::PopItemWidth();
        ImGui::SameLine();

        ImGui::PushStyleColor(ImGuiCol_Button, ImVec4{ 0.1f, 0.25f, 0.8f, 1.0f });
        ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4{ 0.2f, 0.35f, 0.9f, 1.0f });
        ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4{ 0.1f, 0.25f, 0.8f, 1.0f });
        ImGui::PushFont(bold_font);
        if (ImGui::Button("Z", buttonSize))
            values.z = resetValue;
        ImGui::PopFont();
        ImGui::PopStyleColor(3);

        ImGui::SameLine();
        ImGui::DragFloat("##Z", &values.z, speed, 0.0f, 0.0f, "%.2f");
        ImGui::PopItemWidth();

        ImGui::PopStyleVar();

        if (label[0] != '#' && label[1] != '#')
            ImGui::Columns(1);

        ImGui::PopID();
    }

    void SceneEditorView::DrawVec2Control(const char* label, Vector2f& values, const f32 columnWidth, const f32 speed,
                                          const f32 resetValue)
    {
        ImGuiIO& io        = ImGui::GetIO();
        auto     bold_font = io.Fonts->Fonts[0];

        ImGui::PushID(label);

        if (label[0] != '#' && label[1] != '#')
        {
            ImGui::Columns(2);
            ImGui::SetColumnWidth(0, columnWidth);
            ImGui::Text("%s", label);
            ImGui::NextColumn();
        }

        ImGui::PushMultiItemsWidths(3, ImGui::CalcItemWidth());
        ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2{ 0, 0 });

        f32    lineHeight = GImGui->Font->FontSize + GImGui->Style.FramePadding.y * 2.0f;
        ImVec2 buttonSize = { lineHeight + 3.0f, lineHeight };

        ImGui::PushStyleColor(ImGuiCol_Button, ImVec4{ 0.8f, 0.1f, 0.15f, 1.0f });
        ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4{ 0.9f, 0.2f, 0.2f, 1.0f });
        ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4{ 0.8f, 0.1f, 0.15f, 1.0f });
        ImGui::PushFont(bold_font);
        if (ImGui::Button("X", buttonSize))
            values.x = resetValue;
        ImGui::PopFont();
        ImGui::PopStyleColor(3);

        ImGui::SameLine();
        ImGui::DragFloat("##X", &values.x, speed, 0.0f, 0.0f, "%.2f");
        ImGui::PopItemWidth();
        ImGui::SameLine();

        ImGui::PushStyleColor(ImGuiCol_Button, ImVec4{ 0.2f, 0.7f, 0.2f, 1.0f });
        ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4{ 0.3f, 0.8f, 0.3f, 1.0f });
        ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4{ 0.2f, 0.7f, 0.2f, 1.0f });
        ImGui::PushFont(bold_font);
        if (ImGui::Button("Y", buttonSize))
            values.y = resetValue;
        ImGui::PopFont();
        ImGui::PopStyleColor(3);

        ImGui::SameLine();
        ImGui::DragFloat("##Y", &values.y, speed, 0.0f, 0.0f, "%.2f");
        ImGui::PopItemWidth();

        ImGui::PopStyleVar();

        if (label[0] != '#' && label[1] != '#')
            ImGui::Columns(1);

        ImGui::PopID();
    }
} // namespace codex::editor
