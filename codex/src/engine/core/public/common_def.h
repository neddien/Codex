#ifndef CODEX_CORE_COMMON_DEFINITIONS_H
#define CODEX_CORE_COMMON_DEFINITIONS_H

// Use GLM experimental functions
#define GLM_ENABLE_EXPERIMENTAL

#include <cstdint>
#include <engine/core/public/geometry.h>

#if defined(CX_PLATFORM_WINDOWS)
#define NOINLINE     __declspec(noinline)
#define CODEX_EXPORT __declspec(dllexport)
#ifdef CX_BUILD_SHARED
#define CODEX_API __declspec(dllexport)
#elif defined(CX_BUILD_STATIC)
#define CODEX_API
#else
#define CODEX_API __declspec(dllimport)
#endif
#elif defined(CX_PLATFORM_UNIX)
#define NOINLINE     __attribute__((NOINLINE))
#define CODEX_API    __attribute__((visibility("default")))
#define CODEX_EXPORT __attribute__((visibility("default")))
#endif

#ifdef CX_COMPILER_GNUC
#define CX_DEBUG_TRAP()    __builtin_trap()
#define CX_PRETTY_FUNCTION __PRETTY_FUNCTION__
#define CX_PACKED(x)       x __attribute__((__packed__))
#elif CX_COMPILER_MSVC
#define CX_DEBUG_TRAP()    __debugbreak()
#define CX_PRETTY_FUNCTION __FUNCSIG__
#define CX_PACKED(x)       __pragma(pack(push, 1)) x __pragma(pack(pop))
#elif CX_COMPILER_CLANG
#define CX_DEBUG_TRAP()    __builtin_debugtrap()
#define CX_PRETTY_FUNCTION __PRETTY_FUNCTION__
#define CX_PACKED(x)       x __attribute__((__packed__))
#else
#error "Unknown compiler"
#endif

#ifdef CX_CONFIG_DEBUG
#define MGL_DEBUG

// Use cxensure() (in log.h) if you don't want your assert to collapse on Debug/Shipping builds
#define cxassert(x, fmt_str, ...)                                                                                      \
    do {                                                                                                               \
        if (!(x)) {                                                                                                    \
            ::codex::detail::dispatch_log(::codex::LogLevel::Fatal,                                                    \
                                          fmt::format(fmt::runtime("Assertion failed: {} ({}:{}, in {})"),             \
                                                      fmt::format(fmt::runtime(fmt_str) __VA_OPT__(, ) __VA_ARGS__),   \
                                                      __FILE__, __LINE__, __FUNCTION__));                              \
            CX_DEBUG_TRAP();                                                                                           \
        }                                                                                                              \
    } while (0)
#else
#define cxassert(x, ...) (void)(x)
#endif

#define CX_MACRO_STRINGFY(x) #x

#define CX_DEFAULT_LOGGER(name)                                                                                        \
    using __cx_intrinsics_logger = codex::Loggable<name>;                                                              \
    using __cx_intrinsics_logger::log;                                                                                 \
    using __cx_intrinsics_logger::info;                                                                                \
    using __cx_intrinsics_logger::warn;                                                                                \
    using __cx_intrinsics_logger::error;                                                                               \
    using __cx_intrinsics_logger::fatal;                                                                               \
    using __cx_intrinsics_logger::debug;                                                                               \
    using __cx_intrinsics_logger::trace;                                                                               \
    using __cx_intrinsics_logger::Info;                                                                                \
    using __cx_intrinsics_logger::Warn;                                                                                \
    using __cx_intrinsics_logger::Error;                                                                               \
    using __cx_intrinsics_logger::Fatal;                                                                               \
    using __cx_intrinsics_logger::Verbose;                                                                             \
    using __cx_intrinsics_logger::Debug;

namespace codex {
    using usize   = std::size_t;
    using intptr  = std::intptr_t;
    using uintptr = std::uintptr_t;
    using ptrdiff = std::ptrdiff_t;
    using i8      = std::int8_t;
    using i16     = std::int16_t;
    using i32     = std::int32_t;
    using i64     = std::int64_t;
    using u8      = std::uint8_t;
    using u16     = std::uint16_t;
    using u32     = std::uint32_t;
    using u64     = std::uint64_t;
    using f32     = float;
    using f64     = double;
    using f128    = long double;
    using object  = void*;

    template <typename T>
    using opt = std::optional<T>;
    template <typename T, typename U>
    using pair = std::pair<T, U>;

    constexpr object nullobj = nullptr;

    template <typename Derived, typename Base>
    concept DerivedFrom = std::is_base_of<Base, Derived>::value;
    template <typename T>
    concept StringLike = std::constructible_from<std::string_view, T> || std::constructible_from<std::wstring_view, T>;

    [[nodiscard]] constexpr auto bit(const auto nr) noexcept
    { return 1ul << nr; }

