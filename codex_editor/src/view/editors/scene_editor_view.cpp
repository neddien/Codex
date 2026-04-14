#include "scene_editor_view.h"

#include <console_man.h>
#include <editor.h>
#include <editor_application.h>
#include <nfd.h>

#include "panels/project_settings_view.h"
#include "panels/properties_view.h"
#include "panels/scene_hierarchy_view.h"
#include "panels/toolbar_view.h"

#include <imgui.h>
#include <imgui_internal.h>

namespace codex::editor {
    void EditorPanelDeleter::operator()(EditorPanel* panel) noexcept
    {
        delete panel;
    }

    namespace stdfs = std::filesystem;

    cc::Task<int> ltask()
    {
        info("ltask we here.");
        co_await Engine::get_worker_pool();
        info("ltask we on a thread now!");

        std::this_thread::sleep_for(std::chrono::milliseconds(3000));
        info("ltask sleep done");
        co_return 69;
    }

    cc::Task<void> ftask()
    {
        info("ftask here");
        co_await Engine::get_worker_pool();

        auto task = ltask();
        std::this_thread::sleep_for(std::chrono::milliseconds(1000));
        info("came back from sleep");
        int res = co_await task;
        info("res: {}", res);
    }

    void SceneEditorView::on_attach()
    {
        ftask();

        auto mount    = Shared<fs::MemoryMount>::make(0);
        auto disk_mnt = Shared<fs::DiskMount>::make("/tmp/cxvfs", 0);

        {
            auto handle = disk_mnt->open("my_lovely_dog.txt", { fs::FileMode::Create | fs::FileMode::Write });
            if (handle) {
                handle->write_async("bbruh", 5).resume().await_sync();
            } else {
                warn("Failed to open my_lovely_dog.txt for writing");
            }
        }

        // Write a file and create an explicit empty directory directly on the mount.
        mount->open("my/lovely/dog.txt", { fs::FileMode::Write | fs::FileMode::Create })
            ->write("rexxinator is my sweet dog.", 28);
        mount->mkdir("my/lovely/empty_dir");

        fs::VirtualFilesystem vfs;
        vfs.mkdir("/bruh");
        vfs.mount(mount, "/bruh");
        vfs.mount(disk_mnt, "/disk", true);

        // Read the file back through the VFS.
        {
            auto handle = vfs.open("/bruh/my/lovely/dog.txt", { fs::FileMode::Read });
            if (handle) {
                char buf[32]{};
                handle->read(buf, handle->size());
                info("read: {}", buf);
            } else {
                warn("Failed to open /bruh/my/lovely/dog.txt for reading");
            }
        }

        {
            auto handle = vfs.open("/disk/my_lovely_dog.txt", { fs::FileMode::Read });
            if (handle) {
                char buf[32]{};
                handle->read_async(buf, handle->size()).resume().await_sync();
                info("read: {}", buf);
            } else {
                warn("Failed to open /disk/my_lovely_dog.txt for reading");
            }
        }

        // Create a directory through the VFS (propagates to mount).
        vfs.mkdir("/bruh/photos");

        // List /bruh — should show "my" (from mount files) and "photos" (trie node + mount dir).
        info("list /bruh:");
        for (const auto& e : vfs.list("/bruh"))
            info("  {}", e);

        // List /bruh/my/lovely — should show "dog.txt" and "empty_dir".
        info("list /bruh/my/lovely:");
        for (const auto& e : vfs.list("/bruh/my/lovely"))
            info("  {}", e);

        // List /disk — should show "dog.txt" and "empty_dir".
        info("list /disk:");
        for (const auto& e : vfs.list("/disk"))
            info("  {}", e);

        {
            auto pak_handle = vfs.open("/disk/my_pak.cpkz", { fs::FileMode::Create | fs::FileMode::Read |
                                                              fs::FileMode::Write | fs::FileMode::Trunc });

            if (pak_handle) {
                if (vfs.export_to_pak(pak_handle,
                                      fs::PakProperties{
                                          .enable_compression = true,
                                          .flags   = fs::PakFlags::Compressed | fs::PakFlags::CompressionTypeLZ4,
                                          .name    = "editor_tmp",
                                          .version = { 0xff, 0xff, 0xff },
                                      })) {
                    info("Paked created at: {}", pak_handle->path());
                } else {
                    warn("Failed to cook the VFS");
                }
            } else {
                warn("Failed  to open pak handle");
            }
        }

        auto pak_handle = vfs.open("/disk/my_pak.cpkz", { fs::FileMode::Read });
        if (pak_handle) {
            auto pak_mount = Shared<fs::PakMount>::make(pak_handle, 0);
            vfs.mount(std::move(pak_mount), "/cxpkz", true);
        } else {
            warn("Failed to open pak handle");
        }

        info("list /cxpkz:");
        for (const auto& e : vfs.list("/cxpkz", fs::ListOptions::Recursive)) {
            info("  {}", e);
        }

        auto handle = vfs.open("/cxpkz/disk/doc/AssetManager.md");
        if (handle) {
            std::string buffer;
            buffer.resize(handle->size());
            handle->read(buffer.data(), handle->size());
            info("read: {}", buffer);
        } else {
            warn("failed to open /cxpkz/bruh/my/lovely/dog.txt");
        }

        descriptor_ =
            Shared<SceneEditorDescriptor>::from(new SceneEditorDescriptor{ .editor_scene = Shared<Scene>::make() });
        descriptor_->active_scene = descriptor_->editor_scene;

        // Panels
        this->attach_panel<SceneHierarchyView>();
        this->attach_panel<PropertiesView>();
        this->attach_panel<ToolbarView>();

        opengl::FrameBufferProperties props;
        props.attachments = { { .format = opengl::TextureFormat::RGBA8 },
                              { .format = opengl::TextureFormat::RedInt32 } };

        // TODO: This is the scene render resolution so you should not hard code this.
        props.width  = 1920;
        props.height = 1080;
        framebuffer_ = Box<opengl::FrameBuffer>::make(props);

        // EditorLayer::GetCamera().SetProjectionType(scene::Camera::ProjectionType::Perspective);

        // TODO: We should z-index using the depth buffer in the future so that we can also have 3d elements
        // instead of disabling the depth buffer and z-index'ing by sorting our render batches.
        // glEnable(GL_DEPTH_TEST);
        // glDepthFunc(GL_LESS);
    }

