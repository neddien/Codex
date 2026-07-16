cmake_minimum_required(VERSION 3.16)

project("Codex")

include(FetchContent)

# ==================================================== # Export macros that
# describe the current platform   #
# ==================================================== #
if(CMAKE_SYSTEM_NAME STREQUAL "Linux")
  set(CX_PLATFORM_LINUX
      1
      CACHE INTERNAL "")
  set(CX_PLATFORM_UNIX
      1
      CACHE INTERNAL "")
elseif(CMAKE_SYSTEM_NAME STREQUAL "Darwin")
  set(CX_PLATFORM_OSX
      1
      CACHE INTERNAL "")
  set(CX_PLATFORM_UNIX
      1
      CACHE INTERNAL "")
elseif(CMAKE_SYSTEM_NAME STREQUAL "Windows")
  set(CX_PLATFORM_WINDOWS
      1
      CACHE INTERNAL "")
  set(CX_APP_DATA_PATH
      "${CMAKE_INSTALL_PREFIX}"
      CACHE INTERNAL "")
else()
  set(CX_PLATFORM_UNKNOWN 1)
endif()

if(CMAKE_CXX_COMPILER_ID STREQUAL "GNU")
  set(CX_COMPILER_GNUC
      1
      CACHE INTERNAL "")
elseif(CMAKE_CXX_COMPILER_ID STREQUAL "Clang" OR CMAKE_CXX_COMPILER_ID STREQUAL
                                                 "AppleClang")
  set(CX_COMPILER_CLANG
      1
      CACHE INTERNAL "")
elseif(CMAKE_CXX_COMPILER_ID STREQUAL "MSVC")
  set(CX_COMPILER_MSVC
      1
      CACHE INTERNAL "")
else()
  set(CX_COMPILER_UNKNOWN
      1
      CACHE INTERNAL "")
endif()

# GCC's UBSan miscompiles Abseil's SwissTable constexpr policy-function checks
# (abseil-cpp#1634 / GCC PR71962: comparing a hash-slot function pointer to
# nullptr in a constexpr context fails to fold under GCC+UBSan). Clang is
# unaffected, so only strip it for GCC; Clang builds keep full ASan+UBSan.
if(CX_COMPILER_GNUC)
  string(REPLACE "-fsanitize=undefined" "" CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS}")
  string(REPLACE "-fsanitize=undefined" "" CMAKE_C_FLAGS "${CMAKE_C_FLAGS}")
  set(CMAKE_CXX_FLAGS
      "${CMAKE_CXX_FLAGS}"
      CACHE STRING "" FORCE)
  set(CMAKE_C_FLAGS
      "${CMAKE_C_FLAGS}"
      CACHE STRING "" FORCE)
endif()

# Curated strict warning sets for Release/Shipping, shared by codex/ and
# editor/. Deliberately excludes -Wconversion/-Wsign-conversion (GCC/Clang)
# and their MSVC equivalents (/w44244 etc.) since those are the noisiest
# against existing code and warrant their own dedicated cleanup pass rather
# than riding along here.
set(CX_STRICT_WARNINGS_GNU
    -Wnon-virtual-dtor
    # -Wstringop-overflow (part of baseline -Wall) has known false positives on null-checks
    # hidden behind opaque-ish wrapper calls (e.g. Ref<T>::lock() + cxensure()) that GCC's
    # analysis can't fully correlate with a later dereference. Keep it visible as a warning,
    # just don't fail the build on it.
    -Wno-error=stringop-overflow
    -Wcast-align
    -Woverloaded-virtual
    -Wdouble-promotion
    -Wformat=2
    -Wimplicit-fallthrough
    -Wmisleading-indentation
    -Wduplicated-cond
    -Wduplicated-branches
    -Wlogical-op
    -Wuseless-cast
    CACHE INTERNAL "")
set(CX_STRICT_WARNINGS_CLANG
    -Wnon-virtual-dtor
    -Wcast-align
    -Woverloaded-virtual
    -Wdouble-promotion
    -Wformat=2
    -Wimplicit-fallthrough
    -Wmisleading-indentation
    CACHE INTERNAL "")
set(CX_STRICT_WARNINGS_MSVC
    /permissive-
    /w14242
    /w14254
    /w14263
    /w14265
    /w14287
    /we4289
    /w14296
    /w14311
    /w14545
    /w14546
    /w14547
    /w14549
    /w14555
    /w14619
    /w14640
    /w14826
    /w14905
    /w14906
    /w14928
    CACHE INTERNAL "")

# Determine the processor architecture
if(CMAKE_SIZEOF_VOID_P EQUAL 8)
  set(CX_WORD_SIZE_64
      1
      CACHE INTERNAL "")
else()
  set(CX_WORD_SIZE_32
      1
      CACHE INTERNAL "")
endif()
if(CMAKE_SYSTEM_PROCESSOR STREQUAL "x86_64")
  set(CX_CPU_ARCH "x86_64")
elseif(CMAKE_SYSTEM_PROCESSOR STREQUAL "i386")
  set(CX_CPU_ARCH "x86")
elseif(CMAKE_SYSTEM_PROCESSOR STREQUAL "aarch64")
  set(CX_CPU_ARCH "arm64")
endif()

if(CMAKE_BUILD_TYPE STREQUAL "Debug")
  set(CX_BUILD_TYPE_DEBUG
      1
      CACHE INTERNAL "")
elseif(CMAKE_BUILD_TYPE STREQUAL "Release")
  set(CX_BUILD_TYPE_RELEASE
      1
      CACHE INTERNAL "")
elseif(CMAKE_BUILD_TYPE STREQUAL "Shipping")
  set(CX_BUILD_TYPE_SHIPPING
      1
      CACHE INTERNAL "")
endif()

set(CX_ROOT_DIR ${CMAKE_SOURCE_DIR})

# Export compile_commands.json for the LSPs.
set(CMAKE_EXPORT_COMPILE_COMMANDS on)

message(STATUS "${PROJECT_NAME}: Preset: ${CMAKE_PRESET_NAME}")

# If we're not on VS, then copy compile_commands.json for LSPs like clangd and
# ccls.
message(STATUS "${PROJECT_NAME}: Generator: " ${CMAKE_GENERATOR})
if(NOT CMAKE_GENERATOR MATCHES "Visual Studio")
  add_custom_command(
    OUTPUT ${CMAKE_SOURCE_DIR}/compile_commands.json
    COMMAND ${CMAKE_COMMAND} -E copy ${CMAKE_BINARY_DIR}/compile_commands.json
            ${CMAKE_SOURCE_DIR}/compile_commands.json
    DEPENDS ${CMAKE_BINARY_DIR}/compile_commands.json
    COMMENT "Copying compile_commands.json...")

  add_custom_target(copy_compile_commands ALL
                    DEPENDS ${CMAKE_SOURCE_DIR}/compile_commands.json)
else()

endif()

# Add sub projects.
add_subdirectory("codex")
add_subdirectory("codex-shipped")

if(DEFINED CX_ENABLE_NETNT)
  add_subdirectory("NetNT")
endif()
if(DEFINED CX_ENABLE_TESTWORK)
  add_subdirectory("test_work")
endif()
if(DEFINED CX_ENABLE_CEDITOR)
  add_subdirectory("editor")
endif()
