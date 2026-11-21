project("CodexEngine" LANGUAGES C CXX)

# ============= # Version # ============= #
set(CX_VER_MAJ 0)
set(CX_VER_MIN 1)
set(CX_VER_REV 0)

# Enable fPIC on all linkable targets
set(CMAKE_POSITION_INDEPENDENT_CODE on)

# SDL2, box2d, nlohmann_json, lz4, imgui come from Conan (see conanfile.py)
find_package(SDL2 REQUIRED)
find_package(box2d REQUIRED)
find_package(nlohmann_json REQUIRED)
find_package(LZ4 REQUIRED)
find_package(imgui REQUIRED)
find_package(fmt REQUIRED)
find_package(spdlog REQUIRED)
find_package(EnTT REQUIRED)
find_package(magic_enum REQUIRED)
find_package(glm REQUIRED)
find_package(stb REQUIRED)
find_package(absl REQUIRED)
find_package(OpenGL REQUIRED)
find_package(Threads REQUIRED)

# Fetch all the source and header files and the then add them automatically
file(GLOB_RECURSE CX_ENGINE_SOURCES CONFIGURE_DEPENDS "src/engine/*.cpp" "src/platform/open_gl/*.cpp")
if(WIN32)
  file(GLOB_RECURSE CX_PLATFORM_SOURCES CONFIGURE_DEPENDS "src/platform/windows/*.cpp")
elseif(APPLE)
  file(GLOB_RECURSE CX_PLATFORM_SOURCES CONFIGURE_DEPENDS "src/platform/osx/*.cpp" "src/platform/posix/*.cpp")
else()
  file(GLOB_RECURSE CX_PLATFORM_SOURCES CONFIGURE_DEPENDS "src/platform/linux/*.cpp" "src/platform/posix/*.cpp")
endif()

set(IMGUI_BACKENDS_DIR "${CMAKE_BINARY_DIR}/conan/imgui_backends")
set(CX_THIRDPARTY_SOURCES
  "vendor/glad/src/glad.cpp"
  "${IMGUI_BACKENDS_DIR}/imgui_impl_opengl3.cpp"
  "${IMGUI_BACKENDS_DIR}/imgui_impl_sdl2.cpp"
  "${IMGUI_BACKENDS_DIR}/imgui_stdlib.cpp")

# Exclude warnings on third-party sources but per file
set_source_files_properties(${CX_THIRDPARTY_SOURCES} PROPERTIES COMPILE_OPTIONS "$<IF:$<BOOL:${MSVC}>,/w,-w>")

# GCC 15+ fails to compile stb_image's SSE2 SIMD path due to stricter
# immediate-constant enforcement in _mm_slli_si128/_mm_srli_si128.
if(CMAKE_CXX_COMPILER_ID STREQUAL "GNU")
  set_source_files_properties("src/platform/open_gl/stb_image.cpp" PROPERTIES COMPILE_DEFINITIONS STBI_NO_SIMD)
endif()