    void SceneEditorView::on_detach()
    {
        unload_project();
    }

    void SceneEditorView::on_update(const f32 deltaTime)
    {
        auto& d     = descriptor_;
        auto  scene = d->active_scene.lock();

        // Load NBMan on main thread once async compilation succeeds,
        // then attach any pending scripts (from deserialization or recompilation).
        if (d->pending_nb_load.exchange(false)) {
            NBMan::load(d->script_module_path, *scene);
            ConsoleMan::append_message("-- Script module load finished.");

            auto nbc_view = scene->get_all_entities_with_component<NativeBehaviourComponent>();
            for (auto& e : nbc_view)
                e.get_component<NativeBehaviourComponent>().attach_pending_scripts();
        }

        // Auto-hide compilation notification after 3 seconds.
        {
            const auto state = d->compilation_state.load();
            if (state == CompilationState::Succeeded || state == CompilationState::Failed) {
                if (d->compilation_finish_time == 0.0)
                    d->compilation_finish_time = ImGui::GetTime();
                else if (ImGui::GetTime() - d->compilation_finish_time > 3.0) {
                    d->compilation_state.store(CompilationState::Idle);
                    d->compilation_finish_time = 0.0;
                }
            }
        }

        framebuffer_->bind();

        // Viewport resize
        {
            CX_DEBUG_PROFILE_SCOPE("on_update::viewport_resize")

            auto&       camera          = Editor::get_viewport_camera();
            static auto prev_viewport   = viewport_size_;
            static auto prev_camera_pan = camera.pan();
            if (viewport_size_ != prev_viewport || camera.pan() != prev_camera_pan) {
                camera.set_width(viewport_size_.x);
                camera.set_height(viewport_size_.y);
                prev_viewport   = viewport_size_;
                prev_camera_pan = camera.pan();
                framebuffer_->resize((u32)viewport_size_.x, (u32)viewport_size_.y);
            }
        }

        gfx::Renderer::set_clear_colour(0.2f, 0.2f, 0.2f, 1.0f);
        gfx::Renderer::clear();

        debug_draw_.begin(Editor::get_viewport_camera());

        switch (scene->state()) {
            case Scene::State::Edit: {
                CX_DEBUG_PROFILE_SCOPE("on_update::on_editor_update")

                if (d->selected_entity.entity) {
                    if (d->selected_entity.entity.has_component<GridRendererComponent>()) {
                        render_grid(debug_draw_, Editor::get_viewport_camera(),
                                    d->selected_entity.entity.get_component<GridRendererComponent>());
                    }
                }

                visualize_colliders();

                scene->on_editor_update(deltaTime, Editor::get_viewport_camera());
                break;
            }
            case Scene::State::Play: {
                CX_DEBUG_PROFILE_SCOPE("on_update::on_runtime_update")
                scene->on_runtime_update(deltaTime);
                break;
            }
            case Scene::State::Simulate: {
                CX_DEBUG_PROFILE_SCOPE("on_update::on_simulation_update")
                visualize_colliders();
                scene->on_simulation_update(deltaTime, Editor::get_viewport_camera());
                break;
            }
        }

        auto [mx, my] = ImGui::GetMousePos();
        mx -= viewport_bounds_[0].x;
        my -= viewport_bounds_[0].y;
        const Vector2f viewport_size = viewport_bounds_[1] - viewport_bounds_[0];
        const i32      mouse_x       = (i32)mx;
        const i32      mouse_y       = (i32)my;

        if (Input::is_mouse_down(Mouse::LeftMouse) && mouse_x >= 0 && mouse_y >= 0 && mouse_x <= (i32)viewport_size.x &&
            mouse_y <= (i32)viewport_size.y && !gizmo_active_) {
            if (d->active_scene.lock()->state() != Scene::State::Play && !gizmo_active_ &&
                (!d->selected_entity.entity || !d->selected_entity.entity.has_component<TilemapComponent>())) {
                Vector2f scale = { framebuffer_->properties().width / viewport_size.x,
                                   framebuffer_->properties().height / viewport_size.y };
                Vector2f pos   = { mouse_x, viewport_size.y - mouse_y };
                pos *= scale;
                pos          = glm::round(pos);
                const i32 id = framebuffer_->read_pixel(1, (i32)pos.x, (i32)pos.y);
                auto      e  = Entity((entt::entity)id, d->active_scene.lock().get());
                if (e)
                    d->selected_entity.select(e);
            }
        }

        debug_draw_.end();

        framebuffer_->unbind();

        // Update our panels.
        for (auto& panel : view_panels_)
            panel->on_pre_update(deltaTime);
    }

