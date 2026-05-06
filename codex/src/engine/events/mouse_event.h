#pragma once

#include <engine/core/public/input.h>

#include "event.h"

namespace codex::events {
    class MouseEvent : public Event
    {
    public:
        [[nodiscard]] inline Mouse mouse_button() const noexcept { return button_; }
        [[nodiscard]] inline i32   x() const noexcept { return mouse_x_; }
        [[nodiscard]] inline i32   y() const noexcept { return mouse_y_; }

        EVENT_CLASS_CATEGORY(EventCategoryMouse | EventCategoryInput)

    public:
        MouseEvent(const Mouse button, const i32 mouse_x, const i32 mouse_y)
            : button_(button)
            , mouse_x_(mouse_x)
            , mouse_y_(mouse_y)
        {
        }

        Mouse button_;
        i32   mouse_x_;
        i32   mouse_y_;
    };

    class MouseMoveEvent : public MouseEvent
    {
    public:
        using MouseEvent::MouseEvent;

    public:
        [[nodiscard]] std::string to_string() const noexcept override
        {
            return fmt::format("MouseMoveEvent: Button: {}, Pos: ({}, {})", enum_name(button_), mouse_x_, mouse_y_);
        }

        EVENT_CLASS_TYPE(MouseMove)
        EVENT_CLASS_CATEGORY(EventCategoryMouse | EventCategoryInput)
    };

    class MouseScrollEvent : public MouseEvent
    {
    public:
        using MouseEvent::MouseEvent;

        MouseScrollEvent(const Mouse button, const i32 mouse_x, const i32 mouse_y, const i32 offset_x,
                         const i32 offset_y)
            : MouseEvent(button, mouse_x, mouse_y)
            , offset_x_(offset_x)
            , offset_y_(offset_y)
        {
        }

    public:
        [[nodiscard]] inline i32 offset_x() const noexcept { return offset_x_; }
        [[nodiscard]] inline i32 offset_y() const noexcept { return offset_y_; }

    public:
        [[nodiscard]] std::string to_string() const noexcept override
        {
            return fmt::format("MouseScrollEvent: Offset: ({}, {})", offset_x_, offset_y_);
        }

        EVENT_CLASS_TYPE(MouseScroll)
        EVENT_CLASS_CATEGORY(EventCategoryMouse | EventCategoryInput)

    private:
        i32 offset_x_;
        i32 offset_y_;
    };

    class CODEX_API MouseDownEvent : public MouseEvent
    {
    public:
        using MouseEvent::MouseEvent;

    public:
        [[nodiscard]] std::string to_string() const noexcept override
        {
            return fmt::format("MouseDownEvent: Button({})", enum_name(button_));
        }

        EVENT_CLASS_TYPE(MouseDown)
        EVENT_CLASS_CATEGORY(EventCategoryMouse | EventCategoryInput)
    };

    class CODEX_API MouseUpEvent : public MouseEvent
    {
    public:
        using MouseEvent::MouseEvent;

    public:
        [[nodiscard]] std::string to_string() const noexcept override
        {
            return fmt::format("MouseUpEvent: Button({})", enum_name(button_));
        }

        EVENT_CLASS_TYPE(MouseUp)
        EVENT_CLASS_CATEGORY(EventCategoryMouse | EventCategoryInput)
    };
} // namespace codex::events
