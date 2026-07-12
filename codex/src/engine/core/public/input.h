#pragma once

#include "geometry.h"

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

    class CODEX_API Input : public Loggable<"Input">
    {
    private:
        static Input*                        s_instance_;
        static std::unordered_map<Key, bool> s_keys_down_;
        static std::bitset<3>                s_buttons_down_;

    private:
        i32  mouse_pos_x_      = 0;
        i32  mouse_pos_y_      = 0;
        i32  mouse_last_pos_x_ = 0;
        i32  mouse_last_pos_y_ = 0;
        i32  mouse_scroll_x_   = 0;
        i32  mouse_scroll_y_   = 0;
        bool mouse_dragging_   = false;

    public:
        static Input* get();
        static void   dispose();
        static bool   is_key_down(const Key key);
        static bool   is_mouse_down(const Mouse button);
        static ivec2  screen_mouse_pos() noexcept;

    public:
        static inline i32  mouse_x() noexcept { return s_instance_->mouse_pos_x_; }
        static inline i32  mouse_y() noexcept { return s_instance_->mouse_pos_y_; }
        static inline vec2 mouse_delta() noexcept
        {
            return vec2((f32)s_instance_->mouse_last_pos_x_ - (f32)s_instance_->mouse_pos_x_,
                        (f32)s_instance_->mouse_last_pos_y_ - (f32)s_instance_->mouse_pos_y_);
        }
        static inline f32 mouse_delta_x() noexcept
        {
            return (f32)s_instance_->mouse_last_pos_x_ - (f32)s_instance_->mouse_pos_x_;
        }
        static inline f32 mouse_delta_y() noexcept
        {
            return (f32)s_instance_->mouse_last_pos_y_ - (f32)s_instance_->mouse_pos_y_;
        }
        static inline ivec2 mouse_pos() noexcept { return ivec2(mouse_x(), mouse_y()); }

        static inline i32  scroll_x() noexcept { return s_instance_->mouse_scroll_x_; }
        static inline i32  scroll_y() noexcept { return s_instance_->mouse_scroll_y_; }
        static inline bool is_mouse_dragging() noexcept { return s_instance_->mouse_dragging_; }
        static inline void end_frame() noexcept
        {
            s_instance_->mouse_last_pos_x_ = s_instance_->mouse_pos_x_;
            s_instance_->mouse_last_pos_y_ = s_instance_->mouse_pos_y_;
            s_instance_->mouse_scroll_x_   = 0;
            s_instance_->mouse_scroll_y_   = 0;
        }

    public:
        bool on_key_down_event(const events::KeyDownEvent event);
        bool on_key_up_event(const events::KeyUpEvent event);
        bool on_mouse_down_event(const events::MouseDownEvent event);
        bool on_mouse_up_event(const events::MouseUpEvent event);
        bool on_mouse_move_event(const events::MouseMoveEvent event);
        bool on_mouse_scroll_event(const events::MouseScrollEvent event);
    };

} // namespace codex

namespace fmt {
    template <>
    struct formatter<codex::Key> : formatter<std::string_view>
    {
        auto format(const codex::Key& key, format_context& ctx) const
        {
            return formatter<string_view>::format(codex::enum_name(key), ctx);
        }
    };
} // namespace fmt