    void SceneEditorView::on_imgui_render()
    {
        CX_DEBUG_PROFILE_SCOPE()

        auto& d  = descriptor_;
        auto& io = ImGui::GetIO();

        // Dockspace, viewport and gizmo.
        {
            static auto dockspace_open  = true;
            static auto dockspace_flags = ImGuiDockNodeFlags_None;
            static auto fullscreen      = true;
            auto        window_flags    = ImGuiWindowFlags_MenuBar | ImGuiWindowFlags_NoDocking;
            if (fullscreen) {
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

            if (io.ConfigFlags & ImGuiConfigFlags_DockingEnable) {
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
            for (auto& e : dbg::Profiler::get_profilers()) {
                const auto info = e.second.info();
                const auto dur  = e.second.elapsed_as<std::milli, f32>().count();
                ImGui::Text("%s: %fms", info.name.c_str(), dur);
            }
            ImGui::End();
        }

        // File menu
        {
            if (ImGui::BeginMainMenuBar()) {
                if (ImGui::BeginMenu("File")) {
                    // Add File menu items here
                    if (ImGui::MenuItem("Create new project", "Ctrl+N")) {}
                    if (ImGui::MenuItem("Open", "Ctrl+O")) {
                        nfdu8char_t*      outPath   = nullptr;
                        nfdu8filteritem_t filters[] = { { "Codex Project", "cxproj" } };
                        if (NFD_OpenDialogU8(&outPath, filters, 1, nullptr) == NFD_OKAY) {
                            load_project(stdfs::path(outPath));
                            NFD_FreePathU8(outPath);
                        }
                    }
                    if (ImGui::MenuItem("Compile project")) {
                        compile_project();
                    }
                    if (ImGui::MenuItem("Clear build files")) {
                        auto& d = descriptor_;
                        // d->active_scene.lock()->UnloadScriptModule();

                        /*
                        const auto files =
                            fs::get_all_files_with_extensions(d->current_project_path / "Assets/", { ".h", ".hpp", ".hh"
                        }); std::vector<rf::RFScript> rf_files; rf_files.reserve(files.size());

                        const auto output_path = stdfs::absolute(d->current_project_path / "int/");
                        for (const auto& f : files)
                            rf_files.emplace_back(f).EmitMetadata(output_path);
                        rf::RFScript::EmitBaseClass(output_path, rf_files);
                        */

                        sys::ProcessInfo p_info;

#ifdef CX_PLATFORM_WINDOWS
                        p_info.command = "cmake --preset windows-llvm-any-debug --clear";
#elif defined(CX_PLATFORM_LINUX)
                        p_info.command = "./build.py --preset linux-any-debug --clear";
#elif defined(CX_PLATFORM_OSX)
                        p_info.command = "./build.py --preset osx-any-debug --clear";
#endif
                        p_info.on_exit = [this](i32 exitCode)
                        {
                            auto& d = descriptor_;
                            // TODO: Scene should also be thread safe since this callback is being called from a
                            // different thread.
                            // Scene::LoadScriptModule(d->script_module_path); // TODO: COME BACK
                            ConsoleMan::append_message("-- Clear finished.");
                        };
                        p_info.redirect_stdout = true;
                        p_info.redirect_stderr = true;

                        const auto redirector = [](const char* buffer, usize len)
                        { ConsoleMan::append_message(std::string(buffer, len)); };

                        ConsoleMan::append_message("-- Clear started.");
                        auto proc                  = sys::Process::create(std::move(p_info));
                        proc->on_out_data_received = redirector;
                        proc->on_err_data_received = redirector;
                        proc->launch();
                    }
                    if (ImGui::MenuItem("Save", "Ctrl+S")) {
                        // Handle the "Save" action
                        static std::string save_path;
                        if (save_path.empty()) {
                            nfdu8char_t*      outPath   = nullptr;
                            nfdu8filteritem_t filters[] = { { "Codex Project", "cxproj" } };
                            if (NFD_SaveDialogU8(&outPath, filters, 1, nullptr, "default.cxproj") == NFD_OKAY) {
                                save_path = outPath;
                                NFD_FreePathU8(outPath);
                                // TODO: project->Save(path);
                                d->selected_entity.deselect();
                                SerializationManager::save_to_file(*d->active_scene.lock(), save_path.c_str());
                            }
                        } else {
                            d->selected_entity.deselect();
                            SerializationManager::save_to_file(*d->active_scene.lock(), save_path.c_str());
                        }
                    }
                    if (ImGui::MenuItem("Exit", "Alt+F4")) {
                        Engine::get().stop();
                    }

                    ImGui::EndMenu();
                }
                if (ImGui::BeginMenu("Edit")) {
                    if (ImGui::MenuItem("Project Settings")) {
                        this->attach_panel<ProjectSettingsView>();
                    }
                    ImGui::EndMenu();
                }
                ImGui::EndMainMenuBar();
            }
        }

        // Engine viewport
        {
            auto& camera = Editor::get_viewport_camera();

            ImGui::Begin("Viewport");
            const auto viewport_min_region = ImGui::GetWindowContentRegionMin();
            const auto viewport_max_region = ImGui::GetWindowContentRegionMax();
            const auto viewport_offset     = ImGui::GetWindowPos();
            viewport_bounds_[0]            = { viewport_min_region.x + viewport_offset.x,
                                               viewport_min_region.y + viewport_offset.y };
            viewport_bounds_[1]            = { viewport_max_region.x + viewport_offset.x,
                                               viewport_max_region.y + viewport_offset.y };

            auto current_viewport_window_size = ImGui::GetContentRegionAvail();
            viewport_size_ = Vector2f{ current_viewport_window_size.x, current_viewport_window_size.y };
            ImGui::Image(reinterpret_cast<ImTextureID>(framebuffer_->colour_attachment_id_at(0)),
                         current_viewport_window_size, { 0, 1 }, { 1, 0 });

            viewport_focused_ = ImGui::IsWindowFocused();
            viewport_hovered_ = ImGui::IsWindowHovered();

            // Engine::get().get_imgui_layer()->block_events(!viewport_focused_);

            // Guizmo
            {
                if (d->selected_entity.entity) {
                    ImGuizmo::SetOrthographic(true);
                    ImGuizmo::SetDrawlist();

                    ImGuizmo::SetRect(viewport_bounds_[0].x, viewport_bounds_[0].y,
                                      viewport_bounds_[1].x - viewport_bounds_[0].x,
                                      viewport_bounds_[1].y - viewport_bounds_[0].y);

                    // Camera
                    auto proj_mat = camera.projection_matrix();
                    auto view_mat = camera.view_matrix();

                    auto& tc        = d->selected_entity.entity.get_component<TransformComponent>();
                    auto  transform = tc.to_matrix();

                    ImGuizmo::Manipulate(glm::value_ptr(view_mat), glm::value_ptr(proj_mat),
                                         (ImGuizmo::OPERATION)gizmo_mode_, ImGuizmo::MODE::LOCAL,
                                         glm::value_ptr(transform));

                    gizmo_active_ = ImGuizmo::IsOver();

                    if (gizmo_active_ && ImGuizmo::IsUsing()) {
                        Vector3f rotation;
                        codex::math::transform_decompose(transform, tc.position, rotation, tc.scale);
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
            ImGui::Text("FPS: %u", Engine::fps());
            ImGui::Text("Delta time: %f", Engine::delta());
            ImGui::Text("Batch count: %zu", gfx::BatchRenderer2D::batch_count());
            ImGui::Text("Total quad count: %zu", gfx::BatchRenderer2D::quad_count());
            ImGui::End();
        }

        auto block_events = !viewport_focused_;

        // Render our panels.
        {
            CX_DEBUG_PROFILE_SCOPE("panel_render")

            auto it = view_panels_.begin();
            while (it != view_panels_.end()) {
                auto& panel = *it;
                if (!panel->show_)
                    it = view_panels_.erase(it);
                else {
                    block_events = block_events && panel->imgui_block_events();
                    panel->on_pre_imgui_render();
                    ++it;
                }
            }
        }

        Engine::get().mgui_layer()->block_events(block_events);

        // Compilation status overlay (bottom-right)
        {
            const auto state = d->compilation_state.load();
            if (state != CompilationState::Idle) {
                auto*        viewport    = ImGui::GetMainViewport();
                const ImVec2 overlay_pos = { viewport->Pos.x + viewport->Size.x - 20.0f,
                                             viewport->Pos.y + viewport->Size.y - 20.0f };
                ImGui::SetNextWindowPos(overlay_pos, ImGuiCond_Always, { 1.0f, 1.0f });
                ImGui::SetNextWindowBgAlpha(0.75f);

                const auto overlay_flags = ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_NoInputs |
                                           ImGuiWindowFlags_AlwaysAutoResize | ImGuiWindowFlags_NoFocusOnAppearing |
                                           ImGuiWindowFlags_NoNav | ImGuiWindowFlags_NoMove;

                if (ImGui::Begin("##compilation_overlay", nullptr, overlay_flags)) {
                    const float icon_radius    = 8.0f;
                    const float icon_thickness = 2.5f;

                    // Reserve space for the icon
                    ImGui::Dummy({ icon_radius * 2.0f, icon_radius * 2.0f });
                    const ImVec2 icon_min    = ImGui::GetItemRectMin();
                    const ImVec2 icon_center = { icon_min.x + icon_radius, icon_min.y + icon_radius };
                    auto*        draw_list   = ImGui::GetWindowDrawList();

                    ImGui::SameLine();

                    if (state == CompilationState::Compiling) {
                        // Rotating arc spinner
                        const float t     = (float)ImGui::GetTime();
                        const float a_min = t * 3.0f;
                        const float a_max = a_min + IM_PI * 1.5f;
                        draw_list->PathArcTo(icon_center, icon_radius, a_min, a_max, 24);
                        draw_list->PathStroke(IM_COL32(100, 180, 255, 255), 0, icon_thickness);
                        ImGui::Text("Compiling...");
                    } else if (state == CompilationState::Succeeded) {
                        // Green checkmark
                        const ImU32 green = IM_COL32(80, 220, 80, 255);
                        draw_list->AddLine({ icon_center.x - 6.0f, icon_center.y },
                                           { icon_center.x - 1.0f, icon_center.y + 5.0f }, green, icon_thickness);
                        draw_list->AddLine({ icon_center.x - 1.0f, icon_center.y + 5.0f },
                                           { icon_center.x + 7.0f, icon_center.y - 5.0f }, green, icon_thickness);
                        ImGui::Text("Build succeeded");
                    } else if (state == CompilationState::Failed) {
                        // Red X
                        const ImU32 red = IM_COL32(230, 70, 70, 255);
                        draw_list->AddLine({ icon_center.x - 5.0f, icon_center.y - 5.0f },
                                           { icon_center.x + 5.0f, icon_center.y + 5.0f }, red, icon_thickness);
                        draw_list->AddLine({ icon_center.x + 5.0f, icon_center.y - 5.0f },
                                           { icon_center.x - 5.0f, icon_center.y + 5.0f }, red, icon_thickness);
                        ImGui::Text("Build failed");
                    }
                }
                ImGui::End();
            }
        }

        ImGui::End();
    }

    void SceneEditorView::on_event(events::Event& e)
    {
        // Child panel events should be handled first because of painter's rule.
        for (auto& panel : view_panels_) {
            if (e.handled)
                return;
            panel->on_pre_event(e);
        }

        events::EventDispatcher d{ e };
        d.dispatch<events::KeyDownEvent>(bind_event_delegate(this, &SceneEditorView::on_key_down_event));
        d.dispatch<events::MouseDownEvent>(bind_event_delegate(this, &SceneEditorView::on_mouse_down_event));
        d.dispatch<events::MouseMoveEvent>(bind_event_delegate(this, &SceneEditorView::on_mouse_move_event));
        d.dispatch<events::MouseScrollEvent>(bind_event_delegate(this, &SceneEditorView::on_mouse_scroll_event));
    }

    bool SceneEditorView::on_key_down_event(events::KeyDownEvent& e)
    {
        switch (e.key()) {
            using enum codex::Key;
            using enum codex::editor::GizmoMode;

            case Num1: gizmo_mode_ = Translation; return true;
            case Num2: gizmo_mode_ = Rotation; return true;
            case Num3: gizmo_mode_ = Scale; return true;
            default: break;
        }
        return false;
    }

    bool SceneEditorView::on_mouse_down_event(events::MouseDownEvent& e)
    {
        auto mouse_pos = Vector2{ ImGui::GetMousePos().x, ImGui::GetMousePos().y };
        mouse_pos.x -= viewport_bounds_[0].x;
        mouse_pos.y -= viewport_bounds_[0].y;
        mouse_pos.y = (viewport_bounds_[1] - viewport_bounds_[0]).y - mouse_pos.y;
        if (mouse_pos.x >= 0 && mouse_pos.y >= 0 && mouse_pos.x <= viewport_size_.x &&
            mouse_pos.y <= viewport_size_.y) {
            auto& d = descriptor_;
            if (d->selected_entity.entity && d->selected_entity.entity.has_component<TilemapComponent>()) {
                if (Input::is_mouse_down(Mouse::LeftMouse)) {
                    auto& tmc    = d->selected_entity.entity.get_component<TilemapComponent>();
                    auto& camera = Editor::get_viewport_camera();

                    // Vector conversion fiesta
                    const auto camera_dim =
                        Vector3f{ camera.width() * camera.pan(), camera.height() * camera.pan(), 0.0f };
                    auto tile_pos = scene::Camera::screen_coordinates_to_world(camera, mouse_pos, camera.pos());
                    tile_pos =
                        util::snap(tile_pos, Vector3f{ tmc.grid_size, 1.0f }) + Vector3f{ tmc.grid_size / 2.0f, 0.0f };
                    if (tmc.current_state == TilemapComponent::State::Brush) {
                        tmc.add_tile(tile_pos, tmc.current_tile);
                    } else if (tmc.current_state == TilemapComponent::State::Erase) {
                        tmc.remove_tile(tile_pos);
                    }
                    return true;
                }
            }
        }
        return false;
    }

    bool SceneEditorView::on_mouse_move_event(events::MouseMoveEvent& e)
    {
        auto mouse_pos = Vector2{ ImGui::GetMousePos().x, ImGui::GetMousePos().y };
        mouse_pos.x -= viewport_bounds_[0].x;
        mouse_pos.y -= viewport_bounds_[0].y;
        mouse_pos.y = (viewport_bounds_[1] - viewport_bounds_[0]).y - mouse_pos.y;
        if (mouse_pos.x >= 0 && mouse_pos.y >= 0 && mouse_pos.x <= viewport_size_.x &&
            mouse_pos.y <= viewport_size_.y) {
            auto& d = descriptor_;

            if (Input::is_mouse_down(Mouse::MiddleMouse)) {
                if (Input::is_mouse_dragging()) {
                    auto&      camera = Editor::get_viewport_camera();
                    const auto vec    = Vector2f{ Input::mouse_delta_x(), Input::mouse_delta_y() * -1.0f };
                    camera.set_pos(camera.pos() + util::to_vec3f(vec) * camera.pan());
                    return true;
                }
            } else if (d->selected_entity.entity && d->selected_entity.entity.has_component<TilemapComponent>()) {
                if (Input::is_mouse_down(Mouse::LeftMouse)) {
                    auto& tmc    = d->selected_entity.entity.get_component<TilemapComponent>();
                    auto& camera = Editor::get_viewport_camera();

                    // Vector conversion fiesta
                    const auto camera_dim =
                        Vector3f{ camera.width() * camera.pan(), camera.height() * camera.pan(), 0.0f };
                    auto tile_pos = scene::Camera::screen_coordinates_to_world(camera, mouse_pos, camera.pos());
                    tile_pos =
                        util::snap(tile_pos, Vector3f{ tmc.grid_size, 1.0f }) + Vector3f{ tmc.grid_size / 2.0f, 0.0f };
                    if (tmc.current_state == TilemapComponent::State::Brush) {
                        tmc.add_tile(tile_pos, tmc.current_tile);
                    } else if (tmc.current_state == TilemapComponent::State::Erase) {
                        tmc.remove_tile(tile_pos);
                    }
                    return true;
                }
            }
        }
        return false;
    }

    bool SceneEditorView::on_mouse_scroll_event(events::MouseScrollEvent& e)
    {
        auto mouse_pos = Vector2{ ImGui::GetMousePos().x, ImGui::GetMousePos().y };
        mouse_pos.x -= viewport_bounds_[0].x;
        mouse_pos.y -= viewport_bounds_[0].y;
        mouse_pos.y = (viewport_bounds_[1] - viewport_bounds_[0]).y - mouse_pos.y;
        if (mouse_pos.x >= 0 && mouse_pos.y >= 0 && mouse_pos.x <= viewport_size_.x &&
            mouse_pos.y <= viewport_size_.y) {
            auto& camera = Editor::get_viewport_camera();
            camera.set_pan(camera.pan() + e.offset_y() * -0.05f);
            return true;
        }
        return false;
    }

    void SceneEditorView::compile_project()
    {
        auto& d = descriptor_;

        if (d->compilation_state.load() == CompilationState::Compiling)
            return;

        if (NBMan::instance_loaded())
            NBMan::unload();

        d->compilation_state.store(CompilationState::Compiling);

        sys::ProcessInfo p_info;

#ifdef CX_PLATFORM_WINDOWS
        p_info.command = "python scripts/build.py --preset=windows-llvm-any-debug";
#elif defined(CX_PLATFORM_LINUX)
        p_info.command = "python scripts/build.py --preset=linux-any-debug";
#elif defined(CX_PLATFORM_OSX)
        p_info.command = "python scripts/build.py --preset=osx-any-debug";
#endif
        p_info.on_exit = [this](i32 exitCode)
        {
            auto& d = descriptor_;
            if (exitCode == 0) {
                d->pending_nb_load.store(true);
                d->compilation_state.store(CompilationState::Succeeded);
                ConsoleMan::append_message("-- Script build finished.");
            } else {
                d->compilation_state.store(CompilationState::Failed);
                Engine::error("Failed to compile project.");
            }
        };

        p_info.redirect_stdout = true;
        p_info.redirect_stderr = true;

        const auto redirector = [](const char* buffer, usize len)
        { ConsoleMan::append_message(std::string(buffer, len)); };

        ConsoleMan::append_message("-- Script build started.");
        auto proc                  = sys::Process::create(std::move(p_info));
        proc->on_out_data_received = redirector;
        proc->on_err_data_received = redirector;
        proc->launch();
    }

    void SceneEditorView::on_scene_play() noexcept
    {
        auto& d = descriptor_;
        d->selected_entity.deselect();
        d->runtime_scene = Shared<Scene>::make();
        d->editor_scene->copy_to(*d->runtime_scene);
        d->active_scene = d->runtime_scene;
        d->active_scene.lock()->set_state(Scene::State::Play);
        d->active_scene.lock()->on_runtime_start();
    }

    void SceneEditorView::on_scene_simulate() noexcept
    {
        auto& d = descriptor_;
        d->selected_entity.deselect();
        d->runtime_scene = Shared<Scene>::make();
        d->editor_scene->copy_to(*d->runtime_scene);
        d->active_scene = d->runtime_scene;
        d->active_scene.lock()->set_state(Scene::State::Simulate);
        d->active_scene.lock()->on_simulation_start();
    }

    void SceneEditorView::on_scene_stop() noexcept
    {
        auto& d     = descriptor_;
        auto  scene = d->active_scene.lock();
        if (scene->state() == Scene::State::Play)
            d->active_scene.lock()->on_runtime_stop();
        else
            scene->on_simulation_stop();

        d->runtime_scene.reset();
        d->active_scene = d->editor_scene;

        // Just in case when we select an entity during runtime.
        d->selected_entity.entity = Entity::none();
    }

    void SceneEditorView::initialize_panel(EditorPanel& panel) const noexcept
    {
        panel.on_init();
    }

    void SceneEditorView::visualize_colliders() const noexcept
    {
        CX_DEBUG_PROFILE_SCOPE()

        const auto& d = descriptor_;

        const auto box_colliders = d->active_scene.lock()->get_all_entities_with_component<BoxCollider2DComponent>();
        for (const auto& e : box_colliders) {
            const auto& bc = e.get_component<BoxCollider2DComponent>();
            const auto& tc = e.get_component<TransformComponent>();
            debug_draw_.draw_rect_2d({ bc.offset.x + tc.position.x, bc.offset.y + tc.position.y,
                                       bc.size.x * tc.scale.x * 2.0f, bc.size.y * tc.scale.y * 2.0f },
                                     tc.rotation.z);
        }

        const auto circle_colliders =
            d->active_scene.lock()->get_all_entities_with_component<CircleCollider2DComponent>();
        for (const auto& e : circle_colliders) {
            const auto& cc = e.get_component<CircleCollider2DComponent>();
            const auto& tc = e.get_component<TransformComponent>();
            debug_draw_.draw_circle_2d(Vector3f{ cc.offset, 0.0f } + tc.position, cc.radius * tc.scale.x * tc.scale.y,
                                       tc.rotation.z);
        }
    }

    void SceneEditorView::load_project(const std::filesystem::path cxproj)
    {
        auto& d = descriptor_;

        unload_project();

        d->current_project_path = cxproj;
        d->current_project_path = d->current_project_path.parent_path();

        // NOTE: I do not like this.
        stdfs::current_path(d->current_project_path);

        // TODO: Regarding to UnloadScriptModule(), LoadScriptModule() and CompileProject() here:
        // Some projects might just not use C++ for scripting (in the future when add C#
        // support) so make sure to not always force load the C++ script module. For now I will
        // leave this as is because there's only C++ support.

        d->editor_scene.reset(new Scene);

#ifdef CX_PLATFORM_LINUX
        d->script_module_path = d->current_project_path / stdfs::path("lib/libNBMan.so");
#elif defined(CX_PLATFORM_WINDOWS)
        d->script_module_path = d->current_project_path / stdfs::path("lib/NBMan.dll");
#elif defined(CX_PLATFORM_OSX)
        d->script_module_path = d->current_project_path / stdfs::path("lib/libNBMan.dylib");
#endif
        d->active_scene = d->editor_scene;

        // Load FMOD banks
        {
            const auto audio_dir = d->current_project_path / "assets/audio/Build";
            if (stdfs::exists(audio_dir)) {
                for (const auto& entry : stdfs::recursive_directory_iterator(audio_dir)) {
                    if (entry.is_regular_file() && entry.path().extension() == ".bank") {
                        ax::AudioManager::load_bank(entry.path());
                        Engine::info("Loaded FMOD bank: {}", entry.path().string());
                    }
                }
            }
        }

        SerializationManager::load_from_file(*d->editor_scene, cxproj);

        auto project_mount = Shared<fs::DiskMount>::make(d->current_project_path / "assets", 0);
        d->vfs->mount(std::move(project_mount), "/editor/project/assets", true);

        // Don't await_sync!
        AssetRegistry* reg = new AssetRegistry;
        {
            auto watch = dbg::profile_scope();
            reg->scan(*d->vfs, "/editor/project");
            info("AssetRegistry::scan() took {}", watch.elapsed_as<std::milli, f32>());
        }

        // Kick off async compilation; NBMan will be loaded on the main thread
        // once the build succeeds (via pendingNBLoad flag checked in on_update).
        compile_project();
    }

    void SceneEditorView::unload_project()
    {
        if (NBMan::instance_loaded())
            NBMan::unload(!is_shutting_down_);

        descriptor_->reset();
    }

    void SceneEditorView::render_grid(gfx::DebugDraw& renderer, const scene::EditorCamera& camera,
                                      const GridRendererComponent& c) noexcept
    {
        CX_DEBUG_PROFILE_SCOPE()

        const auto camera_dim = Vector2{ camera.width() * camera.pan(), camera.height() * camera.pan() };
        const auto camera_pos = camera.pos() - Vector3f{ camera_dim / 2, 0.0f };
        const auto start_pos  = glm::ceil(camera_pos / Vector3f{ c.cell_size, 1.0f }) * Vector3f{ c.cell_size, 0.0f };
        const auto count      = camera_dim / Vector2{ c.cell_size } + 1;

        for (auto i = 0; i < count.x; ++i) {
            renderer.draw_line_2d({ start_pos.x + i * c.cell_size.x, camera_pos.y },
                                  { start_pos.x + i * c.cell_size.x, camera_pos.y + camera_dim.y }, c.colour);
        }

        for (auto i = 0; i < count.y; ++i) {
            renderer.draw_line_2d({ camera_pos.x, start_pos.y + i * c.cell_size.y },
                                  { camera_pos.x + camera_dim.x, start_pos.y + i * c.cell_size.y }, c.colour);
        }
    }

    void SceneEditorView::draw_vec3_control(const char* label, Vector3f& values, const f32 columnWidth, const f32 speed,
                                            const f32 resetValue)
    {
        ImGuiIO& io        = ImGui::GetIO();
        auto     bold_font = io.Fonts->Fonts[0];

        ImGui::PushID(label);

        if (label[0] != '#' && label[1] != '#') {
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

    void SceneEditorView::draw_vec2_control(const char* label, Vector2f& values, const f32 columnWidth, const f32 speed,
                                            const f32 resetValue)
    {
        ImGuiIO& io        = ImGui::GetIO();
        auto     bold_font = io.Fonts->Fonts[0];

        ImGui::PushID(label);

        if (label[0] != '#' && label[1] != '#') {
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
