#include "scene_editor_view.h"

#include <console_man.h>
#include <editor.h>
#include <editor_application.h>
#include <editor_project.h>
#include <nfd.h>

#include <engine/scene/public/prefab.h>
#include <launcher_settings.h>

#include "panels/asset_properties_view.h"
#include "panels/content_browser_view.h"
#include "panels/project_settings_view.h"
#include "panels/properties_view.h"
#include "panels/scene_hierarchy_view.h"
#include "panels/toolbar_view.h"

#include <imgui.h>
#include <imgui_internal.h>

namespace codex::editor {
    namespace stdfs = std::filesystem;

    namespace {
        constexpr std::string_view project_assets_vfs_root = "/edit/project/assets";

        opt<stdfs::path> project_asset_vfs_path(const stdfs::path& host_path, const stdfs::path& project_path)
        {
            std::error_code ec;
            const auto      assets_root = stdfs::weakly_canonical(project_path / "assets", ec);
            if (ec)
                return std::nullopt;

            auto selected_path = host_path;
            if (selected_path.extension().empty())
                selected_path.replace_extension(".cxscene");
            selected_path = stdfs::weakly_canonical(selected_path, ec);
            if (ec)
                return std::nullopt;

            const auto relative_path = stdfs::relative(selected_path, assets_root, ec);
            if (ec || relative_path.empty() || relative_path.is_absolute() || *relative_path.begin() == "..")
                return std::nullopt;

            return stdfs::path{ project_assets_vfs_root } / relative_path;
        }

        bool save_scene_to_vfs(fs::VirtualFilesystem& vfs, const Scene& scene, const stdfs::path& vfs_path)
        {
            auto handle = vfs.open(vfs_path.generic_string(),
                                   { fs::FileMode::Create | fs::FileMode::Trunc | fs::FileMode::Write });
            if (!handle)
                return false;

            const std::string json = SerializationManager::to_json(scene) + '\n';
            return handle->write(json.data(), json.size()) == json.size();
        }
    } // namespace

    void EditorPanelDeleter::operator()(EditorPanel* panel) noexcept
    { delete panel; }

    void SceneEditorView::on_attach()
    {
        descriptor_               = Shared<SceneEditorDescriptor>::from(new SceneEditorDescriptor{
            .project              = {},
            .active_scene         = {},
            .editor_scene         = Shared<Scene>::make(),
            .runtime_scene        = {},
            .current_scene_path   = {},
            .script_module_path   = {},
            .selected_entity      = {},
            .current_project_path = {},
            .current_project_file = {},
            .registry_state_mutex = {},
            .registry_state_cv    = {},
        });
        descriptor_->active_scene = descriptor_->editor_scene;

        // Panels
        this->attach_panel<SceneHierarchyView>();
        this->attach_panel<PropertiesView>();
        this->attach_panel<ToolbarView>();
        this->attach_panel<ContentBrowserView>();
        this->attach_panel<AssetPropertiesView>();

        load_outline_shader();

        opengl::FrameBufferProperties props;
        props.attachments = {
            { .format = opengl::TextureFormat::RGBA8 },
            { .format = opengl::TextureFormat::RedInt32 },
            { .format = opengl::TextureFormat::Depth24Stencil8 },
        };

        // TODO: This is the scene render resolution so you should not hard code this.
        props.width  = 1920;
        props.height = 1080;
        framebuffer_ = Box<opengl::FrameBuffer>::make(props);

        // EditorLayer::GetCamera().SetProjectionType(scene::Camera::ProjectionType::Perspective);

        // TODO: We should z-index using the depth buffer in the future so that we can also have 3d elements
        // instead of disabling the depth buffer and z-index'ing by sorting our render batches.
        // glEnable(GL_DEPTH_TEST);
        // glDepthFunc(GL_LESS);

        // TODO: Remove this hardcoded path
        std::string_view val = std::getenv("CX_DEFAULT_LOAD_PROJECT");
        if (!val.empty()) {
            load_project(std::string{ val } + "/template.cxproj");
        }
    }

    void SceneEditorView::on_detach()
    { unload_project(); }

