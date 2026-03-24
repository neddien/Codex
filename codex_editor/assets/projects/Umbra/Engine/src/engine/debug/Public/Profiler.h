#pragma once

#define CX_DEBUG_PROFILE_SCOPE(scope_name)                                                                             \
    const auto __profile_scope = codex::dbg::TimeScope(codex::dbg::ProfileInfo{                                        \
        .name_ = scope_name,                                                                                           \
        .file_ = __FILE__,                                                                                             \
        .func_ = CX_PRETTY_FUNCTION,                                                                                   \
        .line_ = __LINE__,                                                                                             \
    });

namespace codex::dbg {
    // Forward declarations.
    class TimeScope;

    struct ProfileInfo
    {
        std::string      name_;
        std::string_view file_;
        std::string_view func_;
        u32              line_;
    };

    class CODEX_API Profiler
    {
    private:
        static Profiler*                                  s_instance_;
        static std::unordered_map<std::string, TimeScope> s_scoped_profilers_;

    public:
        [[nodiscard]] static TimeScope&& get_profile(const std::string& name) noexcept;
        [[nodiscard]] static const std::unordered_map<std::string, TimeScope>& get_profilers() noexcept;
        static void add_profile(TimeScope&& time_scope) noexcept;
    };
} // namespace codex::dbg
