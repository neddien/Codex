#pragma once

#include <engine/core/public/common_def.h>
#include <fmt/core.h>

#undef None

#define EVENT_CLASS_TYPE(event_type_)                                                                                  \
    [[nodiscard]] static EventType static_type()                                                                       \
    {                                                                                                                  \
        return EventType::event_type_;                                                                                 \
    }                                                                                                                  \
    [[nodiscard]] EventType type() const noexcept override                                                             \
    {                                                                                                                  \
        return static_type();                                                                                          \
    }                                                                                                                  \
    [[nodiscard]] const char* name() const noexcept override                                                           \
    {                                                                                                                  \
        return #event_type_;                                                                                           \
    }

#define EVENT_CLASS_CATEGORY(cat_)                                                                                     \
    [[nodiscard]] u32 category() const noexcept override                                                               \
    {                                                                                                                  \
        return cat_;                                                                                                   \
    }

namespace codex::events {
    enum class EventType : u8
    {
        None = 0,

        // Window events.
        WindowClose,
        WindowResize,
        WindowFocus,
        WindowLostFocus,
        WindowMove,

        // Application events.
        AppTick,
        AppUpdate,
        AppRender,

        // Key events.
        KeyDown,
        KeyUp,

        // Mouse events.
        MouseDown,
        MouseUp,
        MouseMove,
        MouseScroll
    };

    enum EventCategory : u32
    {
        None                     = 0,
        EventCategoryApplication = bit(0),
        EventCategoryInput       = bit(1),
        EventCategoryKeyboard    = bit(2),
        EventCategoryMouse       = bit(3),
        EventCategoryMouseButton = bit(4)
    };

    class CODEX_API Event
    {
    public:
        virtual ~Event() = default;

    public:
        [[nodiscard]] virtual EventType   type() const noexcept     = 0;
        [[nodiscard]] virtual const char* name() const noexcept     = 0;
        [[nodiscard]] virtual u32         category() const noexcept = 0;
        [[nodiscard]] virtual std::string to_string() const noexcept { return name(); }

    public:
        [[nodiscard]] bool is_in_category(const EventCategory cat) const { return category() & cat; }

    public:
        bool handled = false;
    };

    class CODEX_API EventDispatcher
    {
    public:
        explicit EventDispatcher(Event& event)
            : event_(event)
        {
        }

    public:
        template <typename T, typename Fn>
            requires(std::is_base_of_v<Event, T>)
        bool dispatch(const Fn& delegate)
        {
            if (event_.type() == T::static_type()) {
                event_.handled |= delegate((T&)event_);
                return true;
            }
            return false;
        }

    private:
        Event& event_;
    };
} // namespace codex::events

namespace fmt {
    template <>
    struct formatter<codex::events::Event> : formatter<std::string_view>
    {
        auto format(const codex::events::Event& event, format_context& ctx) const
        {
            return fmt::format_to(ctx.out(), "(Name: {}, Type: {})", event.name(), codex::enum_name(event.type()));
        }
    };
} // namespace fmt
