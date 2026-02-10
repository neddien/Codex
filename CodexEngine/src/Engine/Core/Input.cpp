#include "Public/Input.h"

#include <Engine/Core/Application.h>
#include <Engine/Events/KeyEvent.h>
#include <Engine/Events/MouseEvent.h>

namespace codex {
    using namespace codex::events;

    [[nodiscard]] inline SDL_Keycode ToSDLKeycode(Key key)
    {
        switch (key)
        {
            case Key::Unknown: return SDLK_UNKNOWN;

            case Key::Return: return SDLK_RETURN;
            case Key::CapsLock: return SDLK_CAPSLOCK;
            case Key::Escape: return SDLK_ESCAPE;
            case Key::Backspace: return SDLK_BACKSPACE;
            case Key::Tab: return SDLK_TAB;
            case Key::Space: return SDLK_SPACE;
            case Key::Exclaim: return SDLK_EXCLAIM;
            case Key::DoubleQuote: return SDLK_QUOTEDBL;
            case Key::Pound: return SDLK_HASH;
            case Key::Percent: return SDLK_PERCENT;
            case Key::Dollar: return SDLK_DOLLAR;
            case Key::Ampersand: return SDLK_AMPERSAND;
            case Key::SingleQuote: return SDLK_QUOTE;
            case Key::LeftParen: return SDLK_LEFTPAREN;
            case Key::RightParen: return SDLK_RIGHTPAREN;
            case Key::Asterisk: return SDLK_ASTERISK;
            case Key::Plus: return SDLK_PLUS;
            case Key::Comma: return SDLK_COMMA;
            case Key::Minus: return SDLK_MINUS;
            case Key::Period: return SDLK_PERIOD;
            case Key::ForwardSlash: return SDLK_SLASH;
            case Key::Num0: return SDLK_0;
            case Key::Num1: return SDLK_1;
            case Key::Num2: return SDLK_2;
            case Key::Num3: return SDLK_3;
            case Key::Num4: return SDLK_4;
            case Key::Num5: return SDLK_5;
            case Key::Num6: return SDLK_6;
            case Key::Num7: return SDLK_7;
            case Key::Num8: return SDLK_8;
            case Key::Num9: return SDLK_9;
            case Key::Colon: return SDLK_COLON;
            case Key::SemiColon: return SDLK_SEMICOLON;
            case Key::Less: return SDLK_LESS;
            case Key::Equals: return SDLK_EQUALS;
            case Key::Greater: return SDLK_GREATER;
            case Key::QuestionMark: return SDLK_QUESTION;
            case Key::At: return SDLK_AT;

            case Key::LeftBracket: return SDLK_LEFTBRACKET;
            case Key::BackSlash: return SDLK_BACKSLASH;
            case Key::RightBracket: return SDLK_RIGHTBRACKET;
            case Key::Caret: return SDLK_CARET;
            case Key::Underscore: return SDLK_UNDERSCORE;
            case Key::BackQuote: return SDLK_BACKQUOTE;

            case Key::A: return SDLK_a;
            case Key::B: return SDLK_b;
            case Key::C: return SDLK_c;
            case Key::D: return SDLK_d;
            case Key::E: return SDLK_e;
            case Key::F: return SDLK_f;
            case Key::G: return SDLK_g;
            case Key::H: return SDLK_h;
            case Key::I: return SDLK_i;
            case Key::J: return SDLK_j;
            case Key::K: return SDLK_k;
            case Key::L: return SDLK_l;
            case Key::M: return SDLK_m;
            case Key::N: return SDLK_n;
            case Key::O: return SDLK_o;
            case Key::P: return SDLK_p;
            case Key::Q: return SDLK_q;
            case Key::R: return SDLK_r;
            case Key::S: return SDLK_s;
            case Key::T: return SDLK_t;
            case Key::U: return SDLK_u;
            case Key::V: return SDLK_v;
            case Key::W: return SDLK_w;
            case Key::X: return SDLK_x;
            case Key::Y: return SDLK_y;
            case Key::Z: return SDLK_z;

            case Key::F1: return SDLK_F1;
            case Key::F2: return SDLK_F2;
            case Key::F3: return SDLK_F3;
            case Key::F4: return SDLK_F4;
            case Key::F5: return SDLK_F5;
            case Key::F6: return SDLK_F6;
            case Key::F7: return SDLK_F7;
            case Key::F8: return SDLK_F8;
            case Key::F9: return SDLK_F9;
            case Key::F10: return SDLK_F10;
            case Key::F11: return SDLK_F11;
            case Key::F12: return SDLK_F12;

            case Key::PrintScreen: return SDLK_PRINTSCREEN;
            case Key::ScrollLock: return SDLK_SCROLLLOCK;
            case Key::Pause: return SDLK_PAUSE;
            case Key::Insert: return SDLK_INSERT;
            case Key::Home: return SDLK_HOME;
            case Key::PageUp: return SDLK_PAGEUP;
            case Key::Delete: return SDLK_DELETE;
            case Key::End: return SDLK_END;
            case Key::PageDown: return SDLK_PAGEDOWN;
            case Key::Right: return SDLK_RIGHT;
            case Key::Left: return SDLK_LEFT;
            case Key::Down: return SDLK_DOWN;
            case Key::Up: return SDLK_UP;

            case Key::NumLockClear: return SDLK_NUMLOCKCLEAR;
            case Key::KeyPadDivide: return SDLK_KP_DIVIDE;
            case Key::KeyPadMultiply: return SDLK_KP_MULTIPLY;
            case Key::KeyPadMinus: return SDLK_KP_MINUS;
            case Key::KeyPadPlus: return SDLK_KP_PLUS;
            case Key::KeyPadEnter: return SDLK_KP_ENTER;
            case Key::KeyPad1: return SDLK_KP_1;
            case Key::KeyPad2: return SDLK_KP_2;
            case Key::KeyPad3: return SDLK_KP_3;
            case Key::KeyPad4: return SDLK_KP_4;
            case Key::KeyPad5: return SDLK_KP_5;
            case Key::KeyPad6: return SDLK_KP_6;
            case Key::KeyPad7: return SDLK_KP_7;
            case Key::KeyPad8: return SDLK_KP_8;
            case Key::KeyPad9: return SDLK_KP_9;
            case Key::KeyPad0: return SDLK_KP_0;
            case Key::KeyPadPeriod: return SDLK_KP_PERIOD;

            case Key::Application: return SDLK_APPLICATION;
            case Key::Power: return SDLK_POWER;
            case Key::KeyPadEquals: return SDLK_KP_EQUALS;
            case Key::F13: return SDLK_F13;
            case Key::F14: return SDLK_F14;
            case Key::F15: return SDLK_F15;
            case Key::F16: return SDLK_F16;
            case Key::F17: return SDLK_F17;
            case Key::F18: return SDLK_F18;
            case Key::F19: return SDLK_F19;
            case Key::F20: return SDLK_F20;
            case Key::F21: return SDLK_F21;
            case Key::F22: return SDLK_F22;
            case Key::F23: return SDLK_F23;
            case Key::F24: return SDLK_F24;
            case Key::Execute: return SDLK_EXECUTE;
            case Key::Help: return SDLK_HELP;
            case Key::Menu: return SDLK_MENU;
            case Key::Select: return SDLK_SELECT;
            case Key::Stop: return SDLK_STOP;
            case Key::Again: return SDLK_AGAIN;
            case Key::Undo: return SDLK_UNDO;
            case Key::Cut: return SDLK_CUT;
            case Key::Copy: return SDLK_COPY;
            case Key::Paste: return SDLK_PASTE;
            case Key::Find: return SDLK_FIND;
            case Key::Mute: return SDLK_MUTE;
            case Key::VolumeUp: return SDLK_VOLUMEUP;
            case Key::VolumeDown: return SDLK_VOLUMEDOWN;
            case Key::KeyPadComma: return SDLK_KP_COMMA;
            case Key::KeyPadEqualsAs400: return SDLK_KP_EQUALSAS400;

            case Key::Alterase: return SDLK_ALTERASE;
            case Key::SysReq: return SDLK_SYSREQ;
            case Key::Cancel: return SDLK_CANCEL;
            case Key::Clear: return SDLK_CLEAR;
            case Key::Prior: return SDLK_PRIOR;
            case Key::Return2: return SDLK_RETURN2;
            case Key::Separator: return SDLK_SEPARATOR;
            case Key::Out: return SDLK_OUT;
            case Key::Oper: return SDLK_OPER;
            case Key::ClearAgain: return SDLK_CLEARAGAIN;
            case Key::CrSel: return SDLK_CRSEL;
            case Key::ExSel: return SDLK_EXSEL;

            case Key::KeyPad00: return SDLK_KP_00;
            case Key::KeyPad000: return SDLK_KP_000;
            case Key::ThousandsSeparator: return SDLK_THOUSANDSSEPARATOR;
            case Key::DecimalSeparator: return SDLK_DECIMALSEPARATOR;
            case Key::CurrencyUnit: return SDLK_CURRENCYUNIT;
            case Key::CurrencySubUnit: return SDLK_CURRENCYSUBUNIT;
            case Key::KeyPadLeftParen: return SDLK_KP_LEFTPAREN;
            case Key::KeyPadRightParen: return SDLK_KP_RIGHTPAREN;
            case Key::KeyPadLeftBrace: return SDLK_KP_LEFTBRACE;
            case Key::KeyPadRightBrace: return SDLK_KP_RIGHTBRACE;
            case Key::KeyPadTab: return SDLK_KP_TAB;
            case Key::KeyPadBackspace: return SDLK_KP_BACKSPACE;
            case Key::KeyPadA: return SDLK_KP_A;
            case Key::KeyPadB: return SDLK_KP_B;
            case Key::KeyPadC: return SDLK_KP_C;
            case Key::KeyPadD: return SDLK_KP_D;
            case Key::KeyPadE: return SDLK_KP_E;
            case Key::KeyPadF: return SDLK_KP_F;
            case Key::KeyPadXOR: return SDLK_KP_XOR;
            case Key::KeyPadPower: return SDLK_KP_POWER;
            case Key::KeyPadPercent: return SDLK_KP_PERCENT;
            case Key::KeyPadLess: return SDLK_KP_LESS;
            case Key::KeyPadGreater: return SDLK_KP_GREATER;
            case Key::KeyPadAmpersand: return SDLK_KP_AMPERSAND;
            case Key::KeyPadDblAmpersand: return SDLK_KP_DBLAMPERSAND;
            case Key::KeyPadVerticalBar: return SDLK_KP_VERTICALBAR;
            case Key::KeyPadDblVerticalBar: return SDLK_KP_DBLVERTICALBAR;
            case Key::KeyPadColon: return SDLK_KP_COLON;
            case Key::KeyPadHash: return SDLK_KP_HASH;
            case Key::KeyPadSpace: return SDLK_KP_SPACE;
            case Key::KeyPadAt: return SDLK_KP_AT;
            case Key::KeyPadExclam: return SDLK_KP_EXCLAM;
            case Key::KeyPadMemStore: return SDLK_KP_MEMSTORE;
            case Key::KeyPadMemRecall: return SDLK_KP_MEMRECALL;
            case Key::KeyPadMemClear: return SDLK_KP_MEMCLEAR;
            case Key::KeyPadMemAdd: return SDLK_KP_MEMADD;
            case Key::KeyPadMemSubtract: return SDLK_KP_MEMSUBTRACT;
            case Key::KeyPadMemMultiply: return SDLK_KP_MEMMULTIPLY;
            case Key::KeyPadMemDivide: return SDLK_KP_MEMDIVIDE;
            case Key::KeyPadPlusMinus: return SDLK_KP_PLUSMINUS;
            case Key::KeyPadClear: return SDLK_KP_CLEAR;
            case Key::KeyPadClearEntry: return SDLK_KP_CLEARENTRY;
            case Key::KeyPadBinary: return SDLK_KP_BINARY;
            case Key::KeyPadOctal: return SDLK_KP_OCTAL;
            case Key::KeyPadDecimal: return SDLK_KP_DECIMAL;
            case Key::KeyPadHexadecimal: return SDLK_KP_HEXADECIMAL;

            case Key::LeftCtrl: return SDLK_LCTRL;
            case Key::LeftShift: return SDLK_LSHIFT;
            case Key::LeftAlt: return SDLK_LALT;
            case Key::LeftGUI: return SDLK_LGUI;
            case Key::RightCtrl: return SDLK_RCTRL;
            case Key::RightShift: return SDLK_RSHIFT;
            case Key::RightAlt: return SDLK_RALT;
            case Key::RightGUI: return SDLK_RGUI;

            case Key::Mode: return SDLK_MODE;
            case Key::AudioNext: return SDLK_AUDIONEXT;
            case Key::AudioPrev: return SDLK_AUDIOPREV;
            case Key::AudioStop: return SDLK_AUDIOSTOP;
            case Key::AudioPlay: return SDLK_AUDIOPLAY;
            case Key::AudioMute: return SDLK_AUDIOMUTE;
            case Key::MediaSelect: return SDLK_MEDIASELECT;
            case Key::WWW: return SDLK_WWW;
            case Key::Mail: return SDLK_MAIL;
            case Key::Calculator: return SDLK_CALCULATOR;
            case Key::Computer: return SDLK_COMPUTER;
            case Key::AppControlSearch: return SDLK_AC_SEARCH;
            case Key::AppControlHome: return SDLK_AC_HOME;
            case Key::AppControlBack: return SDLK_AC_BACK;
            case Key::AppControlForward: return SDLK_AC_FORWARD;
            case Key::AppControlStop: return SDLK_AC_STOP;
            case Key::AppControlRefresh: return SDLK_AC_REFRESH;
            case Key::AppControlBookmarks: return SDLK_AC_BOOKMARKS;

            case Key::BrightnessDown: return SDLK_BRIGHTNESSDOWN;
            case Key::BrightnessUp: return SDLK_BRIGHTNESSUP;
            case Key::DisplaySwitch: return SDLK_DISPLAYSWITCH;
            case Key::KbdIllumToggle: return SDLK_KBDILLUMTOGGLE;
            case Key::KbdIllumDown: return SDLK_KBDILLUMDOWN;
            case Key::KbdIllumUp: return SDLK_KBDILLUMUP;
            case Key::Eject: return SDLK_EJECT;
            case Key::Sleep: return SDLK_SLEEP;
            case Key::App1: return SDLK_APP1;
            case Key::App2: return SDLK_APP2;
            case Key::AudioRewind: return SDLK_AUDIOREWIND;
            case Key::AudioFastForward: return SDLK_AUDIOFASTFORWARD;
            case Key::SoftLeft: return SDLK_SOFTLEFT;
            case Key::SoftRight: return SDLK_SOFTRIGHT;
            case Key::Call: return SDLK_CALL;
            case Key::EndCall: return SDLK_ENDCALL;

            default: return SDLK_UNKNOWN;
        }
    }

