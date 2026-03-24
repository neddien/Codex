#include "public/log.h"

#include <engine/memory/public/memory.h>

namespace codex {
    static mem::Box<lgx::Logger> s_engine_logger;
    static mem::Box<lgx::Logger> s_editor_logger;
    static mem::Box<lgx::Logger> s_nbman_logger;
    static lgx::Logger*          s_active_logger = nullptr;

    auto detail::engine_logger() noexcept -> lgx::Logger&
    {
        return *s_engine_logger;
    }
    auto detail::editor_logger() noexcept -> lgx::Logger&
    {
        return *s_editor_logger;
    }
    auto detail::nbman_logger() noexcept -> lgx::Logger&
    {
        return *s_nbman_logger;
    }
    auto detail::active_logger() noexcept -> lgx::Logger&
    {
        return *s_active_logger;
    }

    void detail::set_active_logger(lgx::Logger& logger) noexcept
    {
        s_active_logger = &logger;
    }

    void detail::init_loggers()
    {
        constexpr auto k_fmt = "[{level}] ({prefix}): {msg}";

        s_engine_logger = mem::Box<lgx::Logger>::make(lgx::Logger::Properties{
            .defaultPrefix = "engine",
            .defaultStyle  = { .format            = k_fmt,
                               .defaultInfoStyle  = fmt::fg(fmt::color::light_sky_blue),
                               .defaultWarnStyle  = fmt::fg(fmt::color::yellow),
                               .defaultErrorStyle = fmt::fg(fmt::color::red) | fmt::emphasis::italic,
                               .defaultFatalStyle = fmt::fg(fmt::color::dark_red) | fmt::emphasis::italic,
                               .defaultDebugStyle = fmt::fg(fmt::color::light_green) } });

        s_editor_logger = mem::Box<lgx::Logger>::make(lgx::Logger::Properties{
            .defaultPrefix = "editor",
            .defaultStyle  = { .format            = k_fmt,
                               .defaultInfoStyle  = fmt::fg(fmt::color::lime_green),
                               .defaultWarnStyle  = fmt::fg(fmt::color::yellow),
                               .defaultErrorStyle = fmt::fg(fmt::color::red) | fmt::emphasis::italic,
                               .defaultFatalStyle = fmt::fg(fmt::color::dark_red) | fmt::emphasis::italic,
                               .defaultDebugStyle = fmt::fg(fmt::color::light_green) } });

        s_nbman_logger = mem::Box<lgx::Logger>::make(lgx::Logger::Properties{
            .defaultPrefix = "behaviour",
            .defaultStyle  = { .format            = k_fmt,
                               .defaultInfoStyle  = fmt::fg(fmt::color::plum),
                               .defaultWarnStyle  = fmt::fg(fmt::color::yellow),
                               .defaultErrorStyle = fmt::fg(fmt::color::red) | fmt::emphasis::italic,
                               .defaultFatalStyle = fmt::fg(fmt::color::dark_red) | fmt::emphasis::italic,
                               .defaultDebugStyle = fmt::fg(fmt::color::light_green) } });

        s_active_logger = s_engine_logger.get();
    }
} // namespace codex