    [[nodiscard]] constexpr std::string_view enum_name(const auto val) noexcept
    { return magic_enum::enum_name(val); }

    template <typename T>
    [[nodiscard]] constexpr auto enum_from(const std::string_view str) noexcept
    { return magic_enum::enum_cast<T>(str); }

    template <typename T>
    [[nodiscard]] constexpr auto enum_cast(const auto val) noexcept
    { return magic_enum::enum_cast<T>(val); }

    template <typename Fn>
    constexpr auto bind_event_delegate(auto* self, Fn delegate)
    {
        return [self, delegate](auto&&... args) { return (self->*delegate)(std::forward<decltype(args)>(args)...); };
    }

    template <typename T, usize __Count>
    [[nodiscard]] constexpr auto array_length(T (&arr)[__Count]) noexcept
    { return __Count; }

    struct InvalidState
    {
    };

    template <usize N>
    struct FixedString
    {
        char data[N]{};
        constexpr FixedString(const char (&str)[N]) noexcept { std::copy_n(str, N, data); }
        constexpr operator std::string_view() const noexcept { return { data, N - 1 }; }
    };
} // namespace codex

namespace std {
    template <>
    struct hash<codex::math::vec2>
    {
        [[nodiscard]] std::size_t operator()(const codex::math::vec2& vec) const noexcept
        { return hash<codex::f32>()(vec.x) ^ hash<codex::f32>()(vec.y); }
    };

    template <>
    struct hash<codex::math::vec3>
    {
        [[nodiscard]] std::size_t operator()(const codex::math::vec3& vec) const noexcept
        { return hash<codex::f32>()(vec.x) ^ hash<codex::f32>()(vec.y) ^ hash<codex::f32>()(vec.z); }
    };

    template <>
    struct hash<codex::math::vec4>
    {
        [[nodiscard]] std::size_t operator()(const codex::math::vec4& vec) const noexcept
        {
            return hash<codex::f32>()(vec.x) ^ hash<codex::f32>()(vec.y) ^ hash<codex::f32>()(vec.z) ^
                   hash<codex::f32>()(vec.w);
        }
    };

    template <>
    struct hash<codex::math::rect>
    {
        [[nodiscard]] std::size_t operator()(const codex::math::rect& rect) const noexcept
        {
            return hash<codex::f32>()(rect.x) ^ hash<codex::f32>()(rect.y) ^ hash<codex::f32>()(rect.w) ^
                   hash<codex::f32>()(rect.h);
        }
    };
} // namespace std

namespace fmt {
    template <>
    struct formatter<codex::math::vec2> : formatter<std::string_view>
    {
        auto format(const codex::math::vec2& vec, format_context& ctx) const
        { return format_to(ctx.out(), "({}, {})", vec.x, vec.y); }
    };
    template <>
    struct formatter<codex::math::ivec2> : formatter<std::string_view>
    {
        auto format(const codex::math::ivec2& vec, format_context& ctx) const
        { return format_to(ctx.out(), "({}, {})", vec.x, vec.y); }
    };
    template <>
    struct formatter<codex::math::vec3> : formatter<std::string_view>
    {
        auto format(const codex::math::vec3& vec, format_context& ctx) const
        { return format_to(ctx.out(), "({}, {}, {})", vec.x, vec.y, vec.z); }
    };
    template <>
    struct formatter<codex::math::ivec3> : formatter<std::string_view>
    {
        auto format(const codex::math::ivec3& vec, format_context& ctx) const
        { return format_to(ctx.out(), "({}, {}, {})", vec.x, vec.y, vec.z); }
    };
    template <>
    struct formatter<codex::math::vec4> : formatter<std::string_view>
    {
        auto format(const codex::math::vec4& vec, format_context& ctx) const
        { return format_to(ctx.out(), "({}, {}, {}, {})", vec.x, vec.y, vec.z, vec.w); }
    };
    template <>
    struct formatter<codex::math::ivec4> : formatter<std::string_view>
    {
        auto format(const codex::math::ivec4& vec, format_context& ctx) const
        { return format_to(ctx.out(), "({}, {}, {}, {})", vec.x, vec.y, vec.z, vec.w); }
    };
    template <>
    struct formatter<codex::math::rect> : formatter<std::string_view>
    {
        auto format(const codex::math::rect& rect, format_context& ctx) const
        { return format_to(ctx.out(), "({}, {}, {}, {})", rect.x, rect.y, rect.w, rect.h); }
    };
    template <>
    struct formatter<codex::math::irect> : formatter<std::string_view>
    {
        auto format(const codex::math::irect& rect, format_context& ctx) const
        { return format_to(ctx.out(), "({}, {}, {}, {})", rect.x, rect.y, rect.w, rect.h); }
    };
} // namespace fmt

#endif // CODEX_CORE_COMMON_DEFINITIONS_H