function(cx_add_engine name type)
  add_library(${name} ${type} ${CX_ENGINE_SOURCES} ${CX_PLATFORM_SOURCES} ${CX_THIRDPARTY_SOURCES})

  # Include directories
  target_include_directories(${name}
    PUBLIC
      "include/"
      "src/"
      "src/pch"
      "${CMAKE_CURRENT_BINARY_DIR}/generated"
    PRIVATE
      "src/engine"
      "src/platform")

  target_include_directories(${name} SYSTEM PUBLIC "vendor/glad/include" "${IMGUI_BACKENDS_DIR}")

  # For MSVC disable CRT secure warnings
  target_compile_definitions(${name}
    PUBLIC  $<$<PLATFORM_ID:Windows>:_CRT_SECURE_NO_WARNINGS>
    PRIVATE CX_COMPILER_BIN="${CMAKE_CXX_COMPILER}")

  # Link libraries
  target_link_libraries(${name}
    PUBLIC
      codex_build_config # Link build config interface, defines the appropriate flags for the CMake, Conan & the Compiler
      SDL2::SDL2
      SDL2::SDL2main
      imgui::imgui
      fmt::fmt
      spdlog::spdlog
      nlohmann_json::nlohmann_json
      glm::glm
      EnTT::EnTT
      magic_enum::magic_enum
      LZ4::lz4_static
      abseil::abseil
    PRIVATE
      codex_warnings # Link strict warnings interface for Release/Shipping builds
      stb::stb box2d::box2d
      fmod::core
      fmod::studio
      OpenGL::GL
      Threads::Threads
      # Fix a dumb fucking linker bug on Ubuntu regarding dlopen
      ${CMAKE_DL_LIBS})

  # Include PCH
  target_precompile_headers(${name} PUBLIC "src/pch/sdafx.h")

  # Version bump
  add_dependencies(${name} CodexVersionBump)

  # Set CXX standard to 20
  set_property(TARGET ${name} PROPERTY CXX_STANDARD 20)

  # Export appropriate build config macros to the compiler.
  target_compile_definitions(${name} PUBLIC
    $<$<CONFIG:Debug>:CX_BUILD_TYPE_DEBUG>
    $<$<CONFIG:Release>:CX_BUILD_TYPE_RELEASE>
    $<$<CONFIG:Shipping>:CX_BUILD_TYPE_SHIPPING>)

  # Multicore compilation for MSVC if CX_PARALLEL is not 0.
  if(CMAKE_CXX_COMPILER_FRONTEND_VARIANT STREQUAL "MSVC" AND NOT CX_PARALLEL EQUAL 0)
    #target_compile_options(${name} PRIVATE "/MP")
  endif()

  # Export assets dir variable and ImGUI backends variable as well.
  set_target_properties(${name} PROPERTIES
    CX_ASSET_DIR            "${CMAKE_CURRENT_SOURCE_DIR}/assets"
    CX_IMGUI_BACKENDS_DIR   "${IMGUI_BACKENDS_DIR}")
endfunction()

cx_add_engine(CodexEngine SHARED)
target_compile_definitions(CodexEngine PRIVATE CX_BUILD_SHARED)

cx_add_engine(CodexEngine-static STATIC)
target_compile_definitions(CodexEngine-static PUBLIC CX_BUILD_STATIC)
# Enable PIC on CodexEngine-static, this is needed if the static lib is going to be linked to
# a shared one, for now that is not the case but having this on doesn't hurt either way.
# Shared libraries have this on by default.
set_target_properties(CodexEngine-static PROPERTIES POSITION_INDEPENDENT_CODE ON)

# Print the platform information
message(STATUS "${PROJECT_NAME}: Platform OS: ${CMAKE_SYSTEM_NAME}")
message(STATUS "${PROJECT_NAME}: Platform Compiler: ${CMAKE_CXX_COMPILER_ID}")
message(STATUS "${PROJECT_NAME}: CPU word size: ${CMAKE_SIZEOF_VOID_P} bytes")
message(STATUS "${PROJECT_NAME}: Graphics API: OpenGL")

# FMOD Studio API 2.03.15 (L builds for Debug)
set(FMOD_MAJOR_VER 203)
set(FMOD_MINOR_VER 15)

# Set the appropriate platform
if(LINUX)
  set(FMOD_PLATFORM "linux")
elseif(WIN32)
  set(FMOD_PLATFORM "win")
endif()

# Set the appropriate architecture
if(CX_ARCH MATCHES "^(x86_64|amd64)$")
    set(FMOD_ARCH "x64")
elseif(CX_ARCH MATCHES "^(aarch64|arm64)$")
    set(FMOD_ARCH "arm64")
endif()

