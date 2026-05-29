#include "public/log.h"

#include <engine/memory/public/memory.h>

#include <spdlog/async.h>
#include <spdlog/sinks/rotating_file_sink.h>
#include <spdlog/sinks/stdout_color_sinks.h>
#include <spdlog/sinks/stdout_sinks.h>
#include <spdlog/sinks/syslog_sink.h>
#include <spdlog/spdlog.h>

namespace codex {
    static std::atomic<bool>                                           s_initialized{ false };
    static std::vector<spdlog::sink_ptr>                               s_spd_sinks{};
    static std::flat_map<usize, std::shared_ptr<spdlog::async_logger>> s_tag_loggers{};
    static std::mutex                                                  s_tag_loggers_mutex{};
    static std::flat_map<usize, std::string_view>                      s_pending_logger_regs{};

    namespace {
        [[nodiscard]] constexpr spdlog::level::level_enum to_spd_level(const LogLevel level) noexcept
        {
            switch (level) {
                using enum LogLevel;
                case Info: return spdlog::level::level_enum::info;
                case Warn: return spdlog::level::level_enum::warn;
                case Error: return spdlog::level::level_enum::err;
                case Fatal: return spdlog::level::level_enum::critical;
                case Debug: return spdlog::level::level_enum::debug;
                case Verbose: return spdlog::level::level_enum::trace;
            }
            return spdlog::level::level_enum::info;
        }

        [[nodiscard]] constexpr spdlog::async_overflow_policy to_spd_policy(const u8 policy) noexcept
        {
            switch (policy) {
                case detail::LogInitProperties::OverrunOld: return spdlog::async_overflow_policy::overrun_oldest;
                case detail::LogInitProperties::OverrunNew: return spdlog::async_overflow_policy::discard_new;
                case detail::LogInitProperties::Block: return spdlog::async_overflow_policy::block;
            }
            return spdlog::async_overflow_policy::block;
        }

#ifdef CX_PLATFORM_UNIX
        [[nodiscard]] constexpr u32 to_syslog_facility(const u8 facility) noexcept
        {
            switch (facility) {
                case detail::LogInitProperties::LogDaemon: return LOG_DAEMON;
                case detail::LogInitProperties::LogUser: return LOG_USER;
            }
            return LOG_USER;
        }
#endif
    } // namespace

    template <typename Mutex>
    class codex_stdout_color_sink final : public spdlog::sinks::base_sink<Mutex>
    {
    public:
        codex_stdout_color_sink(detail::LogStyle style) noexcept
            : style_{ std::move(style) }
        {
        }

    protected:
        void sink_it_(const spdlog::details::log_msg& msg) override
        {
            const time_t time_val = std::chrono::system_clock::to_time_t(msg.time);
            std::tm      tm_val{};
            ::localtime_r(&time_val, &tm_val);

            char date_buf[16];
            std::strftime(date_buf, sizeof(date_buf), "%b %d", &tm_val);

            const std::string_view tag{ msg.logger_name.data(), msg.logger_name.size() };
            const std::string_view payload{ msg.payload.data(), msg.payload.size() };

            std::string line;
            switch (msg.level) {
                default:
                case spdlog::level::info: {
                    line = fmt::format(
                        "{} {} {} {}: {}\n", fmt::styled(std::string_view{ date_buf }, style_.info.date),
                        fmt::styled(fmt::format("{:02d}:{:02d}:{:02d}", tm_val.tm_hour, tm_val.tm_min, tm_val.tm_sec),
                                    style_.info.time),
                        fmt::styled(tag, style_.info.tag), fmt::styled("info", style_.info.level),
                        fmt::styled(payload, style_.info.msg));
                } break;
                case spdlog::level::warn: {
                    line = fmt::format(
                        "{} {} {} {}: {}\n", fmt::styled(std::string_view{ date_buf }, style_.warn.date),
                        fmt::styled(fmt::format("{:02d}:{:02d}:{:02d}", tm_val.tm_hour, tm_val.tm_min, tm_val.tm_sec),
                                    style_.warn.time),
                        fmt::styled(tag, style_.warn.tag), fmt::styled("warn", style_.warn.level),
                        fmt::styled(payload, style_.warn.msg));
                } break;
                case spdlog::level::err: {
                    line = fmt::format(
                        "{} {} {} {}: {}\n", fmt::styled(std::string_view{ date_buf }, style_.err.date),
                        fmt::styled(fmt::format("{:02d}:{:02d}:{:02d}", tm_val.tm_hour, tm_val.tm_min, tm_val.tm_sec),
                                    style_.err.time),
                        fmt::styled(tag, style_.err.tag), fmt::styled("error", style_.err.level),
                        fmt::styled(payload, style_.err.msg));
                } break;
                case spdlog::level::critical: {
                    line = fmt::format(
                        "{} {} {} {}: {}\n", fmt::styled(std::string_view{ date_buf }, style_.fatal.date),
                        fmt::styled(fmt::format("{:02d}:{:02d}:{:02d}", tm_val.tm_hour, tm_val.tm_min, tm_val.tm_sec),
                                    style_.fatal.time),
                        fmt::styled(tag, style_.fatal.tag), fmt::styled("fatal", style_.fatal.level),
                        fmt::styled(payload, style_.fatal.msg));
                } break;
                case spdlog::level::debug: {
                    line = fmt::format(
                        "{} {} {} {}: {}\n", fmt::styled(std::string_view{ date_buf }, style_.debug.date),
                        fmt::styled(fmt::format("{:02d}:{:02d}:{:02d}", tm_val.tm_hour, tm_val.tm_min, tm_val.tm_sec),
                                    style_.debug.time),
                        fmt::styled(tag, style_.debug.tag), fmt::styled("debug", style_.debug.level),
                        fmt::styled(payload, style_.debug.msg));
                } break;
                case spdlog::level::trace: {
                    line = fmt::format(
                        "{} {} {} {}: {}\n", fmt::styled(std::string_view{ date_buf }, style_.verbose.date),
                        fmt::styled(fmt::format("{:02d}:{:02d}:{:02d}", tm_val.tm_hour, tm_val.tm_min, tm_val.tm_sec),
                                    style_.verbose.time),
                        fmt::styled(tag, style_.verbose.tag), fmt::styled("verbose", style_.verbose.level),
                        fmt::styled(payload, style_.verbose.msg));
                } break;
            }

            fwrite(line.data(), 1, line.size(), stdout);
        }

