#ifndef CODEX_CORE_WINDOW_H
#define CODEX_CORE_WINDOW_H

#include <sdafx.h>

#include "Public/CommonDef.h"
#include "Public/Exception.h"
#include "Public/Geomtryd.h"

#include <Engine/Memory/Public/Memory.h>
#include <Engine/Events/Event.h>
#include <Engine/Graphics/Renderer.h>

#include <SDL.h>
#include <SDL_opengl.h>
#include <SDL_syswm.h>

// SDL imports X11 headers on Linux and X11 headers define this absurdly vague and common identifier as a macro.
#undef None

namespace codex {
    // Forward declerations
    class Application;
    class Scene;

    CX_CUSTOM_EXCEPTION(SDLException, "SDL failed to initialize.")
    CX_CUSTOM_EXCEPTION(GLADException, "GLAD failed to initialize.")

    enum class WindowFlags : u32
    {
        Visible        = BitFlag<u32>(0),
        Hidden         = BitFlag<u32>(1),
        Resizable      = BitFlag<u32>(2),
        FullScreen     = BitFlag<u32>(3),
        Borderless     = BitFlag<u32>(4),
        Minimized      = BitFlag<u32>(5),
        Maximized      = BitFlag<u32>(6),
        SkipTaskbar    = BitFlag<u32>(7),
        OpenGLContext  = BitFlag<u32>(8),
        PositionCentre = BitFlag<u32>(9)
    };

    CODEX_API WindowFlags operator|(const WindowFlags& lhv, const WindowFlags& rhv) noexcept;
    CODEX_API u32         operator&(const WindowFlags& lhv, const WindowFlags& rhv) noexcept;

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
        i32         posX       = 0;
        i32         posY       = 0;
        u32         frameCap   = 300;
        WindowFlags flags      = WindowFlags::Visible | WindowFlags::Resizable;
        bool        vsync      = true;
        bool        borderless = false;
    };

    class CODEX_API Window
    {
        using Box                   = std::unique_ptr<Window, std::function<void(Window*)>>;
        using EventCallbackDelegate = std::function<void(events::Event&)>;

        friend class Application;
        friend class mem::Box<Window>;

    private:
        std::string                                        m_Title;
        i32                                                m_Width, m_Height;
        i32                                                m_PosX, m_PosY;
        u32                                                m_Flags;
        u32                                                m_Fps, m_FrameCount, m_FrameCap;
        std::chrono::system_clock::time_point              m_Tp1, m_Tp2;
        const void*                                        m_NativeWindow;
        std::unique_ptr<gfx::Renderer>                     m_Renderer;
        SDL_Window*                                        m_SdlWindow;
        SDL_GLContext                                      m_GlContext;
        SDL_Event                                          m_SdlEvent;
        EventCallbackDelegate                              m_EventCallback;
        std::array<SDL_Cursor*, (usize)SystemCursor::Null> m_SdlCursors;

    private:
        Window();
        Window(const Window& other)                = delete;
        Window& operator=(const Window& other)     = delete;
        Window(Window&& other) noexcept            = delete;
        Window& operator=(Window&& other) noexcept = delete;
        ~Window();

    public:
        inline i32            GetWidth() const noexcept { return m_Width; }
        inline i32            GetHeight() const noexcept { return m_Height; }
        inline void           SetTitle(const char* newTitle) noexcept { SDL_SetWindowTitle(m_SdlWindow, newTitle); }
        inline SDL_Window*    GetNativeWindow() noexcept { return m_SdlWindow; }
        inline SDL_GLContext* GetGlContext() noexcept { return &m_GlContext; }
        inline void SetEventCallback(const EventCallbackDelegate& callback) noexcept { m_EventCallback = callback; }
        inline u32  GetFrameCount() const noexcept { return m_FrameCount; }
        inline void SetCursor(const SystemCursor cursor) noexcept
        {
            auto cursor_ptr = GetSDLCursor(cursor);
            SDL_SetCursor(cursor_ptr);
        }
        inline Vector2 GetPosition() const noexcept
        {
            Vector2 vec;
            SDL_GetWindowPosition(m_SdlWindow, &vec.x, &vec.y);
            return vec;
        }
        inline Vector2 GetSize() const noexcept
        {
            Vector2 vec;
            SDL_GetWindowSize(m_SdlWindow, &vec.x, &vec.y);
            return vec;
        }
        inline void SetPosition(const Vector2& newPos) noexcept
        {
            SDL_SetWindowPosition(m_SdlWindow, newPos.x, newPos.y);
        }
        inline void SetSize(const Vector2& newSize) noexcept { SDL_SetWindowSize(m_SdlWindow, newSize.x, newSize.y); }
        inline void Minimize() const noexcept { SDL_MinimizeWindow(m_SdlWindow); }
        inline void Maximize() const noexcept { SDL_MaximizeWindow(m_SdlWindow); }

    public:
        void        Init(const WindowProperties& windowInfo = WindowProperties(), const void* nativeWindow = nullptr);
        void        OnUpdate(const f32 deltaTime);
        void        SwapBuffers();
        void        ProcessEvents();
        void        SDLCheckError(const i32 line = -1);
        void        SDLThrowError(const i32 line, const std::string_view errorMessage);
        void        OnWindowResize_Event(const i32 newWidth, const i32 newHeight);
        SDL_Cursor* GetSDLCursor(const SystemCursor cursor) noexcept;
    };
} // namespace codex

#endif // CODEX_CORE_WINDOW_H
