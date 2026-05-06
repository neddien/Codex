#include "public/profiler.h"

#include "public/time_scope.h"

namespace codex::dbg {
    Profiler*                                  Profiler::s_instance_ = nullptr;
    std::unordered_map<std::string, TimeScope> Profiler::s_scoped_profilers_;

    [[nodiscard]] TimeScope&& Profiler::get_profile(const std::string& name) noexcept
    {
        return std::move(s_scoped_profilers_[name]);
    }

    [[nodiscard]] const std::unordered_map<std::string, TimeScope>& Profiler::get_profilers() noexcept
    {
        return s_scoped_profilers_;
    }

    void Profiler::add_profile(TimeScope&& time_scope) noexcept
    {
        s_scoped_profilers_[time_scope.info_.name] = std::move(time_scope);
    }
} // namespace codex::dbg
