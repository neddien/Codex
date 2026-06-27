#include "window.h"

#include "engine.h"

#include <engine/events/application_event.h>
#include <engine/events/key_event.h>
#include <engine/events/mouse_event.h>
#include <engine/graphics/debug_draw.h>

#include <imgui.h>
#include <imgui_impl_sdl2.h>

namespace codex {
    using namespace codex::events;
    using namespace codex::imgui;
    using namespace codex::gfx;

    [[nodiscard]] inline Key FromSDLKeycode(const SDL_Keycode code) noexcept
    {
        switch (code) {
            case SDLK_UNKNOWN: return Key::Unknown;

            case SDLK_RETURN: return Key::Return;
            case SDLK_CAPSLOCK: return Key::CapsLock;
            case SDLK_ESCAPE: return Key::Escape;
            case SDLK_BACKSPACE: return Key::Backspace;
            case SDLK_TAB: return Key::Tab;
            case SDLK_SPACE: return Key::Space;
            case SDLK_EXCLAIM: return Key::Exclaim;
            case SDLK_QUOTEDBL: return Key::DoubleQuote;
            case SDLK_HASH: return Key::Pound;
            case SDLK_PERCENT: return Key::Percent;
            case SDLK_DOLLAR: return Key::Dollar;
            case SDLK_AMPERSAND: return Key::Ampersand;
            case SDLK_QUOTE: return Key::SingleQuote;
            case SDLK_LEFTPAREN: return Key::LeftParen;
            case SDLK_RIGHTPAREN: return Key::RightParen;
            case SDLK_ASTERISK: return Key::Asterisk;
            case SDLK_PLUS: return Key::Plus;
            case SDLK_COMMA: return Key::Comma;
            case SDLK_MINUS: return Key::Minus;
            case SDLK_PERIOD: return Key::Period;
            case SDLK_SLASH: return Key::ForwardSlash;
            case SDLK_0: return Key::Num0;
            case SDLK_1: return Key::Num1;
            case SDLK_2: return Key::Num2;
            case SDLK_3: return Key::Num3;
            case SDLK_4: return Key::Num4;
            case SDLK_5: return Key::Num5;
            case SDLK_6: return Key::Num6;
            case SDLK_7: return Key::Num7;
            case SDLK_8: return Key::Num8;
            case SDLK_9: return Key::Num9;
            case SDLK_COLON: return Key::Colon;
            case SDLK_SEMICOLON: return Key::SemiColon;
            case SDLK_LESS: return Key::Less;
            case SDLK_EQUALS: return Key::Equals;
            case SDLK_GREATER: return Key::Greater;
            case SDLK_QUESTION: return Key::QuestionMark;
            case SDLK_AT: return Key::At;

            case SDLK_LEFTBRACKET: return Key::LeftBracket;
            case SDLK_BACKSLASH: return Key::BackSlash;
            case SDLK_RIGHTBRACKET: return Key::RightBracket;
            case SDLK_CARET: return Key::Caret;
            case SDLK_UNDERSCORE: return Key::Underscore;
            case SDLK_BACKQUOTE: return Key::BackQuote;

            case SDLK_a: return Key::A;
            case SDLK_b: return Key::B;
            case SDLK_c: return Key::C;
            case SDLK_d: return Key::D;
            case SDLK_e: return Key::E;
            case SDLK_f: return Key::F;
            case SDLK_g: return Key::G;
            case SDLK_h: return Key::H;
            case SDLK_i: return Key::I;
            case SDLK_j: return Key::J;
            case SDLK_k: return Key::K;
            case SDLK_l: return Key::L;
            case SDLK_m: return Key::M;
            case SDLK_n: return Key::N;
            case SDLK_o: return Key::O;
            case SDLK_p: return Key::P;
            case SDLK_q: return Key::Q;
            case SDLK_r: return Key::R;
            case SDLK_s: return Key::S;
            case SDLK_t: return Key::T;
            case SDLK_u: return Key::U;
            case SDLK_v: return Key::V;
            case SDLK_w: return Key::W;
            case SDLK_x: return Key::X;
            case SDLK_y: return Key::Y;
            case SDLK_z: return Key::Z;

            case SDLK_F1: return Key::F1;
            case SDLK_F2: return Key::F2;
            case SDLK_F3: return Key::F3;
            case SDLK_F4: return Key::F4;
            case SDLK_F5: return Key::F5;
            case SDLK_F6: return Key::F6;
            case SDLK_F7: return Key::F7;
            case SDLK_F8: return Key::F8;
            case SDLK_F9: return Key::F9;
            case SDLK_F10: return Key::F10;
            case SDLK_F11: return Key::F11;
            case SDLK_F12: return Key::F12;

            case SDLK_PRINTSCREEN: return Key::PrintScreen;
            case SDLK_SCROLLLOCK: return Key::ScrollLock;
            case SDLK_PAUSE: return Key::Pause;
            case SDLK_INSERT: return Key::Insert;
            case SDLK_HOME: return Key::Home;
            case SDLK_PAGEUP: return Key::PageUp;
            case SDLK_DELETE: return Key::Delete;
            case SDLK_END: return Key::End;
            case SDLK_PAGEDOWN: return Key::PageDown;
            case SDLK_RIGHT: return Key::Right;
            case SDLK_LEFT: return Key::Left;
            case SDLK_DOWN: return Key::Down;
            case SDLK_UP: return Key::Up;

            case SDLK_NUMLOCKCLEAR: return Key::NumLockClear;
            case SDLK_KP_DIVIDE: return Key::KeyPadDivide;
            case SDLK_KP_MULTIPLY: return Key::KeyPadMultiply;
            case SDLK_KP_MINUS: return Key::KeyPadMinus;
            case SDLK_KP_PLUS: return Key::KeyPadPlus;
            case SDLK_KP_ENTER: return Key::KeyPadEnter;
            case SDLK_KP_1: return Key::KeyPad1;
            case SDLK_KP_2: return Key::KeyPad2;
            case SDLK_KP_3: return Key::KeyPad3;
            case SDLK_KP_4: return Key::KeyPad4;
            case SDLK_KP_5: return Key::KeyPad5;
            case SDLK_KP_6: return Key::KeyPad6;
            case SDLK_KP_7: return Key::KeyPad7;
            case SDLK_KP_8: return Key::KeyPad8;
            case SDLK_KP_9: return Key::KeyPad9;
            case SDLK_KP_0: return Key::KeyPad0;
            case SDLK_KP_PERIOD: return Key::KeyPadPeriod;

            case SDLK_APPLICATION: return Key::Application;
            case SDLK_POWER: return Key::Power;
            case SDLK_KP_EQUALS: return Key::KeyPadEquals;
            case SDLK_F13: return Key::F13;
            case SDLK_F14: return Key::F14;
            case SDLK_F15: return Key::F15;
            case SDLK_F16: return Key::F16;
            case SDLK_F17: return Key::F17;
            case SDLK_F18: return Key::F18;
            case SDLK_F19: return Key::F19;
            case SDLK_F20: return Key::F20;
            case SDLK_F21: return Key::F21;
            case SDLK_F22: return Key::F22;
            case SDLK_F23: return Key::F23;
            case SDLK_F24: return Key::F24;
            case SDLK_EXECUTE: return Key::Execute;
            case SDLK_HELP: return Key::Help;
            case SDLK_MENU: return Key::Menu;
            case SDLK_SELECT: return Key::Select;
            case SDLK_STOP: return Key::Stop;
            case SDLK_AGAIN: return Key::Again;
            case SDLK_UNDO: return Key::Undo;
            case SDLK_CUT: return Key::Cut;
            case SDLK_COPY: return Key::Copy;
            case SDLK_PASTE: return Key::Paste;
            case SDLK_FIND: return Key::Find;
            case SDLK_MUTE: return Key::Mute;
            case SDLK_VOLUMEUP: return Key::VolumeUp;
            case SDLK_VOLUMEDOWN: return Key::VolumeDown;
            case SDLK_KP_COMMA: return Key::KeyPadComma;
            case SDLK_KP_EQUALSAS400: return Key::KeyPadEqualsAs400;

            case SDLK_ALTERASE: return Key::Alterase;
            case SDLK_SYSREQ: return Key::SysReq;
            case SDLK_CANCEL: return Key::Cancel;
            case SDLK_CLEAR: return Key::Clear;
            case SDLK_PRIOR: return Key::Prior;
            case SDLK_RETURN2: return Key::Return2;
            case SDLK_SEPARATOR: return Key::Separator;
            case SDLK_OUT: return Key::Out;
            case SDLK_OPER: return Key::Oper;
            case SDLK_CLEARAGAIN: return Key::ClearAgain;
            case SDLK_CRSEL: return Key::CrSel;
            case SDLK_EXSEL: return Key::ExSel;

            case SDLK_KP_00: return Key::KeyPad00;
            case SDLK_KP_000: return Key::KeyPad000;
            case SDLK_THOUSANDSSEPARATOR: return Key::ThousandsSeparator;
            case SDLK_DECIMALSEPARATOR: return Key::DecimalSeparator;
            case SDLK_CURRENCYUNIT: return Key::CurrencyUnit;
            case SDLK_CURRENCYSUBUNIT: return Key::CurrencySubUnit;
            case SDLK_KP_LEFTPAREN: return Key::KeyPadLeftParen;
            case SDLK_KP_RIGHTPAREN: return Key::KeyPadRightParen;
            case SDLK_KP_LEFTBRACE: return Key::KeyPadLeftBrace;
            case SDLK_KP_RIGHTBRACE: return Key::KeyPadRightBrace;
            case SDLK_KP_TAB: return Key::KeyPadTab;
            case SDLK_KP_BACKSPACE: return Key::KeyPadBackspace;
            case SDLK_KP_A: return Key::KeyPadA;
            case SDLK_KP_B: return Key::KeyPadB;
            case SDLK_KP_C: return Key::KeyPadC;
            case SDLK_KP_D: return Key::KeyPadD;
            case SDLK_KP_E: return Key::KeyPadE;
            case SDLK_KP_F: return Key::KeyPadF;
            case SDLK_KP_XOR: return Key::KeyPadXOR;
            case SDLK_KP_POWER: return Key::KeyPadPower;
            case SDLK_KP_PERCENT: return Key::KeyPadPercent;
            case SDLK_KP_LESS: return Key::KeyPadLess;
            case SDLK_KP_GREATER: return Key::KeyPadGreater;
            case SDLK_KP_AMPERSAND: return Key::KeyPadAmpersand;
            case SDLK_KP_DBLAMPERSAND: return Key::KeyPadDblAmpersand;
            case SDLK_KP_VERTICALBAR: return Key::KeyPadVerticalBar;
            case SDLK_KP_DBLVERTICALBAR: return Key::KeyPadDblVerticalBar;
            case SDLK_KP_COLON: return Key::KeyPadColon;
            case SDLK_KP_HASH: return Key::KeyPadHash;
            case SDLK_KP_SPACE: return Key::KeyPadSpace;
            case SDLK_KP_AT: return Key::KeyPadAt;
            case SDLK_KP_EXCLAM: return Key::KeyPadExclam;
            case SDLK_KP_MEMSTORE: return Key::KeyPadMemStore;
            case SDLK_KP_MEMRECALL: return Key::KeyPadMemRecall;
            case SDLK_KP_MEMCLEAR: return Key::KeyPadMemClear;
            case SDLK_KP_MEMADD: return Key::KeyPadMemAdd;
            case SDLK_KP_MEMSUBTRACT: return Key::KeyPadMemSubtract;
            case SDLK_KP_MEMMULTIPLY: return Key::KeyPadMemMultiply;
            case SDLK_KP_MEMDIVIDE: return Key::KeyPadMemDivide;
            case SDLK_KP_PLUSMINUS: return Key::KeyPadPlusMinus;
            case SDLK_KP_CLEAR: return Key::KeyPadClear;
            case SDLK_KP_CLEARENTRY: return Key::KeyPadClearEntry;
            case SDLK_KP_BINARY: return Key::KeyPadBinary;
            case SDLK_KP_OCTAL: return Key::KeyPadOctal;
            case SDLK_KP_DECIMAL: return Key::KeyPadDecimal;
            case SDLK_KP_HEXADECIMAL: return Key::KeyPadHexadecimal;

            case SDLK_LCTRL: return Key::LeftCtrl;
            case SDLK_LSHIFT: return Key::LeftShift;
            case SDLK_LALT: return Key::LeftAlt;
            case SDLK_LGUI: return Key::LeftGUI;
            case SDLK_RCTRL: return Key::RightCtrl;
            case SDLK_RSHIFT: return Key::RightShift;
            case SDLK_RALT: return Key::RightAlt;
            case SDLK_RGUI: return Key::RightGUI;

            case SDLK_MODE: return Key::Mode;
            case SDLK_AUDIONEXT: return Key::AudioNext;
            case SDLK_AUDIOPREV: return Key::AudioPrev;
            case SDLK_AUDIOSTOP: return Key::AudioStop;
            case SDLK_AUDIOPLAY: return Key::AudioPlay;
            case SDLK_AUDIOMUTE: return Key::AudioMute;
            case SDLK_MEDIASELECT: return Key::MediaSelect;
            case SDLK_WWW: return Key::WWW;
            case SDLK_MAIL: return Key::Mail;
            case SDLK_CALCULATOR: return Key::Calculator;
            case SDLK_COMPUTER: return Key::Computer;
            case SDLK_AC_SEARCH: return Key::AppControlSearch;
            case SDLK_AC_HOME: return Key::AppControlHome;
            case SDLK_AC_BACK: return Key::AppControlBack;
            case SDLK_AC_FORWARD: return Key::AppControlForward;
            case SDLK_AC_STOP: return Key::AppControlStop;
            case SDLK_AC_REFRESH: return Key::AppControlRefresh;
            case SDLK_AC_BOOKMARKS: return Key::AppControlBookmarks;

            case SDLK_BRIGHTNESSDOWN: return Key::BrightnessDown;
            case SDLK_BRIGHTNESSUP: return Key::BrightnessUp;
            case SDLK_DISPLAYSWITCH: return Key::DisplaySwitch;
            case SDLK_KBDILLUMTOGGLE: return Key::KbdIllumToggle;
            case SDLK_KBDILLUMDOWN: return Key::KbdIllumDown;
            case SDLK_KBDILLUMUP: return Key::KbdIllumUp;
            case SDLK_EJECT: return Key::Eject;
            case SDLK_SLEEP: return Key::Sleep;
            case SDLK_APP1: return Key::App1;
            case SDLK_APP2: return Key::App2;
            case SDLK_AUDIOREWIND: return Key::AudioRewind;
            case SDLK_AUDIOFASTFORWARD: return Key::AudioFastForward;
            case SDLK_SOFTLEFT: return Key::SoftLeft;
            case SDLK_SOFTRIGHT: return Key::SoftRight;
            case SDLK_CALL: return Key::Call;
            case SDLK_ENDCALL: return Key::EndCall;

            default: return Key::Unknown;
        }
    }

