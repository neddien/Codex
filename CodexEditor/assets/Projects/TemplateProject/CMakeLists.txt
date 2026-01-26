cmake_minimum_required(VERSION 3.16)

project("TemplateProject")

include(FetchContent)

# For running Python scripts from CMake.
find_package(
  Python3
  COMPONENTS Interpreter
  REQUIRED)

FetchContent_Declare(
  logex
  GIT_REPOSITORY git@github.com:neddien/Logex
  GIT_TAG master)
FetchContent_MakeAvailable(logex)

# For LSPs.
set(CMAKE_EXPORT_COMPILE_COMMANDS on)

# Variables
set(CODEGEN_DIR "${CMAKE_CURRENT_SOURCE_DIR}/int")
set(REFLECTOR "${CMAKE_SOURCE_DIR}/Scripts/reflector.py")

# Files
file(GLOB_RECURSE HEADER_FILES "${CMAKE_CURRENT_SOURCE_DIR}/Assets/*.h")
file(GLOB_RECURSE SOURCE_FILES "${CMAKE_CURRENT_SOURCE_DIR}/Assets/*.cpp")
file(GLOB_RECURSE CODEGEN_HEADERS "${CODEGEN_DIR}/*.h")
file(GLOB_RECURSE CODEGEN_SOURCES "${CODEGEN_DIR}/*.cxr.cpp")
file(GLOB_RECURSE STATIC_LIBS "${CMAKE_CURRENT_SOURCE_DIR}/lib/*.lib")

# Reflector target
add_custom_command(
    OUTPUT ${CODEGEN_DIR}/refl.stamp
    WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
    COMMAND ${CMAKE_COMMAND} -E make_directory ${CODEGEN_DIR}
    COMMAND ${Python3_EXECUTABLE} ${REFLECTOR} ${HEADER_FILES} -o ${CODEGEN_DIR} --compile-commands=${CMAKE_BINARY_DIR}
    DEPENDS ${HEADER_FILES}
    COMMENT "Generating reflection code for NativeBehaviour scripts..."
)
add_custom_target(GenerateReflection ALL DEPENDS ${CODEGEN_DIR}/refl.stamp)

message(":: FILES: " ${CODEGEN_SOURCES})

# The target
add_library(NBMan SHARED ${HEADER_FILES} ${SOURCE_FILES} ${CODEGEN_HEADERS}
                         ${CODEGEN_SOURCES})
add_dependencies(NBMan GenerateReflection)

if(DEFINED CX_PLATFORM_WINDOWS)

endif()

set_property(TARGET NBMan PROPERTY CXX_STANDARD 20)
set(CMAKE_SHARED_LIBRARY_PREFIX "lib")
set(CMAKE_STATIC_LIBRARY_PREFIX "lib")
set_target_properties(NBMan PROPERTIES OUTPUT_NAME "NBMan")
set_target_properties(NBMan PROPERTIES SUFFIX ".dll")

# ==================================================== # Export macros that
# describe the current platform   #
# ==================================================== #

if(CMAKE_SYSTEM_NAME STREQUAL "Linux")
  set(CX_PLATFORM_LINUX 1)
  set(CX_PLATFORM_UNIX 1)
elseif(CMAKE_SYSTEM_NAME STREQUAL "Darwin")
  set(CX_PLATFORM_OSX 1)
  set(CX_PLATFORM_UNIX 1)
elseif(CMAKE_SYSTEM_NAME STREQUAL "Windows")
  set(CX_PLATFORM_WINDOWS 1)
else()
  set(CX_PLATFORM_UNKNOWN 1)
endif()

if(CMAKE_CXX_COMPILER_ID STREQUAL "GNU")
  set(CX_COMPILER_GNUC 1)
elseif(CMAKE_CXX_COMPILER_ID STREQUAL "Clang" OR CMAKE_CXX_COMPILER_ID STREQUAL
                                                 "AppleClang")
  set(CX_COMPILER_CLANG 1)
elseif(CMAKE_CXX_COMPILER_ID STREQUAL "MSVC")
  set(CX_COMPILER_MSVC 1)
else()
  set(CX_COMPILER_UNKNOWN 1)
endif()

# Determine the processor architecture
if(CMAKE_SIZEOF_VOID_P EQUAL 8)
  set(CX_WORD_SIZE_64 1)
else()
  set(CX_WORD_SIZE_32 1)
endif()

