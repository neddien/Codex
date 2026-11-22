cmake_minimum_required(VERSION 3.24)
project("Codex" LANGUAGES C CXX)

include(FetchContent)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin")

# Enable CMakeConfigDeps in Conan, an experimental feature.
list(APPEND CONAN_HOST_PROFILE "${CMAKE_SOURCE_DIR}/cmake/conan-codex-profile")

# This code here detects the current MSVC runtime and writes it to a Conan Host Profile file
# for Conan, so I don't have to hardcode the MSVC runtime version thus locking the whole engine to
# exclusively VS2022 (because I personally use VS2022) or VS2026 (which I think is a downgrade from VS2022).
if(CMAKE_CXX_COMPILER_ID STREQUAL "Clang" AND CMAKE_CXX_SIMULATE_ID STREQUAL "MSVC")
  string(SUBSTRING "${MSVC_VERSION}" 2 1 cx_msvc_minor)
  set(cx_conan_toolset_profile "${CMAKE_BINARY_DIR}/conan_clangcl_toolset")
  file(WRITE "${cx_conan_toolset_profile}" "[settings]\ncompiler.runtime_version=v14${cx_msvc_minor}\n")
  list(APPEND CONAN_HOST_PROFILE "${cx_conan_toolset_profile}")
endif()

# Linux only
# I did not know this but apparently ELF can hard-code the DLL lookup path and this is called RPATH.
# Here we set RPATH to point to ../lib since Debug and Release installations always install like so:
# ./bin/<executable>
# ./lib/<engine & shared libs>
set(CMAKE_INSTALL_RPATH "$ORIGIN/../lib")
set(CX_INSTALL_ROOT "$<$<NOT:$<CONFIG:Shipping>>:$<LOWER_CASE:$<CONFIG>>/>")
set(CX_INSTALL_BIN "${CX_INSTALL_ROOT}bin")
set(CX_INSTALL_LIB "${CX_INSTALL_ROOT}lib")
set(CX_INSTALL_DATA "${CX_INSTALL_ROOT}data")

# Shipping inherits Release's compile options
foreach(lang C CXX)
    set(CMAKE_${lang}_FLAGS_SHIPPING "${CMAKE_${lang}_FLAGS_RELEASE}" CACHE STRING "")
endforeach()
foreach(kind EXE SHARED MODULE)
    set(CMAKE_${kind}_LINKER_FLAGS_SHIPPING "${CMAKE_${kind}_LINKER_FLAGS_RELEASE}" CACHE STRING "")
endforeach()
set(CMAKE_MAP_IMPORTED_CONFIG_SHIPPING Release "") # Must come before find_package() calls, tells find_package that Shipping == Release

add_library(codex_build_config INTERFACE)
target_compile_features(codex_build_config INTERFACE cxx_std_20)
target_compile_definitions(codex_build_config INTERFACE
  $<$<CONFIG:Debug>:CX_BUILD_TYPE_DEBUG>
  $<$<CONFIG:Release>:CX_BUILD_TYPE_RELEASE>
  $<$<CONFIG:Shipping>:CX_BUILD_TYPE_SHIPPING>)

# GCC's UBSan miscompiles Abseil's SwissTable constexpr policy-function checks
# (abseil-cpp#1634 / GCC PR71962: comparing a hash-slot function pointer to
# nullptr in a constexpr context fails to fold under GCC+UBSan). Clang is
# unaffected, so only strip it for GCC; Clang builds keep full ASan+UBSan.
if(CMAKE_CXX_COMPILER_ID STREQUAL "GNU")
  string(REPLACE "-fsanitize=undefined" "" CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS}")
  string(REPLACE "-fsanitize=undefined" "" CMAKE_C_FLAGS "${CMAKE_C_FLAGS}")
  set(CMAKE_CXX_FLAGS
      "${CMAKE_CXX_FLAGS}"
      CACHE STRING "" FORCE)
  set(CMAKE_C_FLAGS
      "${CMAKE_C_FLAGS}"
      CACHE STRING "" FORCE)
endif()

# Warnings Interface for each compiler supported
add_library(codex_warnings INTERFACE)

# Strict warning sets for Release/Shipping.
# I deliberately omit some warnings.
if(CMAKE_CXX_COMPILER_ID STREQUAL "GNU")
    set(cx_warnings_strict
        -Wall
        -Wextra
        -Werror
        -pedantic
        -Wnon-virtual-dtor
        # -Wstringop-overflow (part of -Wall) causes false positives on opaque-ish wrappers like Ref<T>::lock() & cxesnure().
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
    )
    set(cx_warnings_quiet -w)
elseif(CMAKE_CXX_COMPILER_ID MATCHES "^(Clang|AppleClang)$")
    set(cx_warnings_strict
        -Wall
        -Wextra
        -Werror
        -pedantic
        -Wnon-virtual-dtor
        -Wcast-align
        -Woverloaded-virtual
        -Wdouble-promotion
        -Wformat=2
        -Wimplicit-fallthrough
        -Wmisleading-indentation)

    if(CMAKE_CXX_COMPILER_FRONTEND_VARIANT STREQUAL "MSVC")
        # For some reason clang-cl includes C++98 compatability with -Wall.
        set(cx_warnings_strict /W4 /WX /clang:-pedantic)
    endif()
elseif(CMAKE_CXX_COMPILER_ID STREQUAL "MSVC")
    set(cx_warnings_strict
        /W4
        /WX
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
    )
    set(cx_warnings_quiet /w)
endif()
target_compile_options(codex_warnings INTERFACE
  "$<$<CONFIG:Debug>:${cx_warnings_quiet}>"
  "$<$<NOT:$<CONFIG:Debug>>:${cx_warnings_strict}>")


set(CX_ROOT_DIR ${CMAKE_SOURCE_DIR})
string(TOLOWER "${CMAKE_SYSTEM_PROCESSOR}" CX_ARCH)

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

if(CX_ENABLE_NETNT)
  add_subdirectory("NetNT")
endif()
if(CX_ENABLE_TESTWORK)
  add_subdirectory("test_work")
endif()
if(CX_ENABLE_CEDITOR)
  add_subdirectory("editor")
  if(TARGET CodexEditor-installed)
    set_property(DIRECTORY PROPERTY VS_STARTUP_PROJECT CodexEditor-installed)
  endif()
endif()
