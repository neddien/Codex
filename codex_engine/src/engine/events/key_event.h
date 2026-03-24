#pragma once

#include <engine/core/public/input.h>

#include "event.h"

namespace codex::events {
    class CODEX_API KeyEvent : public Event
    {
    protected:
        explicit KeyEvent(const Key key)
            : key_(key) {};

    public:
        [[nodiscard]] Key key() const noexcept { return key_; }

        EVENT_CLASS_CATEGORY(EventCategoryKeyboard | EventCategoryInput)

    protected:
        Key key_;
    };

    class CODEX_API KeyDownEvent : public KeyEvent
    {
    public:
        KeyDownEvent(const Key key, const bool is_repeat)
            : KeyEvent(key)
            , is_repeat_(is_repeat)
        {
        }

    public:
        [[nodiscard]] bool        is_repeat() const noexcept { return is_repeat_; }
        [[nodiscard]] std::string to_string() const noexcept override
        {
            return fmt::format("KeyDownEvent: Key({}), IsRepeat({})", key_, is_repeat_);
        }

        EVENT_CLASS_TYPE(KeyDown);

    private:
        bool is_repeat_;
    };

    class CODEX_API KeyUpEvent : public KeyEvent
    {
    public:
        explicit KeyUpEvent(const Key key)
            : KeyEvent(key)
        {
        }

    public:
        [[nodiscard]] std::string to_string() const noexcept override
        {
            return fmt::format("KeyUpEvent: Key({})", key_);
        }

        EVENT_CLASS_TYPE(KeyUp);
    };
} // namespace codex::events
