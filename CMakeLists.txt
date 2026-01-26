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

# If we're not on VS, then copy compile_commands.json for LSPs like clangd and
# ccls.
message(":: Generator: " ${CMAKE_GENERATOR})
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
add_subdirectory("CodexEngine")

if(DEFINED CX_ENABLE_NETNT)
  add_subdirectory("NetNT")
endif()
if(DEFINED CX_ENABLE_TESTGAME)
  #add_subdirectory("TestWork")
endif()
if(DEFINED CX_ENABLE_CEDITOR)
  add_subdirectory("CodexEditor")
endif()