        void flush_() override { fflush(stdout); }

    private:
        detail::LogStyle style_;
    };

    using codex_stdout_color_sink_mt = codex_stdout_color_sink<std::mutex>;
    using codex_stdout_color_sink_st = codex_stdout_color_sink<spdlog::details::null_mutex>;

    void detail::dispatch_log(const LogLevel level, const usize tag, const std::string_view message) noexcept
    {
        if (s_initialized.load()) {
            std::scoped_lock guard{ s_tag_loggers_mutex };

            if (auto it = s_tag_loggers.find(tag); it != s_tag_loggers.end())
                it->second->log(to_spd_level(level), message);
        }
    }

    void detail::dispatch_log(const LogLevel level, const std::string_view message) noexcept
    {
        if (s_initialized.load()) {
            std::scoped_lock guard{ s_tag_loggers_mutex };
            s_tag_loggers.at(CX_DEFAULT_LOGGER_NAME_HASH)->log(to_spd_level(level), message);
        }
    }

    void detail::init_logger(const detail::LogInitProperties properties)
    {
        if (s_initialized.load())
            return;

        spdlog::init_thread_pool(properties.log_queue_size, properties.log_thread_count);

        if (properties.sinks & LogInitProperties::Stdout && !(properties.sinks & LogInitProperties::StdoutNoColour)) {
            auto stdout_sink = std::make_shared<codex_stdout_color_sink_mt>(properties.style);
            s_spd_sinks.push_back(stdout_sink);
        }

        if (properties.sinks & LogInitProperties::StdoutNoColour) {
            auto stdout_no_colour_sink = std::make_shared<spdlog::sinks::stdout_sink_mt>();
            s_spd_sinks.push_back(stdout_no_colour_sink);
        }

        if (properties.sinks & LogInitProperties::RotatingFile) {
            if (!properties.rotating_file_path.empty()) {
                auto rotating_file_sink = std::make_shared<spdlog::sinks::rotating_file_sink_mt>(
                    properties.rotating_file_path.generic_string(), properties.rotating_file_buf_len,
                    properties.rotating_file_count);
                s_spd_sinks.push_back(rotating_file_sink);
            }
        }

#ifdef CX_PLATFORM_UNIX
        if (properties.sinks & LogInitProperties::Syslog) {
            auto syslog_sink = std::make_shared<spdlog::sinks::syslog_sink_mt>(
                properties.syslog_ident, LOG_PID, to_syslog_facility(properties.syslog_facility), false);
            s_spd_sinks.push_back(syslog_sink);
        }
#endif

        auto default_logger =
            std::make_shared<spdlog::async_logger>(CX_DEFAULT_LOGGER_NAME, s_spd_sinks.begin(), s_spd_sinks.end(),
                                                   spdlog::thread_pool(), to_spd_policy(properties.log_queue_policy));
        spdlog::register_logger(default_logger);

        {
            std::scoped_lock guard{ s_tag_loggers_mutex };
            auto [_, did_insert] =
                s_tag_loggers.try_emplace(util::crypto::fnv1a(CX_DEFAULT_LOGGER_NAME), std::move(default_logger));
            assert(did_insert);

            // If we have any deferred loggers that we need to register
            for (auto it = s_pending_logger_regs.begin(); it != s_pending_logger_regs.end(); ++it) {
                const auto& [hash, name] = *it;

                auto logger =
                    std::make_shared<spdlog::async_logger>(std::string{ name }, s_spd_sinks.begin(), s_spd_sinks.end(),
                                                           spdlog::thread_pool(), spdlog::async_overflow_policy::block);
                auto [_, did_insert] = s_tag_loggers.try_emplace(hash, logger);
                assert(did_insert);
            }

            s_initialized.store(true);
        }
    }

    void detail::dispose_logger()
    {
        std::scoped_lock guard{ s_tag_loggers_mutex };
        s_tag_loggers.clear();
        s_spd_sinks.clear();
    }

    void detail::register_logger(const std::string_view logger_name) noexcept
    {
        std::scoped_lock guard{ s_tag_loggers_mutex };

        const usize hash = util::crypto::fnv1a(logger_name);
        if (s_initialized.load()) {
            if (auto it = s_tag_loggers.find(hash); it == s_tag_loggers.end()) {
                auto logger = std::make_shared<spdlog::async_logger>(std::string{ logger_name }, s_spd_sinks.begin(),
                                                                     s_spd_sinks.end(), spdlog::thread_pool(),
                                                                     spdlog::async_overflow_policy::block);
                auto [_, did_insert] = s_tag_loggers.try_emplace(hash, logger);
                assert(did_insert);
            }
        } else {
            // Defer it to when this is finished loading.
            if (auto it = s_pending_logger_regs.find(hash); it == s_pending_logger_regs.end()) {
                auto [_, did_insert] = s_pending_logger_regs.try_emplace(hash, logger_name);
                assert(did_insert);
            }
        }
    }
} // namespace codex
