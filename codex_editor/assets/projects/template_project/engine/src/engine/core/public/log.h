#pragma once

#include <filesystem>
#include <source_location>
#include <string_view>

#include <Logger.h>

#include <engine/core/public/common_def.h>

namespace codex {
    // Helper to capture call-site source_location for trace().
    // Because variadic templates cannot be followed by defaulted parameters,
    // wrapping the format string in this struct lets the capture happen at the
    // call site automatically:
    //   Engine::trace("hello {}", value)   <- loc captured here, not inside.
    struct TraceLocation
    {
        std::string_view     fmt;
        std::source_location loc;

        TraceLocation(const std::string_view     fmt,
                      const std::source_location loc = std::source_location::current()) noexcept
            : fmt(fmt)
            , loc(loc)
        {
        }
    };

    namespace detail {
        // Thin accessors to the three engine-managed logger instances.
        // Initialised by Engine::internal_init() via detail::init_loggers().
        [[nodiscard]] CODEX_API auto engine_logger() noexcept -> lgx::Logger&;
        [[nodiscard]] CODEX_API auto editor_logger() noexcept -> lgx::Logger&;
        [[nodiscard]] CODEX_API auto nbman_logger() noexcept -> lgx::Logger&;

        // Returns the currently-active logger (engine by default; switch with
        // Engine::use_editor_logger() / use_nbman_logger()).
        [[nodiscard]] CODEX_API auto active_logger() noexcept -> lgx::Logger&;
        CODEX_API void               set_active_logger(lgx::Logger& logger) noexcept;

        // Called once from Engine::internal_init().
        CODEX_API void init_loggers();
    } // namespace detail

    // -------------------------------------------------------------------------
    // Convenience free functions. Route to whichever logger is currently active.
    // Usable in any engine source file without pulling in the heavy Engine class.
    // -------------------------------------------------------------------------
    template <typename... TArgs>
    inline void log(const lgx::Level level, const std::string_view fmt, TArgs&&... args)
    {
        detail::active_logger().Log(level, fmt, std::forward<TArgs>(args)...);
    }
    template <typename... TArgs>
    inline void info(const std::string_view fmt, TArgs&&... args)
    {
        detail::active_logger().Info(fmt, std::forward<TArgs>(args)...);
    }
    template <typename... TArgs>
    inline void warn(const std::string_view fmt, TArgs&&... args)
    {
        detail::active_logger().Warn(fmt, std::forward<TArgs>(args)...);
    }
    template <typename... TArgs>
    inline void error(const std::string_view fmt, TArgs&&... args)
    {
        detail::active_logger().Error(fmt, std::forward<TArgs>(args)...);
    }
    template <typename... TArgs>
    inline void fatal(const std::string_view fmt, TArgs&&... args)
    {
        detail::active_logger().Fatal(fmt, std::forward<TArgs>(args)...);
    }
    template <typename... TArgs>
    inline void trace(TraceLocation fmt_spec, TArgs&&... args)
    {
        const auto& loc = fmt_spec.loc;
        const auto  msg = fmt::format(fmt::runtime("{} {}:{}: {}"),
                                      std::filesystem::path(loc.file_name()).filename().string(), loc.function_name(),
                                      loc.line(), fmt::format(fmt::runtime(fmt_spec.fmt), std::forward<TArgs>(args)...));
        detail::active_logger().Log(lgx::Level::Debug, msg);
    }
} // namespace codex
