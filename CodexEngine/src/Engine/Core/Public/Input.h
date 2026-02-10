#pragma once

#include <sdafx.h>

#include "Geomtryd.h"

namespace codex {
    // Forward decelrations.
    namespace events {
        class KeyDownEvent;
        class KeyUpEvent;
        class MouseDownEvent;
        class MouseUpEvent;
        class MouseMoveEvent;
        class MouseScrollEvent;
    } // namespace events

    enum class Key
    {
        Unknown,

        Return,
        CapsLock,
        Escape,
        Backspace,
        Tab,
        Space,
        Exclaim,
        DoubleQuote,
        Pound,
        Percent,
        Dollar,
        Ampersand,
        SingleQuote,
        LeftParen,
        RightParen,
        Asterisk,
        Plus,
        Comma,
        Minus,
        Period,
        ForwardSlash,
        Num0,
        Num1,
        Num2,
        Num3,
        Num4,
        Num5,
        Num6,
        Num7,
        Num8,
        Num9,
        Colon,
        SemiColon,
        Less,
        Equals,
        Greater,
        QuestionMark,
        At,

        LeftBracket,
        BackSlash,
        RightBracket,
        Caret,
        Underscore,
        BackQuote,

        A,
        B,
        C,
        D,
        E,
        F,
        G,
        H,
        I,
        J,
        K,
        L,
        M,
        N,
        O,
        P,
        Q,
        R,
        S,
        T,
        U,
        V,
        W,
        X,
        Y,
        Z,

        F1,
        F2,
        F3,
        F4,
        F5,
        F6,
        F7,
        F8,
        F9,
        F10,
        F11,
        F12,

        PrintScreen,
        ScrollLock,
        Pause,
        Insert,
        Home,
        PageUp,
        Delete,
        End,
        PageDown,
        Right,
        Left,
        Down,
        Up,

        NumLockClear,
        KeyPadDivide,
        KeyPadMultiply,
        KeyPadMinus,
        KeyPadPlus,
        KeyPadEnter,
        KeyPad1,
        KeyPad2,
        KeyPad3,
        KeyPad4,
        KeyPad5,
        KeyPad6,
        KeyPad7,
        KeyPad8,
        KeyPad9,
        KeyPad0,
        KeyPadPeriod,

        Application,
        Power,
        KeyPadEquals,
        F13,
        F14,
        F15,
        F16,
        F17,
        F18,
        F19,
        F20,
        F21,
        F22,
        F23,
        F24,
        Execute,
        Help,
        Menu,
        Select,
        Stop,
        Again,
        Undo,
        Cut,
        Copy,
        Paste,
        Find,
        Mute,
        VolumeUp,
        VolumeDown,
        KeyPadComma,
        KeyPadEqualsAs400,

        Alterase,
        SysReq,
        Cancel,
        Clear,
        Prior,
        Return2,
        Separator,
        Out,
        Oper,
        ClearAgain,
        CrSel,
        ExSel,

        KeyPad00,
        KeyPad000,
        ThousandsSeparator,
        DecimalSeparator,
        CurrencyUnit,
        CurrencySubUnit,
        KeyPadLeftParen,
        KeyPadRightParen,
        KeyPadLeftBrace,
        KeyPadRightBrace,
        KeyPadTab,
        KeyPadBackspace,
        KeyPadA,
        KeyPadB,
        KeyPadC,
        KeyPadD,
        KeyPadE,
        KeyPadF,
        KeyPadXOR,
        KeyPadPower,
        KeyPadPercent,
        KeyPadLess,
        KeyPadGreater,
        KeyPadAmpersand,
        KeyPadDblAmpersand,
        KeyPadVerticalBar,
        KeyPadDblVerticalBar,
        KeyPadColon,
        KeyPadHash,
        KeyPadSpace,
        KeyPadAt,
        KeyPadExclam,
        KeyPadMemStore,
        KeyPadMemRecall,
        KeyPadMemClear,
        KeyPadMemAdd,
        KeyPadMemSubtract,
        KeyPadMemMultiply,
        KeyPadMemDivide,
        KeyPadPlusMinus,
        KeyPadClear,
        KeyPadClearEntry,
        KeyPadBinary,
        KeyPadOctal,
        KeyPadDecimal,
        KeyPadHexadecimal,