    Input*                        Input::m_Instance = nullptr;
    std::unordered_map<Key, bool> Input::m_KeysDown;
    std::bitset<3>                Input::m_ButtonsDown;

    Input* Input::Get()
    {
        if (!m_Instance)
        {
            m_Instance = new Input();
            lgx::Get("engine").Log(lgx::Info, "Input subsystem initialized.");
        }
        return m_Instance;
    }

    void Input::Dispose()
    {
        if (m_Instance)
        {
            delete m_Instance;
            m_Instance = nullptr;
            lgx::Get("engine").Log(lgx::Info, "Input subsystem disposed.");
        }
    }

    bool Input::OnKeyDown_Event(const KeyDownEvent event)
    {
        m_Instance->m_KeysDown[event.GetKey()] = true;
        return false;
    }

    bool Input::OnKeyUp_Event(const KeyUpEvent event)
    {
        m_Instance->m_KeysDown[event.GetKey()] = false;
        return false;
    }

    bool Input::OnMouseMove_Event(const MouseMoveEvent event)
    {
        m_MouseLastPosX = m_MousePosX;
        m_MouseLastPosY = m_MousePosY;
        m_MousePosX     = event.GetX();
        m_MousePosY     = event.GetY();
        m_MouseDragging = m_ButtonsDown.any();
        return false;
    }

    bool Input::OnMouseUp_Event(const MouseUpEvent event)
    {
        m_ButtonsDown[(usize)event.GetMouseButton()] = false;
        m_Instance->m_MouseDragging                  = false;
        return false;
    }

    bool Input::OnMouseDown_Event(const MouseDownEvent event)
    {
        m_ButtonsDown[(usize)event.GetMouseButton()] = true;
        return false;
    }

    bool Input::OnMouseScroll_Event(const MouseScrollEvent event)
    {
        m_MouseScrollX = event.GetOffsetX();
        m_MouseScrollY = event.GetOffsetY();
        return false;
    }

    bool Input::IsKeyDown(const Key key)
    {
        return m_KeysDown[key];
    }

    bool Input::IsMouseDown(const Mouse button)
    {
        return m_ButtonsDown[(usize)button];
    }

    Vector2 Input::GetScreenMousePos() noexcept
    {
        Vector2 vec;
        SDL_GetGlobalMouseState(&vec.x, &vec.y);
        return vec;
    }
} // namespace codex
