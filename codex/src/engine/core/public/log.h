#pragma once

#include <filesystem>
#include <source_location>
#include <string_view>

#include <fmt/core.h>
#include <fmt/format.h>

#include <engine/core/public/common_def.h>

namespace codex {
    enum class LogLevel : u8
    {
        Info = 0,
        Warn,
        Error,
        Fatal,
        Debug,
        Verbose
    };

    struct TraceLocation
    {
        std::string_view     fmt;
        std::source_location loc = std::source_location::current();

        explicit(false) constexpr TraceLocation(
            const char* fmt, const std::source_location loc = std::source_location::current()) noexcept
            : fmt{ fmt }
            , loc{ loc }
        {
        }
    };

    namespace detail {
        CODEX_API void dispatch_log(LogLevel level, std::string_view prefix, std::string_view message) noexcept;
        CODEX_API void dispatch_log(LogLevel level, std::string_view message) noexcept;

        CODEX_API void init_logger();
        CODEX_API void dispose_logger();

        CODEX_API void add_log_sink(std::ostream* sink) noexcept;
        CODEX_API void remove_log_sink(std::ostream* sink) noexcept;
    } // namespace detail

    template <typename... TArgs>
    inline void log(const LogLevel level, const std::string_view fmt, TArgs&&... args)
    {
        detail::dispatch_log(level, fmt::format(fmt::runtime(fmt), std::forward<TArgs>(args)...));
    }
    template <typename... TArgs>
    inline void info(const std::string_view fmt, TArgs&&... args)
    {
        log(LogLevel::Info, fmt, std::forward<TArgs>(args)...);
    }
    template <typename... TArgs>
    inline void warn(const std::string_view fmt, TArgs&&... args)
    {
        log(LogLevel::Warn, fmt, std::forward<TArgs>(args)...);
    }
    template <typename... TArgs>
    inline void error(const std::string_view fmt, TArgs&&... args)
    {
        log(LogLevel::Error, fmt, std::forward<TArgs>(args)...);
    }
    template <typename... TArgs>
    inline void fatal(const std::string_view fmt, TArgs&&... args)
    {
        log(LogLevel::Fatal, fmt, std::forward<TArgs>(args)...);
    }
    template <typename... TArgs>
    inline void trace(TraceLocation fmt_spec, TArgs&&... args)
    {
        const auto& loc = fmt_spec.loc;
        const auto  msg = fmt::format(fmt::runtime("{} {}:{}: {}"),
                                      std::filesystem::path{ loc.file_name() }.filename().string(), loc.function_name(),
                                      loc.line(), fmt::format(fmt::runtime(fmt_spec.fmt), std::forward<TArgs>(args)...));
        detail::dispatch_log(LogLevel::Debug, msg);
    }

    template <FixedString Tag>
    class Loggable
    {
    protected:
        using enum LogLevel;

        template <typename... TArgs>
        void log(const LogLevel level, const std::string_view fmt, TArgs&&... args) const
        {
            detail::dispatch_log(level, std::string_view{ Tag },
                                 fmt::format(fmt::runtime(fmt), std::forward<TArgs>(args)...));
        }
        template <typename... TArgs>
        void info(const std::string_view fmt, TArgs&&... args) const
        {
            log(LogLevel::Info, fmt, std::forward<TArgs>(args)...);
        }
        template <typename... TArgs>
        void warn(const std::string_view fmt, TArgs&&... args) const
        {
            log(LogLevel::Warn, fmt, std::forward<TArgs>(args)...);
        }
        template <typename... TArgs>
        void error(const std::string_view fmt, TArgs&&... args) const
        {
            log(LogLevel::Error, fmt, std::forward<TArgs>(args)...);
        }
        template <typename... TArgs>
        void fatal(const std::string_view fmt, TArgs&&... args) const
        {
            log(LogLevel::Fatal, fmt, std::forward<TArgs>(args)...);
        }
        template <typename... TArgs>
        void trace(TraceLocation fmt_spec, TArgs&&... args) const
        {
            const auto& loc = fmt_spec.loc;
            const auto  msg = fmt::format(
                fmt::runtime("{} {}:{}: {}"), std::filesystem::path(loc.file_name()).filename().string(),
                loc.function_name(), loc.line(), fmt::format(fmt::runtime(fmt_spec.fmt), std::forward<TArgs>(args)...));
            detail::dispatch_log(LogLevel::Debug, std::string_view{ Tag }, msg);
        }
    };
} // namespace codex
