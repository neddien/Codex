#ifndef CODEX_CORE_COMMON_DEFINITIONS_H
#define CODEX_CORE_COMMON_DEFINITIONS_H

#include <cstdint>
#include <mutex>

#include <magic_enum.hpp>

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

namespace codex {
    using usize       = std::size_t;
    using intptr      = std::intptr_t;
    using uintptr     = std::uintptr_t;
    using i8          = std::int8_t;
    using i16         = std::int16_t;
    using i32         = std::int32_t;
    using i64         = std::int64_t;
    using u8          = std::uint8_t;
    using u16         = std::uint16_t;
    using u32         = std::uint32_t;
    using u64         = std::uint64_t;
    using f32         = float;
    using f64         = double;
    using f128        = long double;
    using object      = void*;
    using LockGuard   = std::lock_guard<std::mutex>;
    using ScopeGuard  = std::scoped_lock<std::mutex>;
    using UniqueGuard = std::unique_lock<std::mutex>;

    constexpr object nullobj = nullptr;

    template <typename Derived, typename Base>
    concept DerivedFrom = std::is_base_of<Base, Derived>::value;

    template <typename T>
    [[nodiscard]] constexpr T BitFlag(const T count) noexcept
    {
        return 1 << count;
    }

    [[nodiscard]] constexpr std::string_view EnumName(const auto val) noexcept
    {
        return magic_enum::enum_name(val);
    }

    template <typename Fn>
    constexpr auto BindEventDelegate(auto* self, Fn delegate)
    {
        return [self, delegate](auto&&... args) { return (self->*delegate)(std::forward<decltype(args)>(args)...); };
    }

    template <typename T, usize __Count>
    [[nodiscard]] constexpr auto ArrayLength(T (&arr)[__Count]) noexcept
    {
        return __Count;
    }

    struct InvalidState
    {
    };
} // namespace codex

#ifdef CX_CONFIG_DEBUG
#define MGL_DEBUG
#define CX_ASSERT(x, msg)                                                                                              \
    if (!(x))                                                                                                          \
    {                                                                                                                  \
        std::cerr << "[CODEX-DEBUG] :: Assertion failed: " << msg << "\n\tStack trace:"                                \
                  << "\n\t\tFunction: " << __FUNCTION__ << "\n\t\tFile: " << __FILE__ << "\n\t\tLine: " << __LINE__    \
                  << std::endl;                                                                                        \
        CX_DEBUG_TRAP();                                                                                               \
    }
#else
#define CX_ASSERT(x, msg) ;
#endif

#define CX_MACRO_STRINGFY(x) #x

#define CX_PROPERTY(name)                                                                                              \
    [[nodiscard]] inline auto Get##name() const noexcept                                                               \
    {                                                                                                                  \
        return m_##name;                                                                                               \
    }                                                                                                                  \
    inline void Set##name(decltype(m_##name) new##name) noexcept                                                       \
    {                                                                                                                  \
        m_##name = std::move(new##name);                                                                               \
    }

#define CX_PROPERTY_GET(name)                                                                                          \
    [[nodiscard]] inline auto Get##name() const noexcept                                                               \
    {                                                                                                                  \
        return m_##name;                                                                                               \
    }

#endif // CODEX_CORE_COMMON_DEFINITIONS_H