    [[nodiscard]] inline Mouse FromSDLMouse(const u8 mouse) noexcept
    {
        switch (mouse) {
            using enum Mouse;
            case SDL_BUTTON_LEFT: return LeftMouse;
            case SDL_BUTTON_MIDDLE: return MiddleMouse;
            case SDL_BUTTON_RIGHT: return RightMouse;
            case SDL_BUTTON_X1: return X1Mouse;
            case SDL_BUTTON_X2: return X2Mouse;
        }
        return {};
    }

    static u32 ToSDLWindowFlags(const WindowFlags& flags) noexcept
    {
        u32 sdl_flags = 0;
        if (flags & WindowFlags::Visible)
            sdl_flags |= SDL_WINDOW_SHOWN;
        if (flags & WindowFlags::Hidden)
            sdl_flags |= SDL_WINDOW_HIDDEN;
        if (flags & WindowFlags::Resizable)
            sdl_flags |= SDL_WINDOW_RESIZABLE;
        if (flags & WindowFlags::Borderless)
            sdl_flags |= SDL_WINDOW_BORDERLESS;
        if (flags & WindowFlags::Minimized)
            sdl_flags |= SDL_WINDOW_MINIMIZED;
        if (flags & WindowFlags::Maximized)
            sdl_flags |= SDL_WINDOW_MAXIMIZED;
        if (flags & WindowFlags::SkipTaskbar)
            sdl_flags |= SDL_WINDOW_SKIP_TASKBAR;
        if (flags & WindowFlags::OpenGLContext)
            sdl_flags |= SDL_WINDOW_OPENGL;
        return sdl_flags;
    }