set(FMOD_DIR "${CMAKE_CURRENT_SOURCE_DIR}/vendor/fmod/fmodstudioapi${FMOD_MAJOR_VER}${FMOD_MINOR_VER}${FMOD_PLATFORM}")
function(cx_import_fmod tgt subdir base)
  set(dir "${FMOD_DIR}/api/${subdir}/lib/${FMOD_ARCH}")
  add_library(${tgt} SHARED IMPORTED GLOBAL)
  set_target_properties(${tgt} PROPERTIES IMPORTED_CONFIGURATIONS "DEBUG;RELEASE" INTERFACE_INCLUDE_DIRECTORIES "${FMOD_DIR}/api/${subdir}/inc")
  # Not sure what this does, but its SYSTEM automatically? Whatever that means
  # I love cmake
  if(WIN32)
    set_target_properties(${tgt} PROPERTIES
      IMPORTED_LOCATION_DEBUG   "${dir}/${base}L.dll"  IMPORTED_IMPLIB_DEBUG   "${dir}/${base}L_vc.lib"
      IMPORTED_LOCATION_RELEASE "${dir}/${base}.dll"   IMPORTED_IMPLIB_RELEASE "${dir}/${base}_vc.lib")
  else()
    set_target_properties(${tgt} PROPERTIES
      IMPORTED_LOCATION_DEBUG   "${dir}/lib${base}L.so.14.${FMOD_MINOR_VER}" IMPORTED_SONAME_DEBUG   "lib${base}L.so.14"
      IMPORTED_LOCATION_RELEASE "${dir}/lib${base}.so.14.${FMOD_MINOR_VER}"  IMPORTED_SONAME_RELEASE "lib${base}.so.14")
  endif()
endfunction()
cx_import_fmod(fmod::core core fmod)
cx_import_fmod(fmod::studio studio fmodstudio)

# ======================= # INSTALLATION # ======================= #
install(TARGETS CodexEngine RUNTIME DESTINATION "${CX_INSTALL_BIN}" LIBRARY DESTINATION "${CX_INSTALL_LIB}")
install(IMPORTED_RUNTIME_ARTIFACTS fmt::fmt SDL2::SDL2 fmod::core fmod::studio
        RUNTIME DESTINATION "${CX_INSTALL_BIN}" LIBRARY DESTINATION "${CX_INSTALL_LIB}")

  # /etc/appname for global configuration files. /usr/local/share/appname for
  # unmodifiable data. /usr/local/(lib, bin)/appname for unmodifiable and
  # machine dependent data. /var/local/(lib, bin)/appname for modifiable data.

# ============= # Version header # ============= #
set(CX_VERSION_HEADER_OUT
    "${CMAKE_CURRENT_BINARY_DIR}/generated/engine/core/public/version.h")

# Version bump target
add_custom_target(
  CodexVersionBump
  COMMAND
    ${CMAKE_COMMAND}
    -D "BUILD_COUNT_FILE=${CMAKE_SOURCE_DIR}/build_count.txt"
    -D "VERSION_HEADER_OUT=${CX_VERSION_HEADER_OUT}"
    -D "CX_VER_MAJ=${CX_VER_MAJ}"
    -D "CX_VER_MIN=${CX_VER_MIN}"
    -D "CX_VER_REV=${CX_VER_REV}"
    -P "${CMAKE_SOURCE_DIR}/cmake/increment_build_count.cmake"
  COMMENT "Incrementing build counter..."
  VERBATIM)

# Bootstrap: create the header at configure time if it doesn't exist yet so
# that LSPs and the first compilation have something to include.
if(NOT EXISTS "${CX_VERSION_HEADER_OUT}")
  if(EXISTS "${CMAKE_SOURCE_DIR}/build_count.txt")
    file(READ "${CMAKE_SOURCE_DIR}/build_count.txt" _cx_build_count)
    string(STRIP "${_cx_build_count}" _cx_build_count)
  else()
    set(_cx_build_count 0)
  endif()
  file(MAKE_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}/generated/engine/core/public")
  file(WRITE "${CX_VERSION_HEADER_OUT}"
"#pragma once

#define CX_VER_MAJ     ${CX_VER_MAJ}
#define CX_VER_MIN     ${CX_VER_MIN}
#define CX_VER_REV     ${CX_VER_REV}
#define CX_BUILD_COUNT ${_cx_build_count}

#define CX_VERSION_STRING      \"${CX_VER_MAJ}.${CX_VER_MIN}.${CX_VER_REV}\"
#define CX_FULL_VERSION_STRING \"${CX_VER_MAJ}.${CX_VER_MIN}.${CX_VER_REV}+${_cx_build_count}\"
")
endif()
