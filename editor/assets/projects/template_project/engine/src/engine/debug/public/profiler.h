#pragma once

namespace codex::dbg {
    // Forward declarations.
    class TimeScope;

    struct ProfileInfo
    {
        std::string      name;
        std::string_view file;
        std::string_view func;
        u32              line;
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
