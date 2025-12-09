#ifndef CODEX_EVENTS_MOUSE_EVENT_H
#define CODEX_EVENTS_MOUSE_EVENT_H

#include <sdafx.h>

#include <Engine/Core/Public/Input.h>

#include "Event.h"

namespace codex::events {
    class MouseEvent : public Event
    {
    protected:
        Mouse m_Button;
        i32   m_MouseX;
        i32   m_MouseY;

    public:
        MouseEvent(const Mouse button, const i32 mouseX, const i32 mouseY)
            : m_Button(button)
            , m_MouseX(mouseX)
            , m_MouseY(mouseY)
        {
        }

    public:
        [[nodiscard]] inline Mouse GetMouseButton() const noexcept { return m_Button; }
        [[nodiscard]] inline i32   GetX() const noexcept { return m_MouseX; }
        [[nodiscard]] inline i32   GetY() const noexcept { return m_MouseY; }

        EVENT_CLASS_CATEGORY(EventCategoryMouse | EventCategoryInput)
    };

    class MouseMoveEvent : public MouseEvent
    {
    public:
        using MouseEvent::MouseEvent;

    public:
        [[nodiscard]] std::string ToString() const noexcept override
        {
            return fmt::format("MouseMoveEvent: Button: {}, Pos: ({}, {})", MouseToString(m_Button), m_MouseX,
                               m_MouseY);
        }

        EVENT_CLASS_TYPE(MouseMove)
        EVENT_CLASS_CATEGORY(EventCategoryMouse | EventCategoryInput)
    };

    class MouseScrollEvent : public MouseEvent
    {
    public:
        using MouseEvent::MouseEvent;

    private:
        i32 m_OffsetX;
        i32 m_OffsetY;

    public:
        MouseScrollEvent(const Mouse button, const i32 mouseX, const i32 mouseY, const i32 offsetX, const i32 offsetY)
            : MouseEvent(button, mouseX, mouseY)
            , m_OffsetX(offsetX)
            , m_OffsetY(offsetY)
        {
        }

    public:
        [[nodiscard]] inline i32 GetOffsetX() const noexcept { return m_OffsetX; }
        [[nodiscard]] inline i32 GetOffsetY() const noexcept { return m_OffsetY; }

    public:
        [[nodiscard]] std::string ToString() const noexcept override
        {
            return fmt::format("MouseScrollEvent: Offset: ({}, {})", m_OffsetX, m_OffsetY);
        }

        EVENT_CLASS_TYPE(MouseScroll)
        EVENT_CLASS_CATEGORY(EventCategoryMouse | EventCategoryInput)
    };

    class CODEX_API MouseDownEvent : public MouseEvent
    {
    public:
        using MouseEvent::MouseEvent;

    public:
        [[nodiscard]] std::string ToString() const noexcept override
        {
            return fmt::format("MouseDownEvent: Button({})", MouseToString(m_Button));
        }

        EVENT_CLASS_TYPE(MouseDown)
        EVENT_CLASS_CATEGORY(EventCategoryMouse | EventCategoryInput)
    };

    class CODEX_API MouseUpEvent : public MouseEvent
    {
    public:
        using MouseEvent::MouseEvent;

    public:
        [[nodiscard]] std::string ToString() const noexcept override
        {
            return fmt::format("MouseUpEvent: Button({})", MouseToString(m_Button));
        }

        EVENT_CLASS_TYPE(MouseUp)
        EVENT_CLASS_CATEGORY(EventCategoryMouse | EventCategoryInput)
    };
} // namespace codex::events

#endif // CODEX_EVENTS_MOUSE_EVENT_H
