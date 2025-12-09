#include "./Public/Profiler.h"

#include "./Public/TimeScope.h"

namespace codex::dbg {
    Profiler*                                  Profiler::s_Instance = nullptr;
    std::unordered_map<std::string, TimeScope> Profiler::s_ScopedProfilers;

    [[nodiscard]] TimeScope&& Profiler::GetProfile(const std::string& name) noexcept
    {
        return std::move(s_ScopedProfilers[name]);
    }

    [[nodiscard]] const std::unordered_map<std::string, TimeScope>& Profiler::GetProfilers() noexcept
    {
        return s_ScopedProfilers;
    }

    void Profiler::AddProfile(TimeScope&& timeScope) noexcept
    {
        s_ScopedProfilers[timeScope.m_Info.name] = std::move(timeScope);
    }
} // namespace codex::dbg
