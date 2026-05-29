#pragma once

#include <sdafx.h>

#include <engine/core/public/common_def.h>
#include <engine/core/public/common_third_party_libs.h>
#include <engine/utils/public/util.h>

namespace codex {
    constexpr auto CX_DEFAULT_LOGGER_NAME      = "Codex";
    constexpr auto CX_DEFAULT_LOGGER_NAME_HASH = util::crypto::fnv1a(CX_DEFAULT_LOGGER_NAME);

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
        struct LogLevelStyle;

        [[nodiscard]] constexpr LogLevelStyle default_log_style_for_info() noexcept;
        [[nodiscard]] constexpr LogLevelStyle default_log_style_for_warn() noexcept;
        [[nodiscard]] constexpr LogLevelStyle default_log_style_for_error() noexcept;
        [[nodiscard]] constexpr LogLevelStyle default_log_style_for_fatal() noexcept;
        [[nodiscard]] constexpr LogLevelStyle default_log_style_for_debug() noexcept;
        [[nodiscard]] constexpr LogLevelStyle default_log_style_for_verbose() noexcept;

        struct LogLevelStyle
        {
            fmt::text_style date;
            fmt::text_style time;
            fmt::text_style tag;
            fmt::text_style level;
            fmt::text_style msg;
        };
        struct LogStyle
        {
            LogLevelStyle info    = default_log_style_for_info();
            LogLevelStyle warn    = default_log_style_for_warn();
            LogLevelStyle err     = default_log_style_for_error();
            LogLevelStyle fatal   = default_log_style_for_fatal();
            LogLevelStyle debug   = default_log_style_for_debug();
            LogLevelStyle verbose = default_log_style_for_verbose();
        };

        constexpr LogLevelStyle default_log_style_for_info() noexcept
        {
            return LogLevelStyle{
                .date  = fmt::fg(fmt::color::dark_gray),
                .time  = fmt::fg(fmt::color::dark_gray),
                .tag   = fmt::fg(fmt::color::deep_sky_blue),
                .level = fmt::fg(fmt::color::dark_gray),
                .msg   = fmt::fg(fmt::color::dark_gray),
            };
        }
        constexpr LogLevelStyle default_log_style_for_warn() noexcept
        {
            return LogLevelStyle{
                .date  = fmt::fg(fmt::color::dark_gray),
                .time  = fmt::fg(fmt::color::dark_gray),
                .tag   = fmt::fg(fmt::color::deep_sky_blue),
                .level = fmt::fg(fmt::color::yellow),
                .msg   = fmt::fg(fmt::color::yellow),
            };
        }
        constexpr LogLevelStyle default_log_style_for_error() noexcept
        {
            return LogLevelStyle{
                .date  = fmt::fg(fmt::color::dark_gray),
                .time  = fmt::fg(fmt::color::dark_gray),
                .tag   = fmt::fg(fmt::color::deep_sky_blue),
                .level = fmt::fg(fmt::color::red),
                .msg   = fmt::fg(fmt::color::red),
            };
        }
        constexpr LogLevelStyle default_log_style_for_fatal() noexcept
        {
            return LogLevelStyle{
                .date  = fmt::fg(fmt::color::dark_gray),
                .time  = fmt::fg(fmt::color::dark_gray),
                .tag   = fmt::fg(fmt::color::deep_sky_blue),
                .level = fmt::fg(fmt::color::dark_red),
                .msg   = fmt::fg(fmt::color::dark_red),
            };
        }
        constexpr LogLevelStyle default_log_style_for_debug() noexcept
        {
            return LogLevelStyle{
                .date  = fmt::fg(fmt::color::dark_gray),
                .time  = fmt::fg(fmt::color::dark_gray),
                .tag   = fmt::fg(fmt::color::deep_sky_blue),
                .level = fmt::fg(fmt::color::light_green),
                .msg   = fmt::fg(fmt::color::light_green),
            };
        }
        constexpr LogLevelStyle default_log_style_for_verbose() noexcept
        {
            return LogLevelStyle{
                .date  = fmt::fg(fmt::color::dark_gray),
                .time  = fmt::fg(fmt::color::dark_gray),
                .tag   = fmt::fg(fmt::color::deep_sky_blue),
                .level = fmt::fg(fmt::color::gray),
                .msg   = fmt::fg(fmt::color::gray),
            };
        }

