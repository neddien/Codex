#ifndef CODEX_PCH_H
#define CODEX_PCH_H

// Turn this off.
// TODO: Disable compilers warnings in cmake.
#ifdef CX_COMPILER_MSVC
#pragma warning(disable : 4005)
#endif

// General
#include <algorithm>
#include <chrono>
#include <exception>
#include <filesystem>
#include <fstream>
#include <functional>
#include <iostream>
#include <memory>
#include <numeric>
#include <random>
#include <regex>
#include <sstream>
#include <type_traits>
#include <typeinfo>
#include <utility>
#include <variant>

// Concurrency
#include <atomic>
#include <coroutine>
#include <mutex>
#include <thread>

// Legacy
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <cstring>

// Collections
#include <array>
#include <bitset>
#include <flat_map>
#include <initializer_list>
#include <map>
#include <queue>
#include <string>
#include <unordered_map>
#include <unordered_set>
#include <vector>

// Reflection
#include <source_location>

// Platform specific
#if defined(CX_PLATFORM_UNIX)
#include <cxxabi.h>
#include <dlfcn.h>
#include <fcntl.h>
#include <pwd.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>

#elif defined(CX_PLATFORM_WINDOWS)
#ifndef NOMINMAX
#define NOMINMAX
#endif

#define WIN32_LEAN_AND_MEAN
#include <shl_obj.h>
#include <windows.h>
#if defined(CreateWindow)
#undef CreateWindow
#endif

#endif

// Define these
#define SDL_MAIN_HANDLED
#define IMGUI_DEFINE_MATH_OPERATORS

// Codex specific
#include <engine/core/public/common_def.h>
#include <engine/core/public/geometry.h>
#include <engine/math/public/math.h>
#include <engine/utils/public/util.h>

// Public Libraries
#include <Logger.h>
#include <engine/core/public/log.h>

#endif // CODEX_PCH_H