    void SceneEditorView::on_update(const f32 dt)
    {
        auto& d     = descriptor_;
        auto  scene = d->active_scene.lock();

        // Load NBMan on main thread once async compilation succeeds,
        // then attach any pending scripts (from deserialization or recompilation).
        if (d->pending_nb_load.exchange(false)) {
            load_script_module(*scene);
            ConsoleMan::append_message("-- Script module load finished.");

            // Attach scripts that were pending because the module wasn't loaded yet
            // (e.g. the scene deserialized before the first compile finished), so
            // they show up in the inspector without having to enter play mode.
            scene->attach_pending_behaviours();
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

        viewport_resize();

        gfx::Renderer::set_clear_colour(0.2f, 0.2f, 0.2f, 1.0f);
        gfx::Renderer::clear();

        debug_draw_.begin(Editor::viewport_camera());

        switch (scene->state()) {
            case Scene::State::Edit: {
                CX_DEBUG_PROFILE_SCOPE("on_update::on_editor_update")

                if (d->selected_entity.entity) {
                    if (d->selected_entity.entity.has_component<GridRendererComponent>()) {
                        render_grid(debug_draw_, Editor::viewport_camera(),
                                    d->selected_entity.entity.get_component<GridRendererComponent>());
                    }
                }

                if (d->visualize_lines)
                    visualize_lines();

                gfx::Renderer::stencil_test(false);
                scene->on_editor_update(dt, Editor::viewport_camera());

                if (d->selected_entity.entity && d->outline_shader &&
                    d->selected_entity.entity.has_component<SpriteRendererComponent>()) {
                    const auto& tc  = d->selected_entity.entity.get_component<TransformComponent>();
                    const auto& src = d->selected_entity.entity.get_component<SpriteRendererComponent>();

                    gfx::Renderer::stencil_mask(0xff);
                    gfx::Renderer::stencil_op(opengl::Enum::Keep, opengl::Enum::Keep, opengl::Enum::Replace);
                    gfx::Renderer::stencil_fn(opengl::Enum::Always, 1, 0xff);
                    gfx::Renderer::stencil_test(true);
                    gfx::Renderer::colour_mask(false, false, false, false);

                    // TODO: This is TRASH but I don't care about it right now.
                    //  render d->selected_entity ONLY!
                    {
                        if (const auto& s = src.sprite(); s) {
                            gfx::BatchRenderer2D::begin(Editor::viewport_camera());

                            const auto size = s.size();
                            // The scaling we do here is the Sprite's size.

                            // TODO: Get rid of this and optimize this?
                            const auto transform =
                                tc.world_mat() * glm::scale(glm::identity<mat4>(), { size.x, size.y, 1.0f });
                            gfx::BatchRenderer2D::render_sprite(src.sprite(), transform,
                                                                static_cast<i32>(d->selected_entity.entity));

                            gfx::BatchRenderer2D::end();
                        }
                    }

                    d->outline_shader->bind();
                    d->outline_shader->set_uniform_4f("u_outline_colour", d->select_colour.x, d->select_colour.y,
                                                      d->select_colour.z, d->select_colour.w);
                    d->outline_shader->set_uniform_1f("u_outline_size", d->outline_border_size);
                    gfx::Renderer::stencil_mask(0x00);
                    gfx::Renderer::stencil_fn(opengl::Enum::NotEqual, 1, 0xff);
                    gfx::Renderer::colour_mask(true, true, true, true);

                    // TODO: This is also TRASH but I don't care about it right now.
                    // render outline!
                    {
                        if (const auto& s = src.sprite(); s) {
                            gfx::BatchRenderer2D::begin(Editor::viewport_camera());

                            const auto size = s.size();
                            // The scaling we do here is the Sprite's size.

                            // TODO: Get rid of this and optimize this?
                            const auto transform =
                                tc.world_mat() * glm::scale(glm::identity<mat4>(), { size.x, size.y, 1.0f });
                            gfx::BatchRenderer2D::render_sprite(src.sprite(), transform,
                                                                static_cast<i32>(d->selected_entity.entity));

                            gfx::BatchRenderer2D::end(d->outline_shader.get());
                        }
                    }

                    gfx::Renderer::stencil_mask(0xff);
                    gfx::Renderer::stencil_test(false);
                    d->outline_shader->unbind();
                }
                break;
            }
            case Scene::State::Play: {
                CX_DEBUG_PROFILE_SCOPE("on_update::on_runtime_update")
                viewport_resize();
                scene->on_runtime_update(dt);
                break;
            }
            case Scene::State::Simulate: {
                CX_DEBUG_PROFILE_SCOPE("on_update::on_simulation_update")

                if (d->visualize_lines)
                    visualize_lines();
                scene->on_simulation_update(dt, Editor::viewport_camera());
                break;
            }
        }

        auto [mx, my] = ImGui::GetMousePos();
        mx -= viewport_bounds_[0].x;
        my -= viewport_bounds_[0].y;
        const vec2 viewport_size = viewport_bounds_[1] - viewport_bounds_[0];
        const i32  mouse_x       = (i32)mx;
        const i32  mouse_y       = (i32)my;

        if (Input::is_mouse_down(Mouse::LeftMouse) && mouse_x >= 0 && mouse_y >= 0 && mouse_x <= (i32)viewport_size.x &&
            mouse_y <= (i32)viewport_size.y && !gizmo_active_) {
            if (d->active_scene.lock()->state() != Scene::State::Play && !gizmo_active_ &&
                (!d->selected_entity.entity || !d->selected_entity.entity.has_component<TilemapComponent>())) {
                vec2 scale = { framebuffer_->properties().width / viewport_size.x,
                               framebuffer_->properties().height / viewport_size.y };
                vec2 pos   = { mouse_x, viewport_size.y - mouse_y };
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
            panel->on_pre_update(dt);
    }

    void SceneEditorView::on_imgui_render()
    {
        CX_DEBUG_PROFILE_SCOPE("SceneEditorView::on_imgui_render")

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
            ImGui::DockSpaceOverViewport(0);

            static bool show_demo_window = true;
            ImGui::ShowDemoWindow(&show_demo_window);
        }

        // Profiler window.
        {
            ImGui::Begin("Profiler");
            for (auto& e : dbg::Profiler::get_profilers()) {
                const auto info = e.second.info();
                const auto dur  = e.second.elapsed_as<std::milli, f32>().count();
                ImGui::Text("%s: %fms", info.name.c_str(), static_cast<double>(dur));
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
                        sys::ProcessInfo p_info;

#ifdef CX_PLATFORM_WINDOWS
                        p_info.command = "cmake --preset windows-llvm-any-debug --clear";
#elif defined(CX_PLATFORM_LINUX)
                        p_info.command = "./build.py --preset linux-any-debug --clear";
#elif defined(CX_PLATFORM_OSX)
                        p_info.command = "./build.py --preset osx-any-debug --clear";
#endif
                        p_info.on_exit = []([[maybe_unused]] i32 exitCode)
                        {
                            // TODO: Scene should also be thread safe since this callback is being called from a
                            // different thread.
                            // Scene::LoadScriptModule(descriptor_->script_module_path); // TODO: COME BACK
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
                    if (ImGui::MenuItem("Package & Export Project", "Ctrl+Shift+E")) {
                        if (d->registry_state == AssetRegistryState::Succeeded) {
                            cook_and_export_project();
                        }
                    }
                    if (ImGui::MenuItem("Save Scene", "Ctrl+Shift+S")) {
                        if (d->registry_state == AssetRegistryState::Succeeded) {
                            if (d->current_scene_path.empty()) {
                                nfdu8char_t*      outPath      = nullptr;
                                nfdu8filteritem_t filters[]    = { { "Codex Scene", "cxscene,cxsc" } };
                                const std::string default_path = (d->current_project_path / "assets").generic_string();
                                const nfdresult_t result =
                                    NFD_SaveDialogU8(&outPath, filters, 1, default_path.c_str(), "scene0.cxscene");
                                if (result == NFD_OKAY) {
                                    const auto vfs_path =
                                        project_asset_vfs_path(stdfs::path{ outPath }, d->current_project_path);
                                    NFD_FreePathU8(outPath);

                                    if (!vfs_path) {
                                        log(Error, "Scenes must be saved inside the project's assets directory");
                                    } else if (save_scene_to_vfs(EditorApplication::vfs(), *d->active_scene.lock(),
                                                                 *vfs_path)) {
                                        d->selected_entity.deselect();
                                        d->current_scene_path = *vfs_path;
                                    } else {
                                        log(Error, "Failed to save scene to VFS path '{}'", vfs_path->generic_string());
                                    }
                                } else if (result == NFD_ERROR)
                                    log(Error, "Failed to open scene save dialog: {}", NFD_GetError());
                            } else {
                                if (!save_scene_to_vfs(EditorApplication::vfs(), *d->active_scene.lock(),
                                                       d->current_scene_path))
                                    log(Error, "Failed to save scene to VFS path '{}'",
                                        d->current_scene_path.generic_string());
                            }
                        }
                    }
                    if (ImGui::MenuItem("Save", "Ctrl+S")) {
                        // Save everything: the active scene to its file, then the project itself.
                        if (d->registry_state == AssetRegistryState::Succeeded) {
                            d->selected_entity.deselect();

                            if (!d->current_scene_path.empty()) {
                                if (!save_scene_to_vfs(EditorApplication::vfs(), *d->active_scene.lock(),
                                                       d->current_scene_path))
                                    log(Error, "Failed to save scene to VFS path '{}'",
                                        d->current_scene_path.generic_string());
                            } else {
                                log(Warn, "No scene file to save to; use 'Save Scene' to pick a location");
                            }

                            if (!d->current_project_file.empty())
                                SerializationManager::save_to_file(d->project, d->current_project_file,
                                                                   SerializationManager::Format::Json);
                            else
                                log(Error, "No project file to save to");
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
            auto                 active_scene = d->active_scene.lock();
            scene::EditorCamera& camera       = Editor::viewport_camera();

            ImGui::Begin("Viewport");
            const auto viewport_min_region = ImGui::GetWindowContentRegionMin();
            const auto viewport_max_region = ImGui::GetWindowContentRegionMax();
            const auto viewport_offset     = ImGui::GetWindowPos();
            viewport_bounds_[0]            = { viewport_min_region.x + viewport_offset.x,
                                               viewport_min_region.y + viewport_offset.y };
            viewport_bounds_[1]            = { viewport_max_region.x + viewport_offset.x,
                                               viewport_max_region.y + viewport_offset.y };

            auto current_viewport_window_size = ImGui::GetContentRegionAvail();
            viewport_size_                    = vec2{ current_viewport_window_size.x, current_viewport_window_size.y };
            ImGui::Image(reinterpret_cast<ImTextureID>(framebuffer_->colour_attachment_id_at(0)),
                         current_viewport_window_size, { 0, 1 }, { 1, 0 });

            if (ImGui::BeginDragDropTarget()) {
                if (const ImGuiPayload* payload = ImGui::AcceptDragDropPayload("CX_ASSET")) {
                    const UUID uuid = *static_cast<const UUID*>(payload->Data);
                    if (const AssetMetadata* meta = AssetManager::registry().asset_metadata(uuid);
                        meta && meta->type == "Prefab") {
                        if (auto prefab = AssetManager::load<scene::Prefab>(uuid); prefab) {
                            Entity entity = active_scene->instantiate_prefab(*prefab);

                            auto mouse_pos = ivec2{ ImGui::GetMousePos().x, ImGui::GetMousePos().y };
                            mouse_pos.x -= viewport_bounds_[0].x;
                            mouse_pos.y -= viewport_bounds_[0].y;
                            mouse_pos.y = (viewport_bounds_[1] - viewport_bounds_[0]).y - mouse_pos.y;
                            if (mouse_pos.x >= 0 && mouse_pos.y >= 0 && mouse_pos.x <= viewport_size_.x &&
                                mouse_pos.y <= viewport_size_.y) {
                                entity.get_component<TransformComponent>().position =
                                    scene::Camera::screen_coordinates_to_world(camera, mouse_pos, camera.pos());
                            }

                            d->selected_entity.select(entity);
                        } else {
                            log(Error, "Failed to load dropped prefab asset {}", uuid);
                        }
                    }
                }
                ImGui::EndDragDropTarget();
            }

            viewport_focused_ = ImGui::IsWindowFocused();
            viewport_hovered_ = ImGui::IsWindowHovered();

            // Engine::get().get_imgui_layer()->block_events(!viewport_focused_);

            // Guizmo
            if (active_scene->state() != Scene::State::Play) {
                if (d->selected_entity.entity && d->selected_entity.entity.has_component<TransformComponent>()) {
                    ImGuizmo::SetOrthographic(true);
                    ImGuizmo::SetDrawlist();

                    ImGuizmo::SetRect(viewport_bounds_[0].x, viewport_bounds_[0].y,
                                      viewport_bounds_[1].x - viewport_bounds_[0].x,
                                      viewport_bounds_[1].y - viewport_bounds_[0].y);

                    // Camera
                    auto proj_mat = camera.projection_matrix();
                    auto view_mat = camera.view_matrix();

                    auto& tc        = d->selected_entity.entity.get_component<TransformComponent>();
                    auto  transform = tc.world_mat();

                    ImGuizmo::Manipulate(glm::value_ptr(view_mat), glm::value_ptr(proj_mat),
                                         (ImGuizmo::OPERATION)gizmo_mode_, ImGuizmo::MODE::LOCAL,
                                         glm::value_ptr(transform));

                    gizmo_active_ = ImGuizmo::IsOver();

                    if (gizmo_active_ && ImGuizmo::IsUsing()) {
                        const bool simulating = d->active_scene.lock()->state() != Scene::State::Edit;
                        if (simulating && d->selected_entity.entity.has_component<RigidBody2DComponent>()) {
                            // Drive the Rigid Body rather than the transform directly since the transform
                            // is owned by the physics engine in Rigid Bodies.
                            auto& rb2d = d->selected_entity.entity.get_component<RigidBody2DComponent>();

                            struct transform final_transform;
                            math::transform_decompose(transform, final_transform.position, final_transform.rotation,
                                                      final_transform.scale);
                            final_transform.rotation = glm::degrees(final_transform.rotation);
                            rb2d.set_transform(final_transform);
                        } else {
                            // Convert back from gizmo world TRS to parent relative local TRS
                            auto local_new = transform;
                            if (d->selected_entity.entity.has_component<HierarchyComponent>()) {
                                auto& hc = d->selected_entity.entity.get_component<HierarchyComponent>();
                                if (hc.parent)
                                    local_new = glm::inverse(hc.parent.transform().world_mat()) * transform;
                            }

                            vec3 rotation;
                            codex::math::transform_decompose(local_new, tc.position, rotation, tc.scale);
                            tc.rotation = glm::degrees(rotation);
                        }
                    }
                }
            }

            ImGui::End();
        }

        // Render info
        {
            ImGui::Begin("RHI Info");
            // switch (Engine::RHI::current_api()) {
            //     case GraphicsAPI::OpenGL:
            //     {
            //         if (ImGui::TreeNodeEx("OpenGL"))
            //     }
            //     break;
            //     case GraphicsAPI::Vulkan:
            //     {
            //         if (ImGui::TreeNodeEx("Vulkan"))
            //     }
            //     break;
            // }

            if (ImGui::CollapsingHeader("OpenGL", ImGuiTreeNodeFlags_DefaultOpen)) {
                ImGui::Text("FPS: %u", Engine::fps());
                ImGui::Text("Delta time: %f", static_cast<double>(Engine::delta()));
                ImGui::Text("Batch count: %zu", gfx::BatchRenderer2D::batch_count());
                ImGui::Text("Total quad count: %zu", gfx::BatchRenderer2D::quad_count());
                ImGui::Text("Command Queue Size: 0");
                // ImGui::TreePop();
            }
            ImGui::End();
        }

        auto block_events = !viewport_focused_;

        // Render our panels
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

        Engine::get().imgui_layer()->block_events(block_events);

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
        auto& d = descriptor_;
        switch (e.key()) {
            using enum codex::Key;
            using enum codex::editor::GizmoMode;

            case Num1: gizmo_mode_ = Translation; return true;
            case Num2: gizmo_mode_ = Rotation; return true;
            case Num3: gizmo_mode_ = Scale; return true;
            case V: d->visualize_lines = !d->visualize_lines;
            default: break;
        }
        return false;
    }

    bool SceneEditorView::on_mouse_down_event([[maybe_unused]] events::MouseDownEvent& e)
    {
        auto mouse_pos = ivec2{ ImGui::GetMousePos().x, ImGui::GetMousePos().y };
        mouse_pos.x -= viewport_bounds_[0].x;
        mouse_pos.y -= viewport_bounds_[0].y;
        mouse_pos.y = (viewport_bounds_[1] - viewport_bounds_[0]).y - mouse_pos.y;
        if (mouse_pos.x >= 0 && mouse_pos.y >= 0 && mouse_pos.x <= viewport_size_.x &&
            mouse_pos.y <= viewport_size_.y) {
            auto& d = descriptor_;
            if (d->selected_entity.entity && d->selected_entity.entity.has_component<TilemapComponent>()) {
                if (Input::is_mouse_down(Mouse::LeftMouse)) {
                    auto& tmc    = d->selected_entity.entity.get_component<TilemapComponent>();
                    auto& camera = Editor::viewport_camera();

                    // Vector conversion fiesta
                    auto tile_pos = scene::Camera::screen_coordinates_to_world(camera, mouse_pos, camera.pos());
                    tile_pos = util::snap(tile_pos, vec3{ tmc.grid_size, 1.0f }) + vec3{ tmc.grid_size / 2.0f, 0.0f };
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

    bool SceneEditorView::on_mouse_move_event([[maybe_unused]] events::MouseMoveEvent& e)
    {
        auto mouse_pos = ivec2{ ImGui::GetMousePos().x, ImGui::GetMousePos().y };
        mouse_pos.x -= viewport_bounds_[0].x;
        mouse_pos.y -= viewport_bounds_[0].y;
        mouse_pos.y = (viewport_bounds_[1] - viewport_bounds_[0]).y - mouse_pos.y;
        if (mouse_pos.x >= 0 && mouse_pos.y >= 0 && mouse_pos.x <= viewport_size_.x &&
            mouse_pos.y <= viewport_size_.y) {
            auto& d = descriptor_;

            if (Input::is_mouse_down(Mouse::MiddleMouse)) {
                if (Input::is_mouse_dragging()) {
                    auto&      camera = Editor::viewport_camera();
                    const auto vec = vec3{ Input::mouse_delta_x(), Input::mouse_delta_y() * -1.0f, .0f } * camera.pan();
                    if (vec.x <= 10 && vec.y <= 10)
                        camera.set_pos(camera.pos() + vec);
                    return true;
                }
            } else if (d->selected_entity.entity && d->selected_entity.entity.has_component<TilemapComponent>()) {
                if (Input::is_mouse_down(Mouse::LeftMouse)) {
                    auto& tmc    = d->selected_entity.entity.get_component<TilemapComponent>();
                    auto& camera = Editor::viewport_camera();

                    auto tile_pos = scene::Camera::screen_coordinates_to_world(camera, mouse_pos, camera.pos());
                    tile_pos = util::snap(tile_pos, vec3{ tmc.grid_size, 1.0f }) + vec3{ tmc.grid_size / 2.0f, 0.0f };
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
        auto mouse_pos = ivec2{ ImGui::GetMousePos().x, ImGui::GetMousePos().y };
        mouse_pos.x -= viewport_bounds_[0].x;
        mouse_pos.y -= viewport_bounds_[0].y;
        mouse_pos.y = (viewport_bounds_[1] - viewport_bounds_[0]).y - mouse_pos.y;
        if (mouse_pos.x >= 0 && mouse_pos.y >= 0 && mouse_pos.x <= viewport_size_.x &&
            mouse_pos.y <= viewport_size_.y) {
            auto& camera = Editor::viewport_camera();

            // TODO: These should be Editor global
            constexpr auto camera_pan_min = 0.005f;
            constexpr auto camera_pan_max = 8.5f;

            const auto new_pan = std::clamp(camera.pan() + e.offset_y() * -0.05f, camera_pan_min, camera_pan_max);
            camera.set_pan(new_pan);
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
                error("Failed to compile project.");
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

    void SceneEditorView::load_script_module(Scene& scene)
    {
        auto& d = descriptor_;

        // Load a uniquely named copy of the module instead of the build output.
        // Loading the build output in place breaks hot-reload: Windows locks a
        // loaded DLL so the next build could not overwrite it, and dlopen would
        // silently reuse a stale image if the file is replaced at the same path.
        const stdfs::path tmp_dir = d->current_project_path / "tmp";

        std::error_code ec;
        stdfs::create_directories(tmp_dir, ec);

        // Best-effort cleanup of copies from previous loads; a still-loaded (and
        // on Windows, locked) copy simply fails to delete and is skipped.
        {
            std::vector<stdfs::path> stale;
            for (const auto& entry : stdfs::directory_iterator(tmp_dir, ec))
                if (entry.path().filename().string().starts_with("nb_"))
                    stale.push_back(entry.path());
            for (const auto& path : stale)
                stdfs::remove(path, ec);
        }

        const auto        stamp = std::chrono::system_clock::now().time_since_epoch().count();
        const stdfs::path tmp_module =
            tmp_dir / fmt::format("nb_{}_{}", stamp, d->script_module_path.filename().string());

        stdfs::copy_file(d->script_module_path, tmp_module, stdfs::copy_options::overwrite_existing, ec);
        if (ec) {
            error("Failed to copy script module to '{}': {}", tmp_module.string(), ec.message());
            NBMan::load(d->script_module_path, scene); // degrade to loading in place
            return;
        }

        NBMan::load(tmp_module, scene);
    }

    void SceneEditorView::on_scene_play() noexcept
    {
        auto& d = descriptor_;
        d->selected_entity.deselect();
        d->runtime_scene = Shared<Scene>::make();
        d->editor_scene->clone_via_serialization(*d->runtime_scene);
        // d->editor_scene->copy_to(*d->runtime_scene);

        d->active_scene   = d->runtime_scene;
        auto active_scene = d->active_scene.lock();
        cxensure(active_scene, "active_scene should not be null right after being set from runtime_scene");

        active_scene->set_state(Scene::State::Play);
        active_scene->on_runtime_start();
    }

    void SceneEditorView::on_scene_simulate() noexcept
    {
        auto& d = descriptor_;
        d->selected_entity.deselect();
        d->runtime_scene = Shared<Scene>::make();
        d->editor_scene->clone_via_serialization(*d->runtime_scene);
        // d->editor_scene->copy_to(*d->runtime_scene);

        d->active_scene   = d->runtime_scene;
        auto active_scene = d->active_scene.lock();
        cxensure(active_scene, "active_scene should not be null right after being set from runtime_scene");

        active_scene->set_state(Scene::State::Simulate);
        active_scene->on_simulation_start();
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
    { panel.on_init(); }

    void SceneEditorView::visualize_lines() const noexcept
    {
        CX_DEBUG_PROFILE_SCOPE("SceneEditorView::visualize_lines")

        const auto&                d      = descriptor_;
        const scene::EditorCamera& camera = Editor::viewport_camera();

        // Visualize colliders
        {
            const auto box_colliders = d->active_scene.lock()->entities_with_component<BoxCollider2DComponent>();
            for (const auto& e : box_colliders) {
                const auto&     bc             = e.get_component<BoxCollider2DComponent>();
                const auto&     tc             = e.transform();
                const transform w_tr           = tc.world_transform();
                const vec2      rotated_offset = glm::rotate(bc.offset, math::to_radf(w_tr.rotation.z));
#if 0
                debug_draw_.draw_half_circle_2d({}, 50.0f, 90.0f);
#endif
                debug_draw_.draw_rect_2d({ rotated_offset.x + w_tr.position.x, rotated_offset.y + w_tr.position.y,
                                           bc.size.x * w_tr.scale.x * 2.0f, bc.size.y * w_tr.scale.y * 2.0f },
                                         w_tr.rotation.z);
            }

            const auto circle_colliders = d->active_scene.lock()->entities_with_component<CircleCollider2DComponent>();
            for (const auto& e : circle_colliders) {
                const auto&     cc             = e.get_component<CircleCollider2DComponent>();
                const auto&     tc             = e.transform();
                const transform w_tr           = tc.world_transform();
                const vec2      rotated_offset = glm::rotate(cc.offset, math::to_radf(w_tr.rotation.z));
                debug_draw_.draw_circle_2d(vec3{ rotated_offset, 0.0f } + w_tr.position,
                                           cc.radius * w_tr.scale.x * w_tr.scale.y, w_tr.rotation.z);
            }

            const auto rj2d_colliders = d->active_scene.lock()->entities_with_component<RevoluteJoint2DComponent>();
            for (const auto& e : rj2d_colliders) {
                auto&                     rj2d  = e.get_component<RevoluteJoint2DComponent>();
                const TransformComponent& trs_a = e.transform();

                const vec3 world_local_anchor_a = trs_a.world_mat() * vec4(rj2d.local_anchor_a, .0f, 1.0f);
                debug_draw_.draw_circle_2d(vec2(world_local_anchor_a), 4 * camera.pan());

                if (rj2d.enable_limit) {
                    Entity referenced = d->active_scene.lock()->entity_by_uuid(rj2d.body_b);
                    if (referenced) {
                        // Same extraction Scene::construct_physics_body uses to feed b2BodyDef::angle,
                        // so the gizmo reads the exact angles the solver sees.
                        const auto world_angle_deg = [](const mat4& m)
                        { return glm::degrees(glm::atan2(m[0].y, m[0].x)); };

                        // Box2D clamps jointAngle = angleB - angleA - referenceAngle to [lower, upper].
                        // bodyA is the referenced body, bodyB is the entity holding the component.
                        const f32 angle_a = world_angle_deg(referenced.transform().world_mat());
                        const f32 angle_b = world_angle_deg(trs_a.world_mat());

                        // referenceAngle is baked from the REST pose when the joint is created, so it
                        // cannot be recovered from the live transforms once the solver moves things.
                        // The editor scene still holds that rest pose (the runtime scene is a UUID-
                        // preserving clone of it), so read the reference straight out of it.
                        // While editing, the live pose IS the rest pose, so the reference falls out of
                        // the current angles directly.
                        f32 reference = angle_b - angle_a;
                        if (d->active_scene.lock()->state() != Scene::State::Edit) {
                            Shared<Scene> rest_scene = d->editor_scene; // copy, so access stays non-const
                            if (rest_scene) {
                                const Entity rest_self = rest_scene->entity_by_uuid(e.uuid());
                                const Entity rest_con  = rest_scene->entity_by_uuid(rj2d.body_b);
                                if (rest_self && rest_con)
                                    reference = world_angle_deg(rest_self.transform().world_mat()) -
                                                world_angle_deg(rest_con.transform().world_mat());
                            }
                        }

                        const f32  ref_world = angle_a + reference;
                        const f32  radius    = 25.0f * camera.pan();
                        const vec2 centre    = vec2(world_local_anchor_a);

                        // Limit cone: fixed to bodyA, so it only swings when the parent rotates.
                        debug_draw_.draw_arc_2d(centre, ref_world + rj2d.lower_angle, ref_world + rj2d.upper_angle,
                                                radius, 20, { 0.0f, 1.0f, 0.0f, 1.0f });

                        // Current-angle needle: fixed to bodyB, meets a cone edge exactly at the limit.
                        debug_draw_.draw_line_2d(centre,
                                                 centre + glm::rotate(vec2{ radius, 0.0f }, glm::radians(angle_b)),
                                                 { 1.0f, 1.0f, 0.0f, 1.0f });
                    }
                }

                Entity bodyb = d->active_scene.lock()->entity_by_uuid(rj2d.body_b);
                if (bodyb) {
                    const TransformComponent& trs_b = bodyb.transform();

                    const vec3 world_local_anchor_b = trs_b.world_mat() * vec4(rj2d.local_anchor_b, .0f, 1.0f);
                    debug_draw_.draw_circle_2d(vec2(world_local_anchor_b), 4 * camera.pan());
                    debug_draw_.draw_line_2d(vec2(world_local_anchor_a), vec2(world_local_anchor_b));
                }
            }
        }

        // Visualize camera
        {
            const auto cameras = d->active_scene.lock()->entities_with_component<CameraComponent>();
            for (const auto& e : cameras) {
                const auto&     tc   = e.get_component<TransformComponent>();
                const auto&     cc   = e.get_component<CameraComponent>();
                const transform w_tr = tc.world_transform();
                debug_draw_.draw_rect_2d(
                    { w_tr.position.x, w_tr.position.y, (f32)cc.camera.width(), (f32)cc.camera.height() },
                    w_tr.rotation.z);
            }
        }
    }

    void SceneEditorView::load_project(const std::filesystem::path cxproj)
    {
        auto& d = descriptor_;

        unload_project();

        if (!std::filesystem::exists(cxproj)) {
            log(Error, "{}: No such file or directory", cxproj.generic_string());
            return;
        }

        d->current_project_file = cxproj;
        d->current_project_path = cxproj;
        d->current_project_path = d->current_project_path.parent_path();

        // NOTE: I do not like this.
        stdfs::current_path(d->current_project_path);

        auto project_mount = Shared<fs::DiskMount>::make(d->current_project_path / "assets", 0);
        EditorApplication::vfs().mount(std::move(project_mount), std::string{ project_assets_vfs_root }, true);

        AssetManager::init(EditorApplication::vfs(), project_assets_vfs_root);

        d->registry_state = AssetRegistryState::Scanning;
        Engine::worker_pool().submit(
            [this]
            {
                std::scoped_lock guard{ descriptor_->registry_state_mutex };
                auto             watch = dbg::profile_scope();
                AssetManager::registry().scan_async(std::string{ project_assets_vfs_root }).await_sync();
                log(Info, "AssetRegistry::scan() took {}ms", watch.elapsed_as<std::milli, f32>().count());
                descriptor_->registry_state       = AssetRegistryState::Succeeded;
                descriptor_->registry_state_ready = true;
                descriptor_->registry_state_cv.notify_all();
            });

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
                        info("Loaded FMOD bank: {}", entry.path().string());
                    }
                }
            }
        }

        SerializationManager::load_from_file(d->project, cxproj, SerializationManager::Format::Json);
        // SerializationManager::load_from_file(*d->editor_scene, cxproj, SerializationManager::Format::Json);

        if (!d->project.last_open_scene.empty()) {
            {
                std::unique_lock guard{ d->registry_state_mutex };
                d->registry_state_cv.wait(guard, [&d] { return d->registry_state_ready; });
            }

            Asset<Scene> scene = AssetManager::load<Scene>(d->project.last_open_scene);
            if (scene) {
                log(Info, "Loaded scene {}", scene.path());
                d->editor_scene = scene.as_shared();
                d->active_scene = d->editor_scene;
            } else {
                log(Error, "Failed to load last open scene {}", d->project.last_open_scene);
            }
        }

        // Kick off async compilation; NBMan will be loaded on the main thread
        // once the build succeeds (via pendingNBLoad flag checked in on_update).
        compile_project();
    }

    void SceneEditorView::unload_project()
    {
        AssetManager::dispose();
        ax::AudioManager::unload_all();
        EditorApplication::vfs().unmount(std::string{ project_assets_vfs_root });

        if (NBMan::instance_loaded())
            NBMan::unload(!is_shutting_down_);

        descriptor_->reset();
    }

    void SceneEditorView::render_grid(gfx::DebugDraw& renderer, const scene::EditorCamera& camera,
                                      const GridRendererComponent& c) noexcept
    {
        CX_DEBUG_PROFILE_SCOPE("SceneEditorView::render_grid")

        const auto camera_dim = ivec2{ camera.width() * camera.pan(), camera.height() * camera.pan() };
        const auto camera_pos = camera.pos() - vec3{ camera_dim / 2, 0.0f };
        const auto start_pos  = glm::ceil(camera_pos / vec3{ c.cell_size, 1.0f }) * vec3{ c.cell_size, 0.0f };
        const auto count      = camera_dim / ivec2{ c.cell_size } + 1;

        for (auto i = 0; i < count.x; ++i) {
            renderer.draw_line_2d({ start_pos.x + i * c.cell_size.x, camera_pos.y },
                                  { start_pos.x + i * c.cell_size.x, camera_pos.y + camera_dim.y }, c.colour);
        }

        for (auto i = 0; i < count.y; ++i) {
            renderer.draw_line_2d({ camera_pos.x, start_pos.y + i * c.cell_size.y },
                                  { camera_pos.x + camera_dim.x, start_pos.y + i * c.cell_size.y }, c.colour);
        }
    }

    void SceneEditorView::load_outline_shader()
    {
        std::string shader_src;
        if (auto fh = EditorApplication::vfs().open("/edit/share/gl_shaders/batch_renderer2d_quad_outline.glsl",
                                                    { fs::FileMode::Read });
            fh) {
            shader_src.resize(fh->size());
            fh->read(shader_src.data(), fh->size());
            descriptor_->outline_shader = Box<gfx::Shader>::make(std::move(shader_src));
            if (descriptor_->outline_shader->compile_shader()) {
                descriptor_->outline_shader->bind();
                descriptor_->outline_shader->set_uniform_4f("u_outline_colour", descriptor_->select_colour.x,
                                                            descriptor_->select_colour.y, descriptor_->select_colour.z,
                                                            descriptor_->select_colour.w);
                descriptor_->outline_shader->unbind();
            } else {
                log(Error, "Failed to compile outline shader");
                descriptor_->outline_shader.reset();
            }
        } else
            log(Error, "Failed to load source for outline shader");
    }

    void SceneEditorView::viewport_resize()

    {
        auto& d = descriptor_;

        CX_DEBUG_PROFILE_SCOPE("on_update::viewport_resize")

        auto scene = d->active_scene.lock();
        cxensure(scene, "active_scene should not be null while viewport is resizing");

        scene::Camera* camera = nullptr;
        if (scene->state() == Scene::State::Play) {
            Entity entity = scene->primary_camera_entity();
            camera        = &entity.get_component<CameraComponent>().camera;
        } else
            camera = &Editor::viewport_camera();

        static auto prev_viewport = viewport_size_;
        if (viewport_size_ != prev_viewport || viewport_size_ != vec2{ camera->width(), camera->height() }) {
            camera->set_width(viewport_size_.x);
            camera->set_height(viewport_size_.y);
            prev_viewport = viewport_size_;
            framebuffer_->resize((u32)viewport_size_.x, (u32)viewport_size_.y);
            log(Info, "Primary framebuffer resize: {}x{}", framebuffer_->properties().width,
                framebuffer_->properties().height);
        }
    }

    void SceneEditorView::cook_and_export_project()
    {
        constexpr std::string_view output_root = "/edit/project/assets/package";
        constexpr std::string_view cook_root   = "/edit/project/assets/package/.cook";

        auto& d   = descriptor_;
        auto& vfs = EditorApplication::vfs();

        const auto      output_host_path = d->current_project_path / "assets/package";
        std::error_code ec;
        stdfs::remove_all(output_host_path, ec);
        if (ec) {
            log(Error, "Failed to clean export directory '{}': {}", output_host_path.generic_string(), ec.message());
            return;
        }

        const auto require_directory = [&](const std::string& path)
        {
            if (!vfs.mkdir(path, true)) {
                log(Error, "Failed to create export directory: {}", path);
                return false;
            }
            return true;
        };
        const auto require_copy = [&](const std::string& source, const std::string& destination, bool recursive = false)
        {
            if (!vfs.cp(source, destination, recursive)) {
                log(Error, "Failed to copy shipped file: {} -> {}", source, destination);
                return false;
            }
            return true;
        };

        if (!require_directory(std::string{ output_root }) || !require_directory(std::string{ output_root } + "/bin") ||
            !require_directory(std::string{ output_root } + "/lib") ||
            !require_directory(std::string{ output_root } + "/data") ||
            !require_directory(std::string{ output_root } + "/config") ||
            !require_directory(std::string{ cook_root } + "/assets/data"))
            return;

#ifdef CX_PLATFORM_WINDOWS
        constexpr std::string_view launcher_name = "CodexLauncher.exe";
        constexpr std::string_view runtime_name  = "ShippedRuntime.exe";
#else
        constexpr std::string_view launcher_name = "CodexLauncher";
        constexpr std::string_view runtime_name  = "ShippedRuntime";
#endif

        if (!require_copy("/edit/install/bin/" + std::string{ launcher_name },
                          std::string{ output_root } + "/bin/" + std::string{ launcher_name }) ||
            !require_copy("/edit/install/bin/" + std::string{ runtime_name },
                          std::string{ output_root } + "/bin/" + std::string{ runtime_name }))
            return;

#ifdef CX_PLATFORM_WINDOWS
        for (const auto& entry : vfs.list("/edit/install/data/shipped/bin", fs::ListOptions::FilesOnly)) {
            if (!require_copy("/edit/install/data/shipped/bin/" + entry, std::string{ output_root } + "/bin/" + entry))
                return;
        }
#else
        constexpr auto executable_permissions =
            stdfs::perms::owner_exec | stdfs::perms::group_exec | stdfs::perms::others_exec;
        for (const std::string_view binary : { launcher_name, runtime_name }) {
            const auto binary_path = output_host_path / "bin" / binary;
            stdfs::permissions(binary_path, executable_permissions, stdfs::perm_options::add, ec);
            if (ec) {
                log(Error, "Failed to make shipped binary executable '{}': {}", binary_path.generic_string(),
                    ec.message());
                return;
            }
        }

        const auto installed_lib_path   = stdfs::path{ CE_INSTALL_DIR } / "data/shipped/lib";
        const auto exported_lib_path    = output_host_path / "lib";
        const auto library_copy_options = stdfs::copy_options::recursive | stdfs::copy_options::overwrite_existing |
                                          stdfs::copy_options::copy_symlinks;
        for (const auto& entry : stdfs::directory_iterator{ installed_lib_path, ec }) {
            if (ec) {
                log(Error, "Failed to read installed library directory '{}': {}", installed_lib_path.generic_string(),
                    ec.message());
                return;
            }

            stdfs::copy(entry.path(), exported_lib_path / entry.path().filename(), library_copy_options, ec);
            if (ec) {
                log(Error, "Failed to copy shipped library '{}': {}", entry.path().generic_string(), ec.message());
                return;
            }
        }
        if (ec) {
            log(Error, "Failed to read installed library directory '{}': {}", installed_lib_path.generic_string(),
                ec.message());
            return;
        }
#endif

        for (const std::string resource : { "gl_shaders", "fonts", "images" }) {
            const std::string source = "/edit/install/data/" + resource;
            if (vfs.exists(source) && !require_copy(source, std::string{ output_root } + "/data", true))
                return;
        }

        EngineProject project    = Engine::project();
        auto*         boot_scene = AssetManager::registry().asset_metadata(d->project.last_open_scene);
        if (!boot_scene) {
            log(Error, "Cannot cook project: boot scene '{}' is not registered", d->project.last_open_scene);
            return;
        }

        project.engine_properties.video_properties.window_flags =
            WindowFlags::Visible | WindowFlags::Resizable | WindowFlags::PositionCentre;
        project.engine_properties.video_properties.window_title = project.name;
        project.engine_properties.cwd                           = "./";
        project.boot_scene                                      = boot_scene->path;
        project.assets_root                                     = "/run/project/assets";
        project.config_root                                     = "/run/config";

        const std::string module_filename = d->script_module_path.filename().generic_string();
        project.native_modules.clear();
        project.native_modules.push_back(module_filename);
        stdfs::copy_file(d->script_module_path, output_host_path / "lib" / module_filename,
                         stdfs::copy_options::overwrite_existing, ec);
        if (ec) {
            log(Error, "Failed to copy project native module '{}': {}", d->script_module_path.generic_string(),
                ec.message());
            return;
        }

        try {
            project.save_to_vfs(vfs, std::string{ output_root } + "/data/project.cxds");
        }
        catch (const std::exception& ex) {
            log(Error, "Failed to write project.cxds: {}", ex.what());
            return;
        }

        std::string settings_error;
        if (!shipped::save_video_settings(output_host_path / "config/video.json",
                                          shipped::VideoSettings::from(project.engine_properties.video_properties),
                                          &settings_error)) {
            log(Error, "Failed to write video settings: {}", settings_error);
            return;
        }
        if (!shipped::save_launcher_settings(output_host_path / "config/launcher.json", {}, &settings_error)) {
            log(Error, "Failed to write launcher settings: {}", settings_error);
            return;
        }

        AssetManager::registry()
            .write_manifest_async(std::string{ cook_root } + "/assets/__registry.manifest.bin")
            .await_sync();
        AssetManager::registry().export_assets_async(std::string{ cook_root } + "/assets/data").await_sync();

        if (vfs.exists("/edit/project/assets/audio") &&
            !require_copy("/edit/project/assets/audio", std::string{ cook_root } + "/assets/audio", true))
            return;

        constexpr std::string_view temporary_pak = "/edit/tmp/registry.cxpk";
        auto                       pak = vfs.open(std::string{ temporary_pak },
                                                  { fs::FileMode::Create | fs::FileMode::Trunc | fs::FileMode::Write });
        if (!pak) {
            log(Error, "Failed to create temporary registry.cxpk");
            return;
        }
        if (!vfs.export_to_pak(pak, {}, std::string{ cook_root })) {
            log(Error, "Failed to package cooked assets");
            return;
        }
        pak.reset();

        if (!vfs.mv(std::string{ temporary_pak }, std::string{ output_root } + "/data/registry.cxpk")) {
            log(Error, "Failed to move registry.cxpk into the export");
            return;
        }

        stdfs::remove_all(output_host_path / ".cook", ec);
        if (ec)
            log(Warn, "Export succeeded, but temporary cook data could not be removed: {}", ec.message());
        else
            log(Info, "Project exported successfully to '{}'", output_host_path.generic_string());
    }

    void SceneEditorView::draw_vec3_control(const char* label, vec3& values, const f32 column_wdith, const f32 speed,
                                            const f32 reset_value)
    {
        ImGuiIO& io        = ImGui::GetIO();
        auto     bold_font = io.Fonts->Fonts[0];

        ImGui::PushID(label);

        if (label[0] != '#' && label[1] != '#') {
            ImGui::Columns(2);
            ImGui::SetColumnWidth(0, column_wdith);
            ImGui::Text("%s", label);
            ImGui::NextColumn();
        }

        ImGui::PushMultiItemsWidths(3, ImGui::CalcItemWidth());
        ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2{ 0, 0 });

        float  lineHeight = ImGui::GetFrameHeight();
        ImVec2 buttonSize = { lineHeight + 3.0f, lineHeight };

        ImGui::PushStyleColor(ImGuiCol_Button, ImVec4{ 0.8f, 0.1f, 0.15f, 1.0f });
        ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4{ 0.9f, 0.2f, 0.2f, 1.0f });
        ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4{ 0.8f, 0.1f, 0.15f, 1.0f });
        ImGui::PushFont(bold_font);
        if (ImGui::Button("X", buttonSize))
            values.x = reset_value;
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
            values.y = reset_value;
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
            values.z = reset_value;
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

    void SceneEditorView::draw_vec2_control(const char* label, vec2& values, const f32 column_width, const f32 speed,
                                            const f32 reset_value)
    {
        ImGuiIO& io        = ImGui::GetIO();
        auto     bold_font = io.Fonts->Fonts[0];

        ImGui::PushID(label);

        if (label[0] != '#' && label[1] != '#') {
            ImGui::Columns(2);
            ImGui::SetColumnWidth(0, column_width);
            ImGui::Text("%s", label);
            ImGui::NextColumn();
        }

        ImGui::PushMultiItemsWidths(3, ImGui::CalcItemWidth());
        ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2{ 0, 0 });

        f32    lineHeight = ImGui::GetFrameHeight();
        ImVec2 buttonSize = { lineHeight + 3.0f, lineHeight };

        ImGui::PushStyleColor(ImGuiCol_Button, ImVec4{ 0.8f, 0.1f, 0.15f, 1.0f });
        ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4{ 0.9f, 0.2f, 0.2f, 1.0f });
        ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4{ 0.8f, 0.1f, 0.15f, 1.0f });
        ImGui::PushFont(bold_font);
        if (ImGui::Button("X", buttonSize))
            values.x = reset_value;
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
            values.y = reset_value;
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

    void SceneEditorView::draw_asset_path([[maybe_unused]] const AssetPath& path,
                                          [[maybe_unused]] const f32        column_width)
    {
    }
} // namespace codex::editor
