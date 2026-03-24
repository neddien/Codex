#pragma once

#include "event.h"

namespace codex::events {
    class CODEX_API WindowResizeEvent : public Event
    {
    public:
        WindowResizeEvent(const i32 width, const i32 height)
            : width_(width)
            , height_(height)
        {
        }

    public:
        [[nodiscard]] inline i32         width() const noexcept { return width_; }
        [[nodiscard]] inline i32         height() const noexcept { return height_; }
        [[nodiscard]] inline std::string to_string() const noexcept override
        {
            return fmt::format("WindowResizeEvent: Width({}), Height({})", width_, height_);
        }

        EVENT_CLASS_TYPE(WindowResize)
        EVENT_CLASS_CATEGORY(EventCategoryApplication)

    private:
        i32 width_;
        i32 height_;
    };

    class CODEX_API WindowCloseEvent : public Event
    {
    public:
        WindowCloseEvent() = default;

        EVENT_CLASS_TYPE(WindowClose)
        EVENT_CLASS_CATEGORY(EventCategoryApplication)
    };
} // namespace codex::events
