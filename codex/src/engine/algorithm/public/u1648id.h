#pragma once

namespace codex {
    struct u1648id
    {
        static constexpr u8  gen_bits   = 16;
        static constexpr u8  index_bits = 48;
        static constexpr u64 index_mask = (1uLL << index_bits) - 1;
        static constexpr u64 gen_mask   = ~index_mask;

        constexpr u1648id() noexcept = default;
        constexpr u1648id(u16 gen, u64 index) noexcept
            : packed_{ ((u64)gen << index_bits) | (index & index_mask) }
        {
        }

        [[nodiscard]] constexpr u64 index() const noexcept { return packed_ & index_mask; }
        [[nodiscard]] constexpr u16 gen() const noexcept { return (u16)(packed_ >> index_bits); }
        [[nodiscard]] constexpr u64 raw() const noexcept { return packed_; }

        constexpr void set_index(u64 _index) noexcept { packed_ = (packed_ & gen_mask) | (_index & index_mask); }
        constexpr void set_gen(u16 _gen) noexcept { packed_ = (packed_ & index_mask) | ((u64)_gen << index_bits); }

        constexpr void inc_index() noexcept { set_index(index() + 1); }
        constexpr void inc_gen() noexcept { set_gen((u16)(gen() + 1)); }

        [[nodiscard]] friend constexpr bool operator==(u1648id, u1648id) noexcept  = default;
        [[nodiscard]] friend constexpr auto operator<=>(u1648id, u1648id) noexcept = default;

        [[nodiscard]] static u1648id invalid_id() noexcept { return u1648id{ (u16)-1, (u64)-1 }; }

    private:
        u64 packed_ = 0;
    };
} // namespace codex

namespace std {
    template <>
    struct hash<codex::u1648id>
    {
        [[nodiscard]] size_t operator()(const codex::u1648id& id) const noexcept
        {
            /* clang-format: no inline */
            return id.raw();
        }
    };
} // namespace std
