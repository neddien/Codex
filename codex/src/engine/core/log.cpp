#include "public/log.h"

#include <Logger.h>
#include <fmt/color.h>

#include <engine/memory/public/memory.h>

namespace codex {
    static lgx::Logger* s_logger = nullptr;

    namespace {
        [[nodiscard]] constexpr lgx::Level to_lgx(const LogLevel level) noexcept
        {
            switch (level) {
                using enum LogLevel;

                case Info: return lgx::Level::Info;
                case Warn: return lgx::Level::Warn;
                case Error: return lgx::Level::Error;
                case Fatal: return lgx::Level::Fatal;
                case Debug: return lgx::Level::Debug;
                case Verbose: return lgx::Level::Verbose;
            }
            return lgx::Level::Info;
        }
    } // namespace

    void detail::dispatch_log(const LogLevel level, const std::string_view prefix,
                              const std::string_view message) noexcept
    {
        if (s_logger)
            s_logger->Log(std::string{ prefix }, to_lgx(level), "{}", message);
    }

    void detail::dispatch_log(const LogLevel level, const std::string_view message) noexcept
    {
        if (s_logger)
            s_logger->Log(to_lgx(level), "{}", message);
    }

    void detail::add_log_sink(std::ostream* sink) noexcept
    {
        if (s_logger) {
            auto streams = s_logger->GetOutputStreams();
            streams.push_back(sink);
            s_logger->SetOutputStreams(std::move(streams));
        }
    }

    void detail::remove_log_sink(std::ostream* sink) noexcept
    {
        if (s_logger) {
            auto streams = s_logger->GetOutputStreams();
            std::erase(streams, sink);
            s_logger->SetOutputStreams(std::move(streams));
        }
    }

    void detail::init_logger()
    {
        if (s_logger)
            return;

        lgx::Logger::Properties props;
        props.defaultPrefix       = "CodexEngine";
        props.defaultStyle.format = "[{textdate} {time}] [{level}] ({prefix}): {msg}";

        props.defaultStyle.infoLabel    = "info";
        props.defaultStyle.warnLabel    = "warn";
        props.defaultStyle.errorLabel   = "error";
        props.defaultStyle.fatalLabel   = "fatal";
        props.defaultStyle.debugLabel   = "debug";
        props.defaultStyle.verboseLabel = "verbose";

        props.defaultStyle.defaultInfoStyle = [](const lgx::StyleArgs& a)
        {
            return fmt::format("{} {} {} {}: {}", fmt::styled(a.textdate, fmt::fg(fmt::color::dark_gray)),
                               fmt::styled(a.time, fmt::fg(fmt::color::dark_gray)),
                               fmt::styled(a.prefix, fmt::fg(fmt::color::deep_sky_blue)), a.level,
                               fmt::styled(a.msg, fmt::fg(fmt::color::dark_gray)));
        };

        props.defaultStyle.defaultWarnStyle = [](const lgx::StyleArgs& a)
        {
            return fmt::format("{} {} {} {}: {}", fmt::styled(a.textdate, fmt::fg(fmt::color::dark_gray)),
                               fmt::styled(a.time, fmt::fg(fmt::color::dark_gray)),
                               fmt::styled(a.prefix, fmt::fg(fmt::color::deep_sky_blue)),
                               fmt::styled(a.level, fmt::fg(fmt::color::yellow)),
                               fmt::styled(a.msg, fmt::fg(fmt::color::yellow)));
        };

        props.defaultStyle.defaultErrorStyle = [](const lgx::StyleArgs& a)
        {
            return fmt::format("{} {} {} {}: {}", fmt::styled(a.textdate, fmt::fg(fmt::color::dark_gray)),
                               fmt::styled(a.time, fmt::fg(fmt::color::dark_gray)),
                               fmt::styled(a.prefix, fmt::fg(fmt::color::deep_sky_blue)),
                               fmt::styled(a.level, fmt::fg(fmt::color::red)),
                               fmt::styled(a.msg, fmt::fg(fmt::color::red)));
        };

        props.defaultStyle.defaultFatalStyle = [](const lgx::StyleArgs& a)
        {
            return fmt::format("{} {} {} {}: {}", fmt::styled(a.textdate, fmt::fg(fmt::color::dark_gray)),
                               fmt::styled(a.time, fmt::fg(fmt::color::dark_gray)),
                               fmt::styled(a.prefix, fmt::fg(fmt::color::deep_sky_blue)),
                               fmt::styled(a.level, fmt::fg(fmt::color::dark_red)),
                               fmt::styled(a.msg, fmt::fg(fmt::color::dark_red)));
        };

        props.defaultStyle.defaultDebugStyle = [](const lgx::StyleArgs& a)
        {
            return fmt::format("{} {} {} {}: {}", fmt::styled(a.textdate, fmt::fg(fmt::color::dark_gray)),
                               fmt::styled(a.time, fmt::fg(fmt::color::dark_gray)),
                               fmt::styled(a.prefix, fmt::fg(fmt::color::deep_sky_blue)),
                               fmt::styled(a.level, fmt::fg(fmt::color::light_green)),
                               fmt::styled(a.msg, fmt::fg(fmt::color::light_green)));
        };

        props.defaultStyle.defaultVerboseStyle = [](const lgx::StyleArgs& a)
        {
            return fmt::format("{} {} {} {}: {}", fmt::styled(a.textdate, fmt::fg(fmt::color::dark_gray)),
                               fmt::styled(a.time, fmt::fg(fmt::color::dark_gray)),
                               fmt::styled(a.prefix, fmt::fg(fmt::color::deep_sky_blue)),
                               fmt::styled(a.level, fmt::fg(fmt::color::gray)),
                               fmt::styled(a.msg, fmt::fg(fmt::color::gray)));
        };

        s_logger = new lgx::Logger{ std::move(props) };
    }

    void detail::dispose_logger()
    {
        if (s_logger) {
            delete s_logger;
            s_logger = nullptr;
        }
    }
} // namespace codex