# Define macros that describe the current platform.
if(DEFINED CX_PLATFORM_LINUX)
  target_compile_definitions(NBMan PUBLIC CX_PLATFORM_LINUX)
  target_compile_definitions(NBMan PUBLIC CX_PLATFORM_UNIX)
  #target_link_libraries(NBMan PRIVATE
  #${CMAKE_CURRENT_SOURCE_DIR}/lib/libCodexEngine-static.a
  #${CMAKE_CURRENT_SOURCE_DIR}/lib/libMGL.a
  #${CMAKE_CURRENT_SOURCE_DIR}/lib/libfmtd.a )
elseif(DEFINED CX_PLATFORM_OSX)
  target_compile_definitions(NBMan PUBLIC CX_PLATFORM_OSX)
  target_compile_definitions(NBMan PUBLIC CX_PLATFORM_UNIX)
  target_link_options(NBMan PUBLIC -dynamiclib -flat_namespace -undefined dynamic_lookup)
elseif(DEFINED CX_PLATFORM_WINDOWS)
  target_compile_definitions(NBMan PUBLIC CX_PLATFORM_WINDOWS)
  target_link_libraries(NBMan PUBLIC ${STATIC_LIBS})
else()
  target_compile_definitions(NBMan PUBLIC CX_PLATFORM_UNKNOWN)
endif()

if(DEFINED CX_COMPILER_GNUC)
  target_compile_definitions(NBMan PUBLIC CX_COMPILER_GNUC)
elseif(DEFINED CX_COMPILER_CLANG)
  target_compile_definitions(NBMan PUBLIC CX_COMPILER_CLANG)
elseif(DEFINED CX_COMPILER_MSVC)
  target_compile_definitions(NBMan PUBLIC CX_COMPILER_MSVC)
else()
  target_compile_definitions(NBMan PUBLIC CX_COMPILER_UNKNOWN)
endif()

# Determine the processor architecture
if(DEFINED CX_WORD_SIZE_64)
  target_compile_definitions(NBMan PUBLIC CX_WORD_SIZE_64)
else()
  target_compile_definitions(NBMan PUBLIC CX_WORD_SIZE_32)
endif()

# Print the platform information
message(":: Platform OS: ${CMAKE_SYSTEM_NAME}")
message(":: Platform Compiler: ${CMAKE_CXX_COMPILER_ID}")
message(":: CPU word size: ${CMAKE_SIZEOF_VOID_P} bytes")
message("")
 
add_custom_command(
  TARGET NBMan
  POST_BUILD
  COMMAND ${CMAKE_COMMAND} -E make_directory "${CMAKE_CURRENT_SOURCE_DIR}/lib"
  COMMAND ${CMAKE_COMMAND} -E copy $<TARGET_FILE:NBMan> "${CMAKE_CURRENT_SOURCE_DIR}/lib/"
)

# Expose Codex's dependencies.
# TODO: Automate this please cause we have to add an entry here everytime we add a new library.
target_include_directories(NBMan PUBLIC Engine/vendor/entt)
target_include_directories(NBMan PUBLIC Engine/vendor/glm)
target_include_directories(NBMan PUBLIC Engine/vendor/glad)
target_include_directories(NBMan PUBLIC Engine/src/PCH)
target_include_directories(NBMan PUBLIC "${logex_SOURCE_DIR}/Logger/src")
target_include_directories(NBMan PUBLIC "${logex_SOURCE_DIR}/Logger/vendor/fmt/include")

# For ease of use.
target_include_directories(NBMan PUBLIC Engine/src)
target_include_directories(NBMan PUBLIC ${CMAKE_SOURCE_DIR})
# Reflector
target_include_directories(NBMan PRIVATE int/)

target_precompile_headers(NBMan PRIVATE Engine/src/PCH/sdafx.h)

# Expose the engine API
target_include_directories(NBMan PUBLIC Engine/include/Public)

# If we're not on VS, then copy compile_commands.json for LSPs like clangd and
# ccls.
if(NOT CMAKE_GENERATOR MATCHES "Visual Studio")
  add_custom_command(
    OUTPUT ${CMAKE_SOURCE_DIR}/compile_commands.json
    COMMAND ${CMAKE_COMMAND} -E copy ${CMAKE_BINARY_DIR}/compile_commands.json
            ${CMAKE_SOURCE_DIR}/compile_commands.json
    DEPENDS ${CMAKE_BINARY_DIR}/compile_commands.json
    COMMENT "Copying compile_commands.json...")

  add_custom_target(copy_compile_commands ALL
                    DEPENDS ${CMAKE_SOURCE_DIR}/compile_commands.json)
endif()
