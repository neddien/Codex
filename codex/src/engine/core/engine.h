#pragma once

#include <engine/concurrency/public/cooperative_executor.h>
#include <engine/concurrency/public/threaded_executor.h>
#include <engine/core/layer_stack.h>
#include <engine/core/public/exception.h>
#include <engine/core/public/log.h>
#include <engine/core/window.h>
#include <engine/events/application_event.h>
#include <engine/events/key_event.h>
#include <engine/events/mouse_event.h>
#include <engine/imgui/im_gui_layer.h>
#include <engine/memory/public/memory.h>
#include <engine/system/utils.h>

int main(int argc, char** argv);

namespace codex {
    // Forward declarations.
    class Input;
    namespace events {
        class WindowResizeEvent;
    } // namespace events
    namespace imgui {
        class ImGuiLayer;
    } // namespace imgui

    CX_CUSTOM_EXCEPTION(InvalidPathException, "The path supplied is invalid.");

    [[nodiscard]] constexpr auto get_dedicated_thread_count() noexcept
    {
        // Fixed and dedicated threads are as follow:
        // Update Thread or Main Thread
        // Audio Thread
        // Physics Thread
        // Available Threads: std::thread::hardware_concurrency() - get_dedicated_thread_count()
        return 3;
    }

    struct EngineArgs
    {
    public:
        i32    count = 0;
        char** args  = nullptr;

    public:
        auto operator[](const usize index) const noexcept -> const char*
        {
            CX_ASSERT(index > (usize)count, "Index out of bounds.");
            return args[index];
        }
    };

    enum class EngineFlags
    {
        Video   = bit(0),
        Audio   = bit(1),
        Input   = bit(2),
        Logger  = bit(3),
        InitAll = 0xFF,
    };
    CX_ENABLE_BITWISE_ENUM(EngineFlags);

    struct EngineProperties
    {
        std::string           name = "Codex Application";
        std::filesystem::path cwd;
        EngineArgs            args;
        EngineFlags           flags = EngineFlags::InitAll;
        WindowProperties      window_properties;
    };

    class CODEX_API Engine : public Loggable<"Engine">
    {
        friend int ::main(int argc, char** argv);
        friend Engine* create_engine(EngineArgs args);

    public:
        explicit Engine(EngineProperties props);
        Engine(const Engine&)            = delete;
        Engine& operator=(const Engine&) = delete;
        Engine(Engine&&)                 = delete;
        Engine& operator=(Engine&&)      = delete;
        virtual ~Engine();

    public:
        // FIXME: Throw a NullReferenceException when an Engine instance hasn't yet been created.
        [[nodiscard]] static inline auto window() noexcept -> Window& { return *s_instance_->window_; }
        [[nodiscard]] static inline auto get() noexcept -> Engine& { return *s_instance_; }
        [[nodiscard]] static inline auto fps() noexcept -> u32
        {
            return static_cast<u32>(1.0f / s_instance_->delta_time_);
        }
        [[nodiscard]] static inline auto frame_cap() noexcept -> u32
        {
            return s_instance_->properties_.window_properties.frame_cap;
        }
        [[nodiscard]] static inline auto delta() noexcept -> f32 { return s_instance_->delta_time_; }
        [[nodiscard]] static inline auto mgui_layer() noexcept -> imgui::ImGuiLayer*
        {
            return s_instance_->imgui_layer_;
        }
        [[nodiscard]] static inline auto get_worker_pool() noexcept -> cc::ThreadedExecutor&
        {
            return *s_instance_->worker_thread_executor_;
        }
        [[nodiscard]] static inline auto get_cooperative_pool() noexcept -> cc::CooperativeExecutor&
        {
            return s_instance_->main_thread_executor_;
        }
        [[nodiscard]] static inline auto get_current_thread_id() noexcept -> u32
        {
            return sys::get_current_thread_id();
        }
        [[nodiscard]] static inline auto cwd() noexcept -> std::filesystem::path
        {
            return std::filesystem::current_path();
        }
        [[nodiscard]] static inline auto engine_concurrency() noexcept { return sys::get_engine_thread_count(); }
        static inline void               set_cwd(const std::filesystem::path& new_cwd)
        {
            if (std::filesystem::exists(new_cwd) && std::filesystem::is_directory(new_cwd)) {
                std::filesystem::current_path(new_cwd);
                s_instance_->properties_.cwd = new_cwd;
            } else {
                throw InvalidPathException("The path supplied '{}' as the current working directory is invalid.",
                                           new_cwd.string());
            }
        }

    public:
        auto         on_window_resize_event(const events::WindowResizeEvent& event) -> bool;
        virtual auto on_init() -> void {};
        auto         run() -> void;
        auto         stop() -> void;
        auto         on_event(events::Event& e) -> void;
        auto         push_layer(Layer* layer) -> void;
        auto         push_overlay(Layer* overlay) -> void;

    protected:
        EngineProperties          properties_;
        Box<Window>               window_    = nullptr;
        bool                      running_   = true;
        bool                      minimized_ = false;
        LayerStack                layer_stack_;
        f32                       delta_time_  = 0.0f;
        imgui::ImGuiLayer*        imgui_layer_ = nullptr;
        Input*                    input_       = nullptr;
        Box<cc::ThreadPool>       thread_pool_;
        cc::CooperativeExecutor   main_thread_executor_;
        Box<cc::ThreadedExecutor> worker_thread_executor_;

    private:
        void internal_init();

    private:
        static Engine* s_instance_;
    };

    [[nodiscard]] auto create_engine(const EngineArgs args) -> Engine*;
} // namespace codex
