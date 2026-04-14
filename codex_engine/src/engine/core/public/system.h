#pragma once

namespace codex {
    template <typename T>
    concept Subsystem = requires(T& s) {
        { T::get() } -> std::same_as<T&>; // singleton accessor
        { s.init() };
        { s.dispose() };
    };

    template <typename T>
    concept Tickable = Subsystem<T> && requires(T& s, const f32 dt) {
        { s.update(dt) };
    };

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

    template <Subsystem... Ss>
    class SystemManager
    {
    public:
        static void init_all() { (Ss::get().init(), ...); }

        // LIFO dispose, reverse init order
        static void dispose_all() { dispose_reverse(std::index_sequence_for<Ss...>{}); }

        static void tick_all(const f32 dt) { (tick_if_tickable<Ss>(dt), ...); }

    private:
        template <typename S>
        static void tick_if_tickable(const f32 dt)
        {
            if constexpr (Tickable<S>)
                S::get().update(dt);
        }

        template <usize... Is>
        static void dispose_reverse(std::index_sequence<Is...>)
        {
            using List = std::tuple<Ss...>;
            (std::tuple_element_t<sizeof...(Ss) - 1 - Is, List>::get().dispose(), ...);
        }
    };
} // namespace codex
