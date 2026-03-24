#pragma once

#include "profiler.h"

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
        using Ratio       = std::nano;
        using Rep         = f64;
        using ChronoClock = std::chrono::steady_clock;

    public:
        inline TimeScope() noexcept = default;
        inline explicit TimeScope(ProfileInfo info) noexcept
            : info_(std::move(info))
            , initiated_(true)
            , initial_tp_(ChronoClock::now())
        {
        }
        inline TimeScope(TimeScope&& other) noexcept            = default;
        inline TimeScope& operator=(TimeScope&& other) noexcept = default;
        inline ~TimeScope() noexcept
        {
            if (initiated_) {
                initiated_ = false;
                duration_  = std::chrono::duration<Rep, Ratio>(ChronoClock::now() - initial_tp_);
                Profiler::add_profile(std::move(*this));
            }
        }

    public:
        [[nodiscard]] ProfileInfo info() const noexcept { return info_; }
        template <typename ToRatio, typename ToRep>
        [[nodiscard]] auto elapsed_as() const noexcept
        {
            return std::chrono::duration_cast<std::chrono::duration<ToRep, ToRatio>>(duration_);
        }
        [[nodiscard]] auto elapsed() const noexcept { return duration_; }

    private:
        ProfileInfo                       info_;
        bool                              initiated_ = false;
        ChronoClock::time_point           initial_tp_;
        std::chrono::duration<Rep, Ratio> duration_;
    };
} // namespace codex::dbg