        LeftCtrl,
        LeftShift,
        LeftAlt,
        LeftGUI,
        RightCtrl,
        RightShift,
        RightAlt,
        RightGUI,

        Mode,
        AudioNext,
        AudioPrev,
        AudioStop,
        AudioPlay,
        AudioMute,
        MediaSelect,
        WWW,
        Mail,
        Calculator,
        Computer,
        AppControlSearch,
        AppControlHome,
        AppControlBack,
        AppControlForward,
        AppControlStop,
        AppControlRefresh,
        AppControlBookmarks,

        BrightnessDown,
        BrightnessUp,
        DisplaySwitch,
        KbdIllumToggle,
        KbdIllumDown,
        KbdIllumUp,
        Eject,
        Sleep,
        App1,
        App2,
        AudioRewind,
        AudioFastForward,
        SoftLeft,
        SoftRight,
        Call,
        EndCall,
    };

    enum class Mouse : u8
    {
        LeftMouse,
        MiddleMouse,
        RightMouse,
        X1Mouse,
        X2Mouse,
    };

    class CODEX_API Input
    {
    private:
        static Input*                        m_Instance;
        static std::unordered_map<Key, bool> m_KeysDown;
        static std::bitset<3>                m_ButtonsDown;

    private:
        i32  m_MousePosX     = 0;
        i32  m_MousePosY     = 0;
        i32  m_MouseLastPosX = 0;
        i32  m_MouseLastPosY = 0;
        i32  m_MouseScrollX  = 0;
        i32  m_MouseScrollY  = 0;
        bool m_MouseDragging = false;

    public:
        static Input*  Get();
        static void    Dispose();
        static bool    IsKeyDown(const Key key);
        static bool    IsMouseDown(const Mouse button);
        static Vector2 GetScreenMousePos() noexcept;

    public:
        static inline i32      GetMouseX() noexcept { return m_Instance->m_MousePosX; }
        static inline i32      GetMouseY() noexcept { return m_Instance->m_MousePosY; }
        static inline Vector2f GetMouseDelta() noexcept
        {
            return Vector2f((f32)m_Instance->m_MouseLastPosX - (f32)m_Instance->m_MousePosX,
                            (f32)m_Instance->m_MouseLastPosY - (f32)m_Instance->m_MousePosY);
        }
        static inline f32 GetMouseDeltaX() noexcept
        {
            return (f32)m_Instance->m_MouseLastPosX - (f32)m_Instance->m_MousePosX;
        }
        static inline f32 GetMouseDeltaY() noexcept
        {
            return (f32)m_Instance->m_MouseLastPosY - (f32)m_Instance->m_MousePosY;
        }
        static inline Vector2 GetMousePos() noexcept { return Vector2(GetMouseX(), GetMouseY()); }

        static inline i32  GetScrollX() noexcept { return m_Instance->m_MouseScrollX; }
        static inline i32  GetScrollY() noexcept { return m_Instance->m_MouseScrollY; }
        static inline bool IsMouseDragging() noexcept { return m_Instance->m_MouseDragging; }
        static inline void EndFrame() noexcept
        {
            m_Instance->m_MouseLastPosX = m_Instance->m_MousePosX;
            m_Instance->m_MouseLastPosY = m_Instance->m_MousePosY;
            m_Instance->m_MouseScrollX  = 0;
            m_Instance->m_MouseScrollY  = 0;
        }

    public:
        bool OnKeyDown_Event(const events::KeyDownEvent event);
        bool OnKeyUp_Event(const events::KeyUpEvent event);
        bool OnMouseDown_Event(const events::MouseDownEvent event);
        bool OnMouseUp_Event(const events::MouseUpEvent event);
        bool OnMouseMove_Event(const events::MouseMoveEvent event);
        bool OnMouseScroll_Event(const events::MouseScrollEvent event);
    };

} // namespace codex

namespace fmt {
    template <>
    struct formatter<codex::Key> : formatter<std::string_view>
    {
        auto format(const codex::Key& key, format_context& ctx) const
        {
            return formatter<string_view>::format(codex::EnumName(key), ctx);
        }
    };
} // namespace fmt
