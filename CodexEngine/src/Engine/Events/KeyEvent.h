#pragma once

#include <sdafx.h>

#include <Engine/Core/Public/Input.h>

#include "Event.h"

namespace codex::events {
    class CODEX_API KeyEvent : public Event
    {
    protected:
        Key m_Key;

    protected:
        KeyEvent(const Key key)
            : m_Key(key){};

    public:
        inline Key GetKey() const noexcept { return m_Key; }

        EVENT_CLASS_CATEGORY(EventCategoryKeyboard | EventCategoryInput)
    };

    class CODEX_API KeyDownEvent : public KeyEvent
    {
    private:
        bool m_IsRepeat;

    public:
        KeyDownEvent(const Key key, const bool isRepeat)
            : KeyEvent(key)
            , m_IsRepeat(isRepeat)
        {
        }

    public:
        bool        IsRepeat() const noexcept { return m_IsRepeat; }
        std::string ToString() const noexcept override
        {
            return fmt::format("KeyDownEvent: Key({}), IsRepeat({})", m_Key, m_IsRepeat);
        }

        EVENT_CLASS_TYPE(KeyDown);
    };

    class CODEX_API KeyUpEvent : public KeyEvent
    {
    public:
        KeyUpEvent(const Key key)
            : KeyEvent(key)
        {
        }

    public:
        std::string ToString() const noexcept override { return fmt::format("KeyUpEvent: Key({})", m_Key); }

        EVENT_CLASS_TYPE(KeyUp);
    };
} // namespace codex::events
