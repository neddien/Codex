#pragma once

namespace codex {
    template <typename Derived>
    class System
    {
    public:
        [[nodiscard]] static Derived& get() noexcept
        {
            static Derived instance;
            return instance;
        }

    protected:
        inline System() noexcept         = default;
        inline ~System() noexcept        = default;
        System(const System&)            = delete;
        System& operator=(const System&) = delete;
    };
} // namespace codex
