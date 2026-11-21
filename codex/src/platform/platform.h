#pragma once

// Platform Macros
#if defined(_WIN32)
#    define CX_PLATFORM_WINDOWS
#    define CX_NOINLINE __declspec(noinline)
#    define CX_EXPORT   __declspec(dllexport)
#    if defined(CX_BUILD_SHARED)
#        define CX_API __declspec(dllexport)
#    elif defined(CX_BUILD_STATIC)
#        define CX_API
#    else
#        define CX_API __declspec(dllimport)
#    endif
#elif defined(__linux__)
#    define CX_PLATFORM_LINUX
#    define CX_PLATFORM_UNIX
#elif defined(__APPLE__ || __ANDROID__) // Generic Unix-like systems
#    define CX_PLATFORM_OSX
#    define CX_PLATFORM_UNIX
#endif
#if defined(CX_PLATFORM_UNIX)
#    define CX_NOINLINE __attribute__((NOINLINE))
#    define CX_API      __attribute__((visibility("default")))
#    define CX_EXPORT   __attribute__((visibility("default")))
#endif

// Compiler Macros
#if defined(__GNUC__)
#    define CX_COMPILER_GNU
#    define CX_DEBUG_TRAP()    __builtin_trap()
#    define CX_PRETTY_FUNCTION __PRETTY_FUNCTION__
#    define CX_PACKED(x)       x __attribute__((__packed__))
#elif defined(_MSC_VER)
#    define CX_COMPILER_MSVC
#    define CX_DEBUG_TRAP()    __debugbreak()
#    define CX_PRETTY_FUNCTION __FUNCSIG__
#    define CX_PACKED(x)       __pragma(pack(push, 1)) x __pragma(pack(pop))
#elif defined(__clang__)
#    define CX_COMPILER_CLANG
#    define CX_DEBUG_TRAP()    __builtin_debugtrap()
#    define CX_PRETTY_FUNCTION __PRETTY_FUNCTION__
#    define CX_PACKED(x)       x __attribute__((__packed__))
#else
#    error "Unknown compiler."
#endif