    void serialize(Archive& ar, WindowProperties& properties)
    {
        ar("title", properties.title);
        ar("width", properties.width);
        ar("height", properties.height);
        ar("pos_x", properties.pos_x);
        ar("pos_y", properties.pos_y);
        ar("frame_cap", properties.frame_cap);
        ar("vsync", properties.vsync);
        if (ar.saving()) {
            u16 flags = (u16)properties.flags;
            ar("flags", flags);
        } else {
            u16 flags;
            ar("flags", flags);
            properties.flags = (WindowFlags)flags;
        }
        ar("borderless", properties.borderless);
    }

    Window::Window()
    {
    }

    Window::~Window()
    {
        // Release cursors.
        for (const auto& e : sdl_cursors_)
            if (e)
                SDL_FreeCursor(e);

        SDL_GL_DeleteContext(gl_context_);
        SDL_DestroyWindow(sdl_window_);
        SDL_Quit();
    }

    void Window::init(const WindowProperties& window_info, const void* native_window)
    {
        flags_         = ToSDLWindowFlags(window_info.flags);
        native_window_ = native_window;
        title_         = window_info.title;
        width_         = window_info.width;
        height_        = window_info.height;

        if (window_info.flags & WindowFlags::PositionCentre) {
            pos_x_ = SDL_WINDOWPOS_CENTERED;
            pos_y_ = SDL_WINDOWPOS_CENTERED;
        } else {
            pos_x_ = window_info.pos_x;
            pos_y_ = window_info.pos_y;
        }

        frame_cap_   = window_info.frame_cap;
        fps_         = 0;
        frame_count_ = 0;
        tp1_         = std::chrono::system_clock::now();
        tp2_         = tp1_;

        // Initialize SDL and OpenGL
        if (SDL_Init(SDL_INIT_VIDEO) < 0) {
            throw SDLException("SDL Failed to initialize.\n\tSDL Error: {}", SDL_GetError());
            // SDLThrowError(__LINE__, "ERROR: FAILED TO INITIALIZE SDL!");
            return;
        }
        SDL_ClearError();

        // Tell SDL to use OpenGL 3.3.0 Core.
#ifdef CX_PLATFORM_OSX
        SDL_GL_SetAttribute(SDL_GL_CONTEXT_FLAGS,
                            SDL_GL_CONTEXT_FORWARD_COMPATIBLE_FLAG); // Always
                                                                     // required
                                                                     // on Mac
#endif
        SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 4);
        SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 5);
        SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_CORE);
        SDL_GL_SetAttribute(SDL_GL_DOUBLEBUFFER, 1);
        SDL_GL_SetAttribute(SDL_GL_DEPTH_SIZE, 24);
        SDL_GL_SetAttribute(SDL_GL_STENCIL_SIZE, 8);

        // Create SDL Window.
        if (native_window_) {
            // sdl_window_ = SDL_CreateWindowFrom(native_window_, flags_ |
            // SDL_WINDOW_OPENGL);
            throw SDLException("Native windows are not supported");
        } else
            sdl_window_ = SDL_CreateWindow(title_.c_str(), pos_x_, pos_y_, width_, height_,
                                           flags_ | SDL_WINDOW_OPENGL | SDL_WINDOW_ALLOW_HIGHDPI);

        if (!sdl_window_) {
            throw SDLException("Failed to create an SDL window.\n\tSDL Error: {}", SDL_GetError());
        }
        SDL_ClearError();

        // Create OpenGL context from SDL Window
        gl_context_ = SDL_GL_CreateContext(sdl_window_);
        if (!gl_context_) {
            throw SDLException("Failed to create an OpenGL context from the SDL "
                               "window.\n\tSDL Error: {}",
                               SDL_GetError());
        }
        SDL_ClearError();

        // Enable VSync
        SDL_GL_SetSwapInterval((window_info.vsync) ? 1 : 0);

        // Initialize GLAD
        if (gladLoadGLLoader((GLADloadproc)SDL_GL_GetProcAddress)) {
            info("OpenGL loaded");
            info("Vendor:\t\t{}", (const char*)glGetString(GL_VENDOR));
            info("Renderer:\t\t{}", (const char*)glGetString(GL_RENDERER));
            info("Version:\t\t{}", (const char*)glGetString(GL_VERSION));
        } else {
            throw GLADException();
        }

        // TODO: Write a logging library.
        info("Window subsystem initialized.");
    }

    void Window::sdl_check_error([[maybe_unused]] const i32 line)
    {
#ifdef CX_CONFIG_DEBUG
        const char* error = SDL_GetError();
        if (*error != 0) {
            fatal("SDL ERROR @ LINE {}: {}", line, error);
            if (line != -1)
                fatal(" + line: {}", line);
            SDL_ClearError();
        }
#endif
    }

    void Window::sdl_throw_error(const i32 line, const std::string_view errorMessage)
    {
        fatal("SDL ERROR @ LINE {}: {} -> {}", line, errorMessage, SDL_GetError());
        SDL_Quit();
        exit(-1);
    }

    void Window::process_events()
    {
        static Engine&     app   = Engine::get();
        static ImGuiLayer* imgui = app.imgui_layer();

        while (SDL_PollEvent(&sdl_event_)) {
            if (imgui)
                ImGui_ImplSDL2_ProcessEvent(&sdl_event_);

            switch (sdl_event_.type) {
                case SDL_QUIT: {
                    Engine::get().stop();
                    break;
                }
                case SDL_WINDOWEVENT: {
                    switch (sdl_event_.window.event) {
                        case SDL_WINDOWEVENT_CLOSE: {
                            Engine::get().stop();
                            break;
                        }
                        case SDL_WINDOWEVENT_RESIZED: {
                            i32 width  = sdl_event_.window.data1;
                            i32 height = sdl_event_.window.data2;
                            width_     = width;
                            height_    = height;

                            WindowResizeEvent e{ width, height };
                            if (event_callback_) {
                                event_callback_(e);
                            }
                            break;
                        }
                    }
                }
                case SDL_MOUSEMOTION: {
                    MouseMoveEvent e{ FromSDLMouse(sdl_event_.button.button), sdl_event_.motion.x,
                                      sdl_event_.motion.y };
                    if (event_callback_) {
                        event_callback_(e);
                    }
                    break;
                }
                case SDL_MOUSEBUTTONDOWN: {
                    MouseDownEvent e{ FromSDLMouse(sdl_event_.button.button), sdl_event_.motion.x,
                                      sdl_event_.motion.y };
                    if (event_callback_) {
                        event_callback_(e);
                    }
                    break;
                }
                case SDL_MOUSEBUTTONUP: {
                    MouseUpEvent e{ FromSDLMouse(sdl_event_.button.button), sdl_event_.motion.x, sdl_event_.motion.y };
                    if (event_callback_) {
                        event_callback_(e);
                    }
                    break;
                }
                case SDL_MOUSEWHEEL: {
                    MouseScrollEvent e{ FromSDLMouse(sdl_event_.button.button - 1), sdl_event_.motion.x,
                                        sdl_event_.motion.y, sdl_event_.wheel.x, sdl_event_.wheel.y };
                    if (event_callback_) {
                        event_callback_(e);
                    }
                    break;
                }
                case SDL_KEYDOWN: {
                    KeyDownEvent e{ FromSDLKeycode(sdl_event_.key.keysym.sym),
                                    static_cast<bool>(sdl_event_.key.repeat) };
                    if (event_callback_) {
                        event_callback_(e);
                    }
                    break;
                }
                case SDL_KEYUP: {
                    KeyUpEvent e{ FromSDLKeycode(sdl_event_.key.keysym.sym) };
                    if (event_callback_) {
                        event_callback_(e);
                    }
                    break;
                }
            }
        }
    }

    // TODO: Remove, legacy code.
    void Window::on_update([[maybe_unused]] const f32 delta_time)
    {
        // static opengl::FrameBufferProperties props(get_width(), get_height(), {
        // opengl::TextureFormat::RGBA8, opengl::TextureFormat::RedInt32 }); static
        // opengl::FrameBuffer* fb = new opengl::FrameBuffer(props);
        renderer_->set_clear_colour(0.2f, 0.2f, 0.2f, 1.0f);
        renderer_->clear();

#ifdef CODEX_CONF_DEBUG
        DebugDraw::Begin();
#endif

        // Poll events
        process_events();

        // Update scene
#ifdef CODEX_CONF_DEBUG
        DebugDraw::Render();
#endif

        if (frame_cap_ > 0)
            SDL_Delay((u32)(1.0f / (f32)frame_cap_ * 1000.0f));
        frame_count_++;
        SDL_GL_SwapWindow(sdl_window_);
    }

    void Window::swap_buffers()
    {
        frame_count_++;
        SDL_GL_SwapWindow(sdl_window_);
    }

    void Window::on_window_resize_event(const i32 new_width, const i32 new_height)
    {
        if (new_width == 0 || new_height == 0)
            return;
        glViewport(0, 0, new_width, new_height);
    }

    SDL_Cursor* Window::sdl_cursor(const SystemCursor cursor) noexcept
    {
        auto* cursor_ptr = sdl_cursors_[(usize)cursor];
        if (cursor_ptr)
            return cursor_ptr;
        else {
            cursor_ptr                  = SDL_CreateSystemCursor((SDL_SystemCursor)cursor);
            sdl_cursors_[(usize)cursor] = cursor_ptr;
        }
        return cursor_ptr;
    }
} // namespace codex
