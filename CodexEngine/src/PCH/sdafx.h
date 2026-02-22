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
#include <string>
#include <unordered_map>
#include <vector>

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
#include <ShlObj.h>
#include <windows.h>
#if defined(CreateWindow)
#undef CreateWindow
#endif

#endif

// Define these
#define SDL_MAIN_HANDLED
#define IMGUI_DEFINE_MATH_OPERATORS

// Codex specific
#include <Engine/Core/Public/CommonDef.h>
#include <Engine/Core/Public/Geomtryd.h>
#include <Engine/Math/Public/Math.h>
#include <Engine/Utils/Public/Util.h>

// Library specific
/*
#include <Logger.h>
#include <fmt/args.h>
#include <fmt/chrono.h>
#include <fmt/color.h>
#include <fmt/core.h>
#include <fmt/format.h>
#include <glm/ext/matrix_clip_space.hpp> // glm::perspective
#include <glm/ext/matrix_transform.hpp>  // glm::translate, glm::rotate, glm::scale
#include <glm/ext/scalar_constants.hpp>  // glm::pi
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp> // glm::value_ptr
#include <glm/gtx/matrix_decompose.hpp>
#include <glm/gtx/rotate_vector.hpp> // glm::rotate(glm::vecX)
#include <glm/mat4x4.hpp>            // glm::mat4
#include <glm/vec2.hpp>
#include <glm/vec3.hpp> // glm::vec3
#include <glm/vec4.hpp> // glm::vec4
// #include <NetNative.h>
#include <box2d/box2d.h>
#include <entt.hpp>
#include <imgui.h>
#include <imgui_internal.h>
#include <misc/cpp/imgui_stdlib.h>
#include <nlohmann/json.hpp>
#include <nlohmann/json_fwd.hpp>
#include <nlohmann/ordered_map.hpp>
*/

// Public Libraries
#include <Logger.h>

#endif // CODEX_PCH_H
