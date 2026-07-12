cmake_minimum_required(VERSION 3.16)

project("TemplateProject")

include(FetchContent)

# For running Python scripts from CMake.
find_package(
  Python3
  COMPONENTS Interpreter
  REQUIRED)

# Libraries
find_package(fmt REQUIRED)
find_package(EnTT REQUIRED)
find_package(glm REQUIRED)
find_package(magic_enum REQUIRED)
find_package(absl REQUIRED)
find_package(enet REQUIRED)

# For LSPs.
set(CMAKE_EXPORT_COMPILE_COMMANDS on)

# Variables
set(CODEGEN_DIR "${CMAKE_CURRENT_SOURCE_DIR}/int")
set(REFLECTOR "${CMAKE_SOURCE_DIR}/scripts/reflector.py")

# Files
# CONFIGURE_DEPENDS re-triggers CMake when headers are added/removed.
file(GLOB_RECURSE HEADER_FILES CONFIGURE_DEPENDS "${CMAKE_CURRENT_SOURCE_DIR}/assets/*.h")
file(GLOB_RECURSE SOURCE_FILES CONFIGURE_DEPENDS "${CMAKE_CURRENT_SOURCE_DIR}/assets/*.cpp")
file(GLOB_RECURSE STATIC_LIBS "${CMAKE_CURRENT_SOURCE_DIR}/lib/*.lib")
file(GLOB VENDOR_SUBDIRS "${CMAKE_CURRENT_SOURCE_DIR}/engine/vendor/*")

# Determine which headers contain RF_CLASS and compute the expected codegen
# output filenames (reflector names outputs after the input header stem).
# Doing this at configure time lets CMake know about the generated files before
# they exist so they are compiled on the very first build.
set(REFLECTABLE_HEADERS "")
foreach(_header ${HEADER_FILES})
    file(STRINGS "${_header}" _rf_check REGEX "RF_CLASS")
    if(_rf_check)
        list(APPEND REFLECTABLE_HEADERS "${_header}")
    endif()
endforeach()

set(CODEGEN_SOURCES "")
foreach(_header ${REFLECTABLE_HEADERS})
    get_filename_component(_stem "${_header}" NAME_WE)
    list(APPEND CODEGEN_SOURCES "${CODEGEN_DIR}/${_stem}.cxr.cpp")
endforeach()

# Mark the files as generated so CMake doesn't require them to exist yet.
if(CODEGEN_SOURCES)
    set_source_files_properties(${CODEGEN_SOURCES} PROPERTIES GENERATED TRUE)
endif()

# Collect macros to pass to the reflector.
set(REFLECTOR_DEFINES "")
if(DEFINED CX_PLATFORM_LINUX)
    list(APPEND REFLECTOR_DEFINES "-DCX_PLATFORM_LINUX=1")
    list(APPEND REFLECTOR_DEFINES "-DCX_PLATFORM_UNIX=1")
elseif(DEFINED CX_PLATFORM_OSX)
    list(APPEND REFLECTOR_DEFINES "-DCX_PLATFORM_OSX=1")
    list(APPEND REFLECTOR_DEFINES "-DCX_PLATFORM_UNIX=1")
elseif(DEFINED CX_PLATFORM_WINDOWS)
    list(APPEND REFLECTOR_DEFINES "-DCX_PLATFORM_WINDOWS=1")
endif()

if(DEFINED CX_COMPILER_GNUC)
    list(APPEND REFLECTOR_DEFINES "-DCX_COMPILER_GNUC=1")
elseif(DEFINED CX_COMPILER_CLANG)
    list(APPEND REFLECTOR_DEFINES "-DCX_COMPILER_CLANG=1")
elseif(DEFINED CX_COMPILER_MSVC)
    list(APPEND REFLECTOR_DEFINES "-DCX_COMPILER_MSVC=1")
endif()

# Reflector target — list the actual .cxr.cpp files as OUTPUT so the
# compiler dependency chain is correct from the first build.
add_custom_command(
    OUTPUT ${CODEGEN_SOURCES} "${CODEGEN_DIR}/refl.stamp"
    WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
    COMMAND ${CMAKE_COMMAND} -E make_directory ${CODEGEN_DIR}
    COMMAND ${Python3_EXECUTABLE} ${REFLECTOR} ${HEADER_FILES} ${SOURCE_FILES} -o ${CODEGEN_DIR} --compile-commands=${CMAKE_BINARY_DIR} ${REFLECTOR_DEFINES}
    COMMAND ${CMAKE_COMMAND} -E touch "${CODEGEN_DIR}/refl.stamp"
    DEPENDS ${HEADER_FILES}
    COMMENT "Generating reflection code for native behaviour scripts..."
)
add_custom_target(GenerateReflection ALL DEPENDS ${CODEGEN_SOURCES} "${CODEGEN_DIR}/refl.stamp")

# The target
add_library(NBMan SHARED ${HEADER_FILES} ${SOURCE_FILES} ${CODEGEN_SOURCES})
add_dependencies(NBMan GenerateReflection)

if(DEFINED CX_PLATFORM_WINDOWS)

endif()

set_property(TARGET NBMan PROPERTY CXX_STANDARD 23)

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

if(CMAKE_CXX_COMPILER_ID STREQUAL "GNU")
  # GCC emits STB_GNU_UNIQUE symbols for inline statics (the engine headers pull
  # plenty of these in: System<T>::get() instances, entt type-index counters...),
  # and glibc marks any DSO exporting such symbols as NODELETE. dlclose() then
  # becomes a permanent no-op, so the editor's script hot-reload silently reuses
  # the old module image and its static registrars never re-run. Clang and MSVC
  # do not emit these symbols. NOTE: if another library is ever linked INTO this
  # module, its sources need this flag as well.
  target_compile_options(NBMan PRIVATE -fno-gnu-unique)
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

# Installing a project must also restore its native behaviour module under the
# project root. An absolute destination is intentional here: this is mutable
# project output consumed by the editor, not part of the toolchain install
# prefix. CMake creates the directory when it does not exist.
install(
  TARGETS NBMan
  RUNTIME DESTINATION "${CMAKE_CURRENT_SOURCE_DIR}/lib"
  LIBRARY DESTINATION "${CMAKE_CURRENT_SOURCE_DIR}/lib"
)

# Expose Codex's dependencies.
foreach(dir ${VENDOR_SUBDIRS})
  if(IS_DIRECTORY ${dir})
    target_include_directories(NBMan PUBLIC ${dir})
  endif()
endforeach()

target_include_directories(NBMan PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}/engine/src/pch")

# Libraries - Shared libraries will have only their headers included while static libraries will be linked
target_include_directories(NBMan PUBLIC
      $<TARGET_PROPERTY:fmt::fmt,INTERFACE_INCLUDE_DIRECTORIES>)
target_link_libraries(NBMan PUBLIC EnTT::EnTT glm::glm magic_enum::magic_enum abseil::abseil enet::enet)

# TODO: Fix glad issue
#target_include_directories(NBMan PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}/engine/vendor")

# For ease of use.
target_include_directories(NBMan PUBLIC "engine/src")
target_include_directories(NBMan PUBLIC ${CMAKE_SOURCE_DIR})

target_precompile_headers(NBMan PRIVATE "engine/src/pch/sdafx.h")

# Expose the engine API
target_include_directories(NBMan PUBLIC "engine/include/public")

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
