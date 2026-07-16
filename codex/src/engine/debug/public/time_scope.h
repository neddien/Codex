#pragma once

#include "profiler.h"

#ifdef CX_CONFIG_DEBUG
#define CX_DEBUG_PROFILE_SCOPE(...)                                                                                    \
    const auto __cx_intrinsics_current_scope_profiler = codex::dbg::profile_scope(__VA_ARGS__);
#elif defined(CX_ENSURE_PROFILER)
#define CX_DEBUG_PROFILE_SCOPE(...)                                                                                    \
    const auto __cx_intrinsics_current_scope_profiler = codex::dbg::profile_scope(__VA_ARGS__);
#else
#define CX_DEBUG_PROFILE_SCOPE(...)
#endif

namespace codex::dbg {
    template <typename T>
    concept IsChronoPeriod = requires { typename T::period; };

    template <typename T>
    concept IsChronoClock = requires {
        { T::now() } -> std::convertible_to<std::chrono::time_point<T>>;
    };

    class TimeScope
    {
        friend class Profiler;

    public:
        using ratio        = std::nano;
        using rep          = f64;
        using chrono_clock = std::chrono::steady_clock;

    public:
        inline TimeScope() noexcept = default;
        inline explicit TimeScope(ProfileInfo info) noexcept
            : info_{ std::move(info) }
            , initiated_{ true }
            , initial_tp_{ chrono_clock::now() }
        {
        }
        inline TimeScope(TimeScope&& other) noexcept            = default;
        inline TimeScope& operator=(TimeScope&& other) noexcept = default;
        inline ~TimeScope() noexcept
        {
            if (initiated_) {
                stop();
                Profiler::add_profile(std::move(*this));
            }
        }

    public:
        [[nodiscard]] ProfileInfo info() const noexcept { return info_; }
        template <typename ToRatio, typename ToRep>
        [[nodiscard]] auto elapsed_as() const noexcept
        {
            const_cast<TimeScope*>(this)->stop();
            return std::chrono::duration_cast<std::chrono::duration<ToRep, ToRatio>>(duration_);
        }
        [[nodiscard]] auto elapsed() const noexcept
        {
            const_cast<TimeScope*>(this)->stop();
            return duration_;
        }

    private:
        void stop() noexcept
        {
            if (initiated_) {
                initiated_ = false;
                duration_  = std::chrono::duration<rep, ratio>(chrono_clock::now() - initial_tp_);
            }
        }

    private:
        ProfileInfo                               info_;
        mutable bool                              initiated_ = false;
        chrono_clock::time_point                  initial_tp_;
        mutable std::chrono::duration<rep, ratio> duration_;
    };

    [[nodiscard]] inline TimeScope profile_scope(
        std::string_view name = {}, const std::source_location loc = std::source_location::current()) noexcept
    {
        if (name.empty())
            name = loc.function_name();

        return TimeScope{ ProfileInfo{
            .name = std::string{ name },
            .file = loc.file_name(),
            .func = loc.function_name(),
            .line = loc.line(),
        } };
    }
} // namespace codex::dbg
