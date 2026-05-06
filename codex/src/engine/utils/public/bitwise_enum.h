#pragma once

namespace codex {
    // BitmaskEnum concept. Opt-in by calling CX_ENABLE_BITWISE_ENUM(MyEnum) in
    // the same namespace as the enum — the concept finds the sentinel via ADL.
    template <typename E>
    concept BitmaskEnum = std::is_enum_v<E> && requires(E e) { enable_bitwise_enum_for(e); };

    // Returned by bitwise operators. Supports contextual bool conversion (if/&&/||)
    // and implicitly converts back to E so assignments and chaining stay transparent.
    template <BitmaskEnum E>
    struct bit_result
    {
        E value;

        constexpr operator bool() const noexcept
        {
            return static_cast<std::underlying_type_t<E>>(value) != 0;
        }
        constexpr operator E() const noexcept { return value; }
        constexpr bool operator!() const noexcept { return !static_cast<bool>(*this); }
    };

    // Extracts E from either E directly or bit_result<E>. Used to make binary
    // operators accept any mix of E and bit_result<E> without separate overloads.
    template <typename T>
    struct enum_type_of : std::type_identity<T> {};
    template <BitmaskEnum E>
    struct enum_type_of<bit_result<E>> : std::type_identity<E> {};
    template <typename T>
    using enum_type_of_t = typename enum_type_of<T>::type;

    template <typename T>
    concept BitOperand = BitmaskEnum<enum_type_of_t<T>>;

    template <BitmaskEnum E>
    [[nodiscard]] constexpr bool to_bool(const E e) noexcept
    {
        return static_cast<std::underlying_type_t<E>>(e) != 0;
    }

    template <BitmaskEnum E>
    [[nodiscard]] constexpr bool to_bool(const bit_result<E> r) noexcept
    {
        return static_cast<bool>(r);
    }
} // namespace codex

// Binary operators accept any combination of E and bit_result<E> as operands,
// e.g. (E & E), (E & bit_result<E>), (bit_result<E> & E), (bit_result<E> & bit_result<E>).
template <codex::BitOperand L, codex::BitOperand R>
    requires std::same_as<codex::enum_type_of_t<L>, codex::enum_type_of_t<R>>
[[nodiscard]] constexpr auto operator|(const L lhs, const R rhs) noexcept
    -> codex::bit_result<codex::enum_type_of_t<L>>
{
    using E = codex::enum_type_of_t<L>;
    using U = std::underlying_type_t<E>;
    return { static_cast<E>(static_cast<U>(static_cast<E>(lhs)) | static_cast<U>(static_cast<E>(rhs))) };
}

template <codex::BitOperand L, codex::BitOperand R>
    requires std::same_as<codex::enum_type_of_t<L>, codex::enum_type_of_t<R>>
[[nodiscard]] constexpr auto operator&(const L lhs, const R rhs) noexcept
    -> codex::bit_result<codex::enum_type_of_t<L>>
{
    using E = codex::enum_type_of_t<L>;
    using U = std::underlying_type_t<E>;
    return { static_cast<E>(static_cast<U>(static_cast<E>(lhs)) & static_cast<U>(static_cast<E>(rhs))) };
}

template <codex::BitOperand L, codex::BitOperand R>
    requires std::same_as<codex::enum_type_of_t<L>, codex::enum_type_of_t<R>>
[[nodiscard]] constexpr auto operator^(const L lhs, const R rhs) noexcept
    -> codex::bit_result<codex::enum_type_of_t<L>>
{
    using E = codex::enum_type_of_t<L>;
    using U = std::underlying_type_t<E>;
    return { static_cast<E>(static_cast<U>(static_cast<E>(lhs)) ^ static_cast<U>(static_cast<E>(rhs))) };
}

template <codex::BitOperand T>
[[nodiscard]] constexpr codex::bit_result<codex::enum_type_of_t<T>> operator~(const T operand) noexcept
{
    using E = codex::enum_type_of_t<T>;
    return { static_cast<E>(~static_cast<std::underlying_type_t<E>>(static_cast<E>(operand))) };
}

// Assignment operators: lhs is always E&; rhs can be E or bit_result<E>.
template <codex::BitmaskEnum E>
constexpr E& operator|=(E& lhs, const E rhs) noexcept
{
    return lhs = lhs | rhs;
}

template <codex::BitmaskEnum E>
constexpr E& operator|=(E& lhs, const codex::bit_result<E> rhs) noexcept
{
    return lhs = lhs | rhs;
}

template <codex::BitmaskEnum E>
constexpr E& operator&=(E& lhs, const E rhs) noexcept
{
    return lhs = lhs & rhs;
}

template <codex::BitmaskEnum E>
constexpr E& operator&=(E& lhs, const codex::bit_result<E> rhs) noexcept
{
    return lhs = lhs & rhs;
}

template <codex::BitmaskEnum E>
constexpr E& operator^=(E& lhs, const E rhs) noexcept
{
    return lhs = lhs ^ rhs;
}

template <codex::BitmaskEnum E>
constexpr E& operator^=(E& lhs, const codex::bit_result<E> rhs) noexcept
{
    return lhs = lhs ^ rhs;
}

template <codex::BitmaskEnum E>
[[nodiscard]] constexpr bool operator!(const E e) noexcept
{
    return static_cast<std::underlying_type_t<E>>(e) == 0;
}

// Place immediately after the enum definition, in the same namespace.
// ADL will find this sentinel when evaluating the BitmaskEnum concept.
#define CX_ENABLE_BITWISE_ENUM(E) inline void enable_bitwise_enum_for(E) noexcept {}
