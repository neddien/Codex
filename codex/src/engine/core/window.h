#pragma once

#include <SDL.h>
#include <SDL_syswm.h>

#include <engine/core/public/log.h>
#include <engine/events/event.h>
#include <engine/graphics/renderer.h>
#include <engine/memory/public/memory.h>

#include <SDL_opengl.h>

#include "public/common_def.h"
#include "public/exception.h"
#include "public/geometry.h"

// SDL imports X11 headers on Linux and X11 headers define this absurdly vague and common identifier as a macro.
#undef None

namespace codex {
    // Forward declerations
    class Engine;
    class Scene;

    CX_CUSTOM_EXCEPTION(SDLException, "SDL failed to initialize.")
    CX_CUSTOM_EXCEPTION(GLADException, "GLAD failed to initialize.")

    enum class WindowFlags : u32
    {
        Visible        = bit(0),
        Hidden         = bit(1),
        Resizable      = bit(2),
        FullScreen     = bit(3),
        Borderless     = bit(4),
        Minimized      = bit(5),
        Maximized      = bit(6),
        SkipTaskbar    = bit(7),
        OpenGLContext  = bit(8),
        PositionCentre = bit(9)
    };
    CX_ENABLE_BITWISE_ENUM(WindowFlags);

    enum class SystemCursor
    {
        Arrow               = SDL_SYSTEM_CURSOR_ARROW,
        IBeam               = SDL_SYSTEM_CURSOR_IBEAM,
        Wait                = SDL_SYSTEM_CURSOR_WAIT,
        Crosshair           = SDL_SYSTEM_CURSOR_CROSSHAIR,
        Resize              = SDL_SYSTEM_CURSOR_SIZEALL,
        WaitArrow           = SDL_SYSTEM_CURSOR_WAITARROW,
        DiagonalLeftResize  = SDL_SYSTEM_CURSOR_SIZENWSE,
        DiagonalRightResize = SDL_SYSTEM_CURSOR_SIZENESW,
        VerticalResize      = SDL_SYSTEM_CURSOR_SIZENS,
        HorizontalResize    = SDL_SYSTEM_CURSOR_SIZEWE,
        No                  = SDL_SYSTEM_CURSOR_NO,
        Hand                = SDL_SYSTEM_CURSOR_HAND,

        Null
    };

    struct WindowProperties
    {
        const char* title      = "Codex - Window";
        i32         width      = 1280;
        i32         height     = 720;
        i32         pos_x      = 0;
        i32         pos_y      = 0;
        u32         frame_cap  = 300;
        WindowFlags flags      = WindowFlags::Visible | WindowFlags::Resizable;
        bool        vsync      = true;
        bool        borderless = false;
    };

    class CODEX_API Window : public Loggable<"Window">
    {
        friend class Engine;
        friend class Box<Window>;
        friend struct std::default_delete<Window>;

    private:
        using Box                   = std::unique_ptr<Window, std::function<void(Window*)>>;
        using EventCallbackDelegate = std::function<void(events::Event&)>;

    private:
        Window();
        Window(const Window& other)                = delete;
        Window& operator=(const Window& other)     = delete;
        Window(Window&& other) noexcept            = delete;
        Window& operator=(Window&& other) noexcept = delete;
        ~Window();

    public:
        [[nodiscard]] inline i32            width() const noexcept { return width_; }
        [[nodiscard]] inline i32            height() const noexcept { return height_; }
        [[nodiscard]] inline SDL_Window*    native_window() noexcept { return sdl_window_; }
        [[nodiscard]] inline SDL_GLContext* gl_context() noexcept { return &gl_context_; }
        [[nodiscard]] inline u32            frame_count() const noexcept { return frame_count_; }
        [[nodiscard]] inline Vector2        position() const noexcept
        {
            Vector2 vec;
            SDL_GetWindowPosition(sdl_window_, &vec.x, &vec.y);
            return vec;
        }
        [[nodiscard]] inline Vector2 size() const noexcept
        {
            Vector2 vec;
            SDL_GetWindowSize(sdl_window_, &vec.x, &vec.y);
            return vec;
        }
        inline void set_title(const char* new_title) noexcept { SDL_SetWindowTitle(sdl_window_, new_title); }
        inline void set_event_callback(const EventCallbackDelegate& callback) noexcept { event_callback_ = callback; }
        inline void set_cursor(const SystemCursor cursor) noexcept
        {
            auto cursor_ptr = sdl_cursor(cursor);
            SDL_SetCursor(cursor_ptr);
        }
        inline void set_position(const Vector2& new_pos) noexcept
        {
            SDL_SetWindowPosition(sdl_window_, new_pos.x, new_pos.y);
        }
        inline void set_size(const Vector2& new_size) noexcept
        {
            SDL_SetWindowSize(sdl_window_, new_size.x, new_size.y);
        }
        inline void minimize() const noexcept { SDL_MinimizeWindow(sdl_window_); }
        inline void maximize() const noexcept { SDL_MaximizeWindow(sdl_window_); }

    public:
        void        init(const WindowProperties& windowInfo = WindowProperties{}, const void* nativeWindow = nullptr);
        void        on_update(const f32 deltaTime);
        void        swap_buffers();
        void        process_events();
        void        sdl_check_error(const i32 line = -1);
        void        sdl_throw_error(const i32 line, const std::string_view errorMessage);
        void        on_window_resize_event(const i32 new_width, const i32 new_height);
        SDL_Cursor* sdl_cursor(const SystemCursor cursor) noexcept;

    private:
        std::string                                        title_;
        i32                                                width_, height_;
        i32                                                pos_x_, pos_y_;
        u32                                                flags_;
        u32                                                fps_, frame_count_, frame_cap_;
        std::chrono::system_clock::time_point              tp1_, tp2_;
        const void*                                        native_window_;
        std::unique_ptr<gfx::Renderer>                     renderer_;
        SDL_Window*                                        sdl_window_;
        SDL_GLContext                                      gl_context_;
        SDL_Event                                          sdl_event_;
        EventCallbackDelegate                              event_callback_;
        std::array<SDL_Cursor*, (usize)SystemCursor::Null> sdl_cursors_;
    };
} // namespace codex
