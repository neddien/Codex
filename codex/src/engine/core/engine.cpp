#include "engine.h"

#include <engine/asset_manager/public/asset_manager.h>
#include <engine/audio/audio_system.h>
#include <engine/debug/public/profiler.h>
#include <engine/debug/public/time_scope.h>
#include <engine/scene/component_factory.h>
#include <engine/scene/public/components.inl>
#include <engine/scene/public/scene.h>

#include "public/exception.h"
#include "public/input.h"
#include <engine/core/public/version.h>

namespace codex {
    namespace stdfs = std::filesystem;
    using namespace codex::events;
    using namespace codex::imgui;
    using namespace codex::gfx;
    using namespace codex::ax;

    Engine* Engine::s_instance_ = nullptr;

    Engine::Engine(EngineProperties args)
        : properties_{ std::move(args) }
    {
        internal_init();
    }

    Engine::~Engine()
    {
        layer_stack_.clear();

        if (properties_.flags & EngineFlags::Input)
            Input::dispose();

        if (properties_.flags & EngineFlags::Audio)
            AudioSystem::dispose();

        // Always dispose the logger at the very end.
        if (properties_.flags & EngineFlags::Logger)
            codex::detail::dispose_logger();

        s_instance_ = nullptr;
    }

    void Engine::internal_init()
    {
        if (properties_.flags & EngineFlags::Logger)
            codex::detail::init_logger();

        log(Info, "Codex Engine v{} (build {} {})", CX_VERSION_STRING, CX_BUILD_COUNT, CX_COMPILER_BIN);

        thread_pool_ = Box<cc::ThreadPool>::make(std::thread::hardware_concurrency() - sys::get_engine_thread_count());
        worker_thread_executor_ = Box<cc::ThreadedExecutor>::make(*thread_pool_);

        try {
            if (stdfs::exists(properties_.cwd) && stdfs::is_directory(properties_.cwd)) {
                stdfs::current_path(properties_.cwd);
            } else {
                throw InvalidPathException("The path supplied '{}' as the current working directory is invalid.",
                                           properties_.cwd.string());
            }

            s_instance_ = this;

            if (properties_.flags & EngineFlags::Video) {
                window_ = Box<Window>::make();
                window_->init(properties_.window_properties);
                window_->set_event_callback(bind_event_delegate(this, &Engine::on_event));
            }

            if (properties_.flags & EngineFlags::Input) {
                input_ = Input::get();
            }

            register_all_components();

            if (properties_.flags & EngineFlags::Audio) {
                AudioSystem::init();
            }

            imgui_layer_ = new ImGuiLayer();
            push_overlay(imgui_layer_);
        }
        catch (const CodexException& ex) {
            fatal(ex.to_string());
            std::exit(EXIT_FAILURE);
        }
    }

    auto Engine::on_window_resize_event(const WindowResizeEvent& event) -> bool
    {
        const auto x = event.width();
        const auto y = event.height();
        glViewport(0, 0, x, y);
        if (imgui_layer_) {
            auto& io         = ImGui::GetIO();
            io.DisplaySize.x = (f32)x;
            io.DisplaySize.y = (f32)y;
        }
        return false;
    }

    void Engine::run()
    {
        using clock = std::chrono::high_resolution_clock;

        while (running_) {
            const auto frame_start = clock::now();

            try {
                // Tick the cooperative scheduler
                main_thread_executor_.tick();

                if (window_) {
                    window_->process_events();

                    if (!minimized_) {
                        for (Layer* layer : layer_stack_) {
                            CX_DEBUG_PROFILE_SCOPE("layer_update")
                            layer->on_update(delta_time_);
                        }

                        if (imgui_layer_) {
                            CX_DEBUG_PROFILE_SCOPE("im_gui_layer_update")

                            imgui_layer_->begin();
                            for (Layer* layer : layer_stack_)
                                layer->on_imgui_render();
                            imgui_layer_->end();
                        }
                    }

                    window_->swap_buffers();

                    // FIXME: Fix the mouse dragging thing for now...
                    // What?????
                    if (properties_.flags & EngineFlags::Input)
                        Input::end_frame();
                }

                if (ax::AudioSystem::is_valid())
                    ax::AudioSystem::update();
            }
            catch (const CodexException& ex) {
                fatal(ex.to_string());
                std::exit(EXIT_FAILURE);
            }

            const auto frame_time = std::chrono::duration_cast<std::chrono::milliseconds>(clock::now() - frame_start);

            // Cap the framerate if needed.
            if (properties_.window_properties.frame_cap > 0) {
                static auto desired_frame_time =
                    std::chrono::milliseconds(1000 / properties_.window_properties.frame_cap);

                if (frame_time < desired_frame_time)
                    std::this_thread::sleep_for(desired_frame_time - frame_time);
            }

            delta_time_ = std::chrono::duration<f32>(clock::now() - frame_start).count();
        }
    }

    void Engine::stop()
    {
        running_ = false;
    }

    void Engine::on_event(Event& e)
    {
        EventDispatcher d(e);

        if (properties_.flags & EngineFlags::Input) {
            d.dispatch<KeyDownEvent>(bind_event_delegate(input_, &Input::on_key_down_event));
            d.dispatch<KeyUpEvent>(bind_event_delegate(input_, &Input::on_key_up_event));
            d.dispatch<MouseDownEvent>(bind_event_delegate(input_, &Input::on_mouse_down_event));
            d.dispatch<MouseUpEvent>(bind_event_delegate(input_, &Input::on_mouse_up_event));
            d.dispatch<MouseMoveEvent>(bind_event_delegate(input_, &Input::on_mouse_move_event));
            d.dispatch<MouseScrollEvent>(bind_event_delegate(input_, &Input::on_mouse_scroll_event));
        }

        if (properties_.flags & EngineFlags::Video)
            d.dispatch<WindowResizeEvent>(bind_event_delegate(this, &Engine::on_window_resize_event));

        for (auto it = layer_stack_.rbegin(); it != layer_stack_.rend(); ++it) {
            if (e.handled)
                break;

            (*it)->on_event(e);
        }
    }

    void Engine::push_layer(Layer* layer)
    {
        layer_stack_.push_layer(layer);
        layer->on_attach();
    }

    void Engine::push_overlay(Layer* overlay)
    {
        layer_stack_.push_overlay(overlay);
        overlay->on_attach();
    }
} // namespace codex