        struct LogInitProperties
        {
            enum Sinks : u8
            {
                Stdout         = bit(0),
                StdoutNoColour = bit(1), // StdoutNoColour(1) overrides Stdout(0) if both are present
                RotatingFile   = bit(2),
                Syslog         = bit(3), // Ignored on non-Unix systems
            };
            enum QueuePolicy : u8
            {
                OverrunOld,
                OverrunNew,
                Block,
            };
            enum SyslogFacility : u8
            {
                LogDaemon,
                LogUser,
            };

        public:
            u8       sinks = Sinks::Stdout;
            LogStyle style{};
            u8       log_thread_count = 1;                                 // One thread for the async logger is enough
            u32      log_queue_size   = 8192;                              // 8192 max logs until the buffer is full
            u8       log_queue_policy = QueuePolicy::OverrunOld;           // Discard old messages when buffer is full
            std::filesystem::path rotating_file_path;                      // Required if RotatingFile(2) is set
            u32                   rotating_file_buf_len = 1024 * 1024 * 5; // 5Mb
            u8                    rotating_file_count   = 1;               // Just one file is enough
            std::string           syslog_ident          = "codex";         // Syslog identifier
            u8                    syslog_facility       = SyslogFacility::LogUser; // Syslog facility
        };

        CODEX_API void dispatch_log(LogLevel level, const usize tag, std::string_view message) noexcept;
        CODEX_API void dispatch_log(LogLevel level, std::string_view message) noexcept;

        CODEX_API void init_logger(const LogInitProperties properties);
        CODEX_API void dispose_logger();
        CODEX_API void register_logger(const std::string_view logger_name) noexcept;
    } // namespace detail

    template <typename... TArgs>
    constexpr void log(const LogLevel level, const std::string_view fmt, TArgs&&... args)
    {
        detail::dispatch_log(level, fmt::format(fmt::runtime(fmt), std::forward<TArgs>(args)...));
    }
    template <typename... TArgs>
    constexpr void info(const std::string_view fmt, TArgs&&... args)
    {
        log(LogLevel::Info, fmt, std::forward<TArgs>(args)...);
    }
    template <typename... TArgs>
    constexpr void warn(const std::string_view fmt, TArgs&&... args)
    {
        log(LogLevel::Warn, fmt, std::forward<TArgs>(args)...);
    }
    template <typename... TArgs>
    constexpr void error(const std::string_view fmt, TArgs&&... args)
    {
        log(LogLevel::Error, fmt, std::forward<TArgs>(args)...);
    }
    template <typename... TArgs>
    constexpr void fatal(const std::string_view fmt, TArgs&&... args)
    {
        log(LogLevel::Fatal, fmt, std::forward<TArgs>(args)...);
    }
    template <typename... TArgs>
    constexpr void trace(TraceLocation fmt_spec, TArgs&&... args)
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
        constexpr Loggable() noexcept { detail::register_logger(Tag); }

    protected:
        using enum LogLevel;

        template <typename... TArgs>
        constexpr void log(const LogLevel level, const std::string_view fmt, TArgs&&... args) const noexcept
        {
            detail::dispatch_log(level, util::crypto::fnv1a(Tag),
                                 fmt::format(fmt::runtime(fmt), std::forward<TArgs>(args)...));
        }
        template <typename... TArgs>
        constexpr void info(const std::string_view fmt, TArgs&&... args) const noexcept
        {
            log(LogLevel::Info, fmt, std::forward<TArgs>(args)...);
        }
        template <typename... TArgs>
        constexpr void warn(const std::string_view fmt, TArgs&&... args) const noexcept
        {
            log(LogLevel::Warn, fmt, std::forward<TArgs>(args)...);
        }
        template <typename... TArgs>
        constexpr void error(const std::string_view fmt, TArgs&&... args) const noexcept
        {
            log(LogLevel::Error, fmt, std::forward<TArgs>(args)...);
        }
        template <typename... TArgs>
        constexpr void fatal(const std::string_view fmt, TArgs&&... args) const noexcept
        {
            log(LogLevel::Fatal, fmt, std::forward<TArgs>(args)...);
        }
        template <typename... TArgs>
        constexpr void trace(const TraceLocation fmt_spec, TArgs&&... args) const noexcept
        {
            const auto& loc = fmt_spec.loc;
            const auto  msg = fmt::format(
                fmt::runtime("{} {}:{}: {}"), std::filesystem::path(loc.file_name()).filename().string(),
                loc.function_name(), loc.line(), fmt::format(fmt::runtime(fmt_spec.fmt), std::forward<TArgs>(args)...));
            detail::dispatch_log(LogLevel::Debug, util::crypto::fnv1a(Tag), msg);
        }
    };
} // namespace codex
