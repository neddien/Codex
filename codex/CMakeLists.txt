project("CodexEngine")

# ============= # Version      # ============= #
set(CX_VER_MAJ 0)
set(CX_VER_MIN 1)
set(CX_VER_REV 0)

# Enable fPIC on all linkable targets
set(CMAKE_POSITION_INDEPENDENT_CODE on)

# Fetch modules
FetchContent_Declare(
  logex
  GIT_REPOSITORY git@github.com:neddien/Logex
  GIT_TAG master)
FetchContent_MakeAvailable(logex)

# SDL2, box2d, nlohmann_json, lz4, imgui come from Conan (see conanfile.py)
find_package(SDL2 REQUIRED)
find_package(box2d REQUIRED)
find_package(nlohmann_json REQUIRED)
find_package(LZ4 REQUIRED)
find_package(imgui REQUIRED)

set(ENGINE_PLATFORM_DIR "src/platform")

# Fetch all the source and header files and the then add them automatically
file(GLOB_RECURSE ENGINE_SOURCES "src/engine/*.cpp")
file(GLOB_RECURSE ENGINE_HEADERS "src/engine/*.h")
file(GLOB_RECURSE ENGINE_INLINES "src/engine/*.inl")
file(GLOB_RECURSE PLATFORM_WINDOWS_SOURCES "${ENGINE_PLATFORM_DIR}/windows/*.cpp")
file(GLOB_RECURSE PLATFORM_LINUX_SOURCES "${ENGINE_PLATFORM_DIR}/linux/*.cpp")
file(GLOB_RECURSE PLATFORM_POSIX_SOURCES "${ENGINE_PLATFORM_DIR}/posix/*.cpp")
file(GLOB_RECURSE PLATFORM_OSX_SOURCES "${ENGINE_PLATFORM_DIR}/osx/*.cpp")
file(GLOB_RECURSE PLATFORM_OPENGL_SOURCES "${ENGINE_PLATFORM_DIR}/open_gl/*.cpp")

# Export all symbols on Windows (NOTE TO SELF: Do not uncomment).
#set(CMAKE_WINDOWS_EXPORT_ALL_SYMBOLS ON)
set(CX_ENGINE_ROOT_DIR
    ${CMAKE_CURRENT_SOURCE_DIR}
    PARENT_SCOPE)

add_library(CodexEngine SHARED ${ENGINE_HEADERS} ${ENGINE_INLINES} ${ENGINE_SOURCES})
add_library(CodexEngine-static STATIC ${ENGINE_HEADERS} ${ENGINE_SOURCES})

# Set the C++ Standard to 20 for this target.
set_property(TARGET CodexEngine PROPERTY CXX_STANDARD 20)
set_property(TARGET CodexEngine-static PROPERTY CXX_STANDARD 20)

# Tell the guy to export the symbols. This is only relevant for Windows really.
target_compile_definitions(CodexEngine PRIVATE CX_BUILD_SHARED)
target_compile_definitions(CodexEngine-static PUBLIC CX_BUILD_STATIC)

# Few things to export to the engine.
target_compile_definitions(CodexEngine
                           PRIVATE CX_PROJECT_NAME="${CMAKE_PROJECT_NAME}")
target_compile_definitions(CodexEngine-static
                           PRIVATE CX_PROJECT_NAME="${CMAKE_PROJECT_NAME}")
target_compile_definitions(CodexEngine
                           PRIVATE CX_PROJECT_NAME="${CMAKE_PROJECT_NAME}")
target_compile_definitions(CodexEngine-static
                           PRIVATE CX_PROJECT_NAME="${CMAKE_PROJECT_NAME}")

# Position independent code for shared libraries.
if(DEFINED CX_PLATFORM_UNIX)
  set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fPIC")
endif()

target_compile_definitions(CodexEngine PRIVATE CX_BUILD_LIB)

# Define macros that describe the current platform.
if(DEFINED CX_PLATFORM_LINUX)
  target_compile_definitions(CodexEngine PUBLIC CX_PLATFORM_LINUX)
  target_compile_definitions(CodexEngine PUBLIC CX_PLATFORM_UNIX)
  target_compile_definitions(CodexEngine-static PUBLIC CX_PLATFORM_LINUX)
  target_compile_definitions(CodexEngine-static PUBLIC CX_PLATFORM_UNIX)
  target_sources(CodexEngine PRIVATE ${PLATFORM_LINUX_SOURCES})
  target_sources(CodexEngine-static PRIVATE ${PLATFORM_LINUX_SOURCES})
  target_sources(CodexEngine PRIVATE ${PLATFORM_POSIX_SOURCES})
  target_sources(CodexEngine-static PRIVATE ${PLATFORM_POSIX_SOURCES})
  set(CX_GRAPHICS_API "OpenGL")
elseif(DEFINED CX_PLATFORM_OSX)
  target_compile_definitions(CodexEngine PUBLIC CX_PLATFORM_OSX)
  target_compile_definitions(CodexEngine PUBLIC CX_PLATFORM_UNIX)
  target_compile_definitions(CodexEngine-static PUBLIC CX_PLATFORM_OSX)
  target_compile_definitions(CodexEngine-static PUBLIC CX_PLATFORM_UNIX)
  target_sources(CodexEngine PRIVATE ${PLATFORM_OSX_SOURCES})
  target_sources(CodexEngine-static PRIVATE ${PLATFORM_OSX_SOURCES})
  target_sources(CodexEngine PRIVATE ${PLATFORM_POSIX_SOURCES})
  target_sources(CodexEngine-static PRIVATE ${PLATFORM_POSIX_SOURCES})
  set(CX_GRAPHICS_API "OpenGL")
elseif(DEFINED CX_PLATFORM_WINDOWS)
  target_compile_definitions(CodexEngine PUBLIC CX_PLATFORM_WINDOWS
                                                _CRT_SECURE_NO_WARNINGS)
  target_compile_definitions(CodexEngine-static PUBLIC CX_PLATFORM_WINDOWS
                                                       _CRT_SECURE_NO_WARNINGS)
  target_sources(CodexEngine PRIVATE ${PLATFORM_WINDOWS_SOURCES})
  target_sources(CodexEngine-static PRIVATE ${PLATFORM_WINDOWS_SOURCES})
  set(CX_GRAPHICS_API "OpenGL")
else()
  target_compile_definitions(CodexEngine PUBLIC CX_PLATFORM_UNKNOWN)
  target_compile_definitions(CodexEngine-static PUBLIC CX_PLATFORM_UNKNOWN)
  set(CX_GRAPHICS_API "Unknown")
endif()

# target_compile_definitions(CodexEngine PUBLIC CX_CPU_ARCH=\"${CX_CPU_ARCH}\")

if(DEFINED CX_COMPILER_GNUC)
  if(DEFINED CX_BUILD_TYPE_RELEASE OR DEFINED CX_BUILD_TYPE_SHIPPING)
    target_compile_options(CodexEngine PRIVATE -Wall -Wextra -Werror -pedantic)
    target_compile_options(CodexEngine-static PRIVATE -Wall -Wextra -Werror
                                                      -pedantic)
  endif()
  target_compile_definitions(CodexEngine PUBLIC CX_COMPILER_GNUC)
  target_compile_definitions(CodexEngine-static PUBLIC CX_COMPILER_GNUC)
elseif(DEFINED CX_COMPILER_CLANG)
  if(DEFINED CX_BUILD_TYPE_RELEASE OR DEFINED CX_BUILD_TYPE_SHIPPING)
    target_compile_options(CodexEngine PRIVATE -Wall -Wextra -Werror -pedantic)
    target_compile_options(CodexEngine-static PRIVATE -Wall -Wextra -Werror
                                                      -pedantic)
  endif()
  target_compile_definitions(CodexEngine PUBLIC CX_COMPILER_CLANG)
  target_compile_definitions(CodexEngine-static PUBLIC CX_COMPILER_CLANG)
  # target_compile_options(CodexEngine PUBLIC -fsanitize=address
  # -fno-omit-frame-pointer) target_link_options(CodexEngine PUBLIC
  # -fsanitize=address) target_compile_options(CodexEngine-static PUBLIC
  # -fsanitize=address -fno-omit-frame-pointer)
  # target_link_options(CodexEngine-static PUBLIC -fsanitize=address)
elseif(DEFINED CX_COMPILER_MSVC)
  if(DEFINED CX_BUILD_TYPE_RELEASE OR DEFINED CX_BUILD_TYPE_SHIPPING)
    target_compile_options(CodexEngine PRIVATE /Wall /W4 /WX)
    target_compile_options(CodexEngine-static PRIVATE /Wall /W4 /WX)
  endif()
  target_compile_definitions(CodexEngine PUBLIC CX_COMPILER_MSVC)
  target_compile_definitions(CodexEngine-static PUBLIC CX_COMPILER_MSVC)
else()
  target_compile_definitions(CodexEngine PUBLIC CX_COMPILER_UNKNOWN)
  target_compile_definitions(CodexEngine-static PUBLIC CX_COMPILER_UNKNOWN)
endif()

target_compile_definitions(CodexEngine PUBLIC CX_COMPILER_BIN="${CMAKE_CXX_COMPILER}")
target_compile_definitions(CodexEngine-static PUBLIC CX_COMPILER_BIN="${CMAKE_CXX_COMPILER}")

# Determine the processor architecture
if(DEFINED CX_WORD_SIZE_64)
  target_compile_definitions(CodexEngine PUBLIC CX_WORD_SIZE_64)
  target_compile_definitions(CodexEngine-static PUBLIC CX_WORD_SIZE_64)
else()
  target_compile_definitions(CodexEngine PUBLIC CX_WORD_SIZE_32)
  target_compile_definitions(CodexEngine-static PUBLIC CX_WORD_SIZE_32)
endif()

if(DEFINED CX_BUILD_TYPE_DEBUG)
  target_compile_definitions(CodexEngine PUBLIC CX_BUILD_TYPE_DEBUG)
  target_compile_definitions(CodexEngine-static PUBLIC CX_BUILD_TYPE_DEBUG)
  # Disable warnings for debug builds.
  if(DEFINED CX_COMPILER_MSVC)
    target_compile_options(CodexEngine PRIVATE /W0)
    target_compile_options(CodexEngine-static PRIVATE /W0)
  else()
    target_compile_options(CodexEngine PRIVATE -w)
    target_compile_options(CodexEngine-static PRIVATE -w)
  endif()
elseif(DEFINED CX_BUILD_TYPE_RELEASE)
  target_compile_definitions(CodexEngine PUBLIC CX_BUILD_TYPE_RELEASE)
  target_compile_definitions(CodexEngine-static PUBLIC CX_BUILD_TYPE_RELEASE)
elseif(DEFINED CX_BUILD_TYPE_SHIPPING)
  target_compile_definitions(CodexEngine PUBLIC CX_BUILD_TYPE_SHIPPING)
  target_compile_definitions(CodexEngine-static PUBLIC CX_BUILD_TYPE_SHIPPING)
endif()

# Print the platform information
message(":: Platform OS: ${CMAKE_SYSTEM_NAME}")
message(":: Platform Compiler: ${CMAKE_CXX_COMPILER_ID}")
message(":: CPU word size: ${CMAKE_SIZEOF_VOID_P} bytes")
message(":: Graphics API: ${CX_GRAPHICS_API}")
message("")

if(DEFINED CX_COMPILER_CLANG OR DEFINED CX_COMPILER_GCC)
  # set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fsanitize=address
  # -fno-omit-frame-pointer") set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS}
  # -fsanitize=address -fno-omit-frame-pointer")
endif()

# ============= # Graphics API # ============= #
if(CX_GRAPHICS_API STREQUAL "OpenGL") 
  # glad
  set(GLAD_INCLUDE_DIR "vendor/glad/include")
  set(GLAD_SOURCE_DIR "vendor/glad/src")
  target_include_directories(CodexEngine PUBLIC ${GLAD_INCLUDE_DIR})
  target_sources(CodexEngine PRIVATE ${GLAD_SOURCE_DIR}/glad.cpp)
  target_include_directories(CodexEngine-static PUBLIC ${GLAD_INCLUDE_DIR})
  target_sources(CodexEngine-static PRIVATE ${GLAD_SOURCE_DIR}/glad.cpp)

  # OpenGL
  target_sources(CodexEngine PRIVATE ${PLATFORM_OPENGL_SOURCES})
  target_sources(CodexEngine-static PRIVATE ${PLATFORM_OPENGL_SOURCES})

  # stb
  target_include_directories(CodexEngine PUBLIC "vendor/stb")
  target_include_directories(CodexEngine-static PUBLIC "vendor/stb")

  # GCC 15+ fails to compile stb_image's SSE2 SIMD path due to stricter
  # immediate-constant enforcement in _mm_slli_si128/_mm_srli_si128.
  if(CMAKE_CXX_COMPILER_ID STREQUAL "GNU")
    set_source_files_properties(
      src/platform/open_gl/stb_image.cpp
      PROPERTIES COMPILE_DEFINITIONS "STBI_NO_SIMD"
    )
  endif()

  # glm
  add_subdirectory(vendor/glm ${PROJECT_BINARY_DIR}/glm)
  set(GLM_INCLUDE_DIR "vendor/glm/glm")
  set(GLM_LIBRARIES glm)
  target_include_directories(CodexEngine PUBLIC ${GLM_INCLUDE_DIR})
  target_link_libraries(CodexEngine PRIVATE ${GLM_LIBRARIES})
  target_include_directories(CodexEngine-static PUBLIC ${GLM_INCLUDE_DIR})
  target_link_libraries(CodexEngine-static PRIVATE ${GLM_LIBRARIES})
endif()

# ============= # Libraries   # ============= #

# SDL2
if(ANDROID)
  target_link_libraries(CodexEngine PUBLIC SDL2::SDL2 SDL2::SDL2main)
  target_link_libraries(CodexEngine-static PUBLIC SDL2::SDL2 SDL2::SDL2main)
else()
  target_link_libraries(CodexEngine PUBLIC SDL2::SDL2-static SDL2::SDL2main)
  target_link_libraries(CodexEngine-static PUBLIC SDL2::SDL2-static SDL2::SDL2main)
endif()

# Logex
set(LOGEX_LIBRARIES logex-static)
target_link_libraries(CodexEngine PUBLIC ${LOGEX_LIBRARIES})
target_link_libraries(CodexEngine-static PUBLIC ${LOGEX_LIBRARIES})

# nlohmann json
target_link_libraries(CodexEngine PRIVATE nlohmann_json::nlohmann_json)
target_link_libraries(CodexEngine-static PRIVATE nlohmann_json::nlohmann_json)

# ImGui, core from Conan, backends compiled by us because they are
# thin platform-specific adapters that imgui intentionally excludes from the library.
# Conan's data cmake sets imgui_PACKAGE_FOLDER_<CONFIG> as a plain path.
string(TOUPPER "${CMAKE_BUILD_TYPE}" _imgui_cfg)
set(_imgui_root "${imgui_PACKAGE_FOLDER_${_imgui_cfg}}")

foreach(_cx_tgt CodexEngine CodexEngine-static)
    target_link_libraries(${_cx_tgt} PUBLIC imgui::imgui)
    target_include_directories(${_cx_tgt} PUBLIC
        "${_imgui_root}/res/bindings"
        "${_imgui_root}/res/misc/cpp")
    target_sources(${_cx_tgt} PRIVATE
        "${_imgui_root}/res/bindings/imgui_impl_opengl3.cpp"
        "${_imgui_root}/res/bindings/imgui_impl_sdl2.cpp"
        "${_imgui_root}/res/misc/cpp/imgui_stdlib.cpp")
endforeach()

# glm
set(GLM_LIBRARIES glm)
target_link_libraries(CodexEngine PRIVATE ${GLM_LIBRARIES})
target_link_libraries(CodexEngine-static PRIVATE ${GLM_LIBRARIES})
target_include_directories(CodexEngine PUBLIC "vendor/glm")
target_include_directories(CodexEngine-static PUBLIC "vendor/glm")

# box2d
target_link_libraries(CodexEngine PRIVATE box2d::box2d)
target_link_libraries(CodexEngine-static PRIVATE box2d::box2d)

# lz4
target_link_libraries(CodexEngine PUBLIC LZ4::lz4_static)
target_link_libraries(CodexEngine-static PUBLIC LZ4::lz4_static)

# Net Native
# set(NETNT_INCLUDE_DIR ${CMAKE_SOURCE_DIR}/NetNT/src)
# target_include_directories(CodexEngine PUBLIC ${NETNT_INCLUDE_DIR})
# target_link_libraries(CodexEngine NetNT)
# target_include_directories(CodexEngine-static PUBLIC ${NETNT_INCLUDE_DIR})
# target_link_libraries(CodexEngine-static NetNT)

# Link OpenGL (IF UNIX LIKE)
find_package(OpenGL REQUIRED)
target_include_directories(CodexEngine PUBLIC ${OPENGL_INCLUDE_DIR})
target_link_libraries(CodexEngine PRIVATE ${OPENGL_LIBRARIES})

target_include_directories(CodexEngine-static PUBLIC ${OPENGL_INCLUDE_DIR})
target_link_libraries(CodexEngine-static PRIVATE ${OPENGL_LIBRARIES})

# entt
set(ENTT_INCLUDE_DIR vendor/entt)
target_include_directories(CodexEngine PUBLIC ${ENTT_INCLUDE_DIR})
target_include_directories(CodexEngine-static PUBLIC ${ENTT_INCLUDE_DIR})

# magic_enum
set(MAGIC_ENUM_INCLUDE_DIR vendor/magic_enum)
target_include_directories(CodexEngine PUBLIC ${MAGIC_ENUM_INCLUDE_DIR})
target_include_directories(CodexEngine-static PUBLIC ${MAGIC_ENUM_INCLUDE_DIR})

# FMOD Studio API 2.03.13
if(DEFINED CX_PLATFORM_LINUX)
  set(FMOD_MINOR_VER 13)
  set(FMOD_DIR
      "${CMAKE_CURRENT_SOURCE_DIR}/vendor/fmod/fmodstudioapi203${FMOD_MINOR_VER}linux")
  set(FMOD_CORE_LIB_DIR "${FMOD_DIR}/api/core/lib/${CX_CPU_ARCH}")
  set(FMOD_STUDIO_LIB_DIR "${FMOD_DIR}/api/studio/lib/${CX_CPU_ARCH}")

  # Debug builds use logging variants (L suffix).
  if(DEFINED CX_BUILD_TYPE_DEBUG)
    set(FMOD_CORE_LIB_FILE "libfmodL.so.14.${FMOD_MINOR_VER}")
    set(FMOD_CORE_SONAME "libfmodL.so.14")
    set(FMOD_STUDIO_LIB_FILE "libfmodstudioL.so.14.${FMOD_MINOR_VER}")
    set(FMOD_STUDIO_SONAME "libfmodstudioL.so.14")
  else()
    set(FMOD_CORE_LIB_FILE "libfmod.so.14.${FMOD_MINOR_VER}")
    set(FMOD_CORE_SONAME "libfmod.so.14")
    set(FMOD_STUDIO_LIB_FILE "libfmodstudio.so.14.${FMOD_MINOR_VER}")
    set(FMOD_STUDIO_SONAME "libfmodstudio.so.14")
  endif()

  # Create IMPORTED targets for the pre-built FMOD libraries.
  add_library(fmod SHARED IMPORTED)
  set_target_properties(fmod PROPERTIES
    IMPORTED_LOCATION "${FMOD_CORE_LIB_DIR}/${FMOD_CORE_LIB_FILE}"
    IMPORTED_SONAME "${FMOD_CORE_SONAME}")

  add_library(fmodstudio SHARED IMPORTED)
  set_target_properties(fmodstudio PROPERTIES
    IMPORTED_LOCATION "${FMOD_STUDIO_LIB_DIR}/${FMOD_STUDIO_LIB_FILE}"
    IMPORTED_SONAME "${FMOD_STUDIO_SONAME}")

  target_include_directories(CodexEngine PUBLIC "${FMOD_DIR}/api/core/inc"
                                                "${FMOD_DIR}/api/studio/inc")
  target_include_directories(CodexEngine-static PUBLIC "${FMOD_DIR}/api/core/inc"
                                                       "${FMOD_DIR}/api/studio/inc")
  target_link_libraries(CodexEngine PRIVATE fmod fmodstudio)
  target_link_libraries(CodexEngine-static PRIVATE fmod fmodstudio)
elseif(DEFINED CX_PLATFORM_WINDOWS)
  set(FMOD_DIR
      "${CMAKE_CURRENT_SOURCE_DIR}/vendor/fmod/fmodstudioapi20312win")

  # Determine library architecture subdirectory.
  if(DEFINED CX_WORD_SIZE_64)
    set(FMOD_ARCH "x64")
  else()
    set(FMOD_ARCH "x86")
  endif()

  set(FMOD_CORE_LIB_DIR "${FMOD_DIR}/api/core/lib/${FMOD_ARCH}")
  set(FMOD_STUDIO_LIB_DIR "${FMOD_DIR}/api/studio/lib/${FMOD_ARCH}")

  # Debug builds use logging variants (L suffix).
  if(DEFINED CX_BUILD_TYPE_DEBUG)
    set(FMOD_CORE_LIB "fmodL_vc.lib")
    set(FMOD_CORE_DLL "fmodL.dll")
    set(FMOD_STUDIO_LIB "fmodstudioL_vc.lib")
    set(FMOD_STUDIO_DLL "fmodstudioL.dll")
  else()
    set(FMOD_CORE_LIB "fmod_vc.lib")
    set(FMOD_CORE_DLL "fmod.dll")
    set(FMOD_STUDIO_LIB "fmodstudio_vc.lib")
    set(FMOD_STUDIO_DLL "fmodstudio.dll")
  endif()

  # Create IMPORTED targets for the pre-built FMOD libraries.
  add_library(fmod SHARED IMPORTED)
  set_target_properties(fmod PROPERTIES
    IMPORTED_IMPLIB "${FMOD_CORE_LIB_DIR}/${FMOD_CORE_LIB}"
    IMPORTED_LOCATION "${FMOD_CORE_LIB_DIR}/${FMOD_CORE_DLL}")

  add_library(fmodstudio SHARED IMPORTED)
  set_target_properties(fmodstudio PROPERTIES
    IMPORTED_IMPLIB "${FMOD_STUDIO_LIB_DIR}/${FMOD_STUDIO_LIB}"
    IMPORTED_LOCATION "${FMOD_STUDIO_LIB_DIR}/${FMOD_STUDIO_DLL}")

  target_include_directories(CodexEngine PUBLIC "${FMOD_DIR}/api/core/inc"
                                                "${FMOD_DIR}/api/studio/inc")
  target_include_directories(CodexEngine-static PUBLIC "${FMOD_DIR}/api/core/inc"
                                                       "${FMOD_DIR}/api/studio/inc")
  target_link_libraries(CodexEngine PRIVATE fmod fmodstudio)
  target_link_libraries(CodexEngine-static PRIVATE fmod fmodstudio)

  # Store DLL paths for installation and post-build copy.
  set(FMOD_CORE_DLL_PATH "${FMOD_CORE_LIB_DIR}/${FMOD_CORE_DLL}" PARENT_SCOPE)
  set(FMOD_STUDIO_DLL_PATH "${FMOD_STUDIO_LIB_DIR}/${FMOD_STUDIO_DLL}" PARENT_SCOPE)
  set(FMOD_CORE_DLL_PATH "${FMOD_CORE_LIB_DIR}/${FMOD_CORE_DLL}")
  set(FMOD_STUDIO_DLL_PATH "${FMOD_STUDIO_LIB_DIR}/${FMOD_STUDIO_DLL}")

  # Copy FMOD DLLs to output directory after build.
  add_custom_command(TARGET CodexEngine POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_if_different
      "${FMOD_CORE_DLL_PATH}"
      "$<TARGET_FILE_DIR:CodexEngine>"
    COMMAND ${CMAKE_COMMAND} -E copy_if_different
      "${FMOD_STUDIO_DLL_PATH}"
      "$<TARGET_FILE_DIR:CodexEngine>"
    COMMENT "Copying FMOD DLLs to output directory"
  )
endif()

# Threads (for pthreads on Nix systems)
find_package(Threads REQUIRED)
target_link_libraries(CodexEngine PRIVATE Threads::Threads)
target_link_libraries(CodexEngine-static PRIVATE Threads::Threads)

# ================================= # Configuration specific macros   #
# ================================= #
target_compile_definitions(
  CodexEngine
  PRIVATE $<$<CONFIG:Debug>:CX_CONFIG_DEBUG>
          $<$<CONFIG:Release>:CX_CONFIG_RELEASE>
          $<$<CONFIG:Shipping>:CX_CONFIG_SHIPPING>)
target_compile_definitions(
  CodexEngine-static
  PRIVATE $<$<CONFIG:Debug>:CX_CONFIG_DEBUG>
          $<$<CONFIG:Release>:CX_CONFIG_RELEASE>
          $<$<CONFIG:Shipping>:CX_CONFIG_SHIPPING>)

# ======================= # INSTALLATION # ======================= #
if(DEFINED CX_PLATFORM_WINDOWS)
  install(TARGETS glm CodexEngine DESTINATION bin)
  install(FILES "${FMOD_CORE_DLL_PATH}" "${FMOD_STUDIO_DLL_PATH}" DESTINATION bin)

  # Program Files (or same location as the binary) for unmodifiable file data.
  # App Data for modifidable file data.
elseif(DEFINED CX_PLATFORM_UNIX)
  install(
    TARGETS glm
            CodexEngine
            CodexEngine-static
    DESTINATION lib)

  # Install FMOD libraries and create SONAME symlinks.
  install(FILES "${FMOD_CORE_LIB_DIR}/${FMOD_CORE_LIB_FILE}" DESTINATION lib)
  install(FILES "${FMOD_STUDIO_LIB_DIR}/${FMOD_STUDIO_LIB_FILE}" DESTINATION lib)
  install(CODE "
    execute_process(COMMAND \${CMAKE_COMMAND} -E create_symlink
      \"${FMOD_CORE_LIB_FILE}\"
      \"\$ENV{DESTDIR}\${CMAKE_INSTALL_PREFIX}/lib/${FMOD_CORE_SONAME}\")
    execute_process(COMMAND \${CMAKE_COMMAND} -E create_symlink
      \"${FMOD_STUDIO_LIB_FILE}\"
      \"\$ENV{DESTDIR}\${CMAKE_INSTALL_PREFIX}/lib/${FMOD_STUDIO_SONAME}\")
  ")

  # /etc/appname for global configuration files. /usr/local/share/appname for
  # unmodifiable data. /usr/local/(lib, bin)/appname for unmodifiable and
  # machine dependent data. /var/local/(lib, bin)/appname for modifiable data.
endif()

# Export the asset path.
set(CX_ENGINE_ASSET_PATH
    "${CMAKE_CURRENT_SOURCE_DIR}/assets"
    PARENT_SCOPE)
# if(DEFINED CX_PLATFORM_WINDOWS) install(TARGETS CodexEngine CodexEngine-static
# DESTINATION bin) install(DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}/assets/"
# DESTINATION bin) elseif(DEFINED CX_PLATFORM_UNIX) install(TARGETS CodexEngine
# CodexEngine-static DESTINATION bin) install(DIRECTORY
# "${CMAKE_CURRENT_SOURCE_DIR}/assets/" DESTINATION share/${PROJECT_NAME})
# endif()

# =============== # MISC # =============== # Fix a dumb fucking linker bug on
# Ubuntu regarding dlopen.
target_link_libraries(CodexEngine PRIVATE ${CMAKE_DL_LIBS})
target_link_libraries(CodexEngine-static PRIVATE ${CMAKE_DL_LIBS})

# Define standalone mode for the static library since the static version is
# going to be used for shipping target_compile_definitions(CodexEngine-static
# PRIVATE CX_MODE_STANDALONE 1)

# Make the base include file public
target_include_directories(CodexEngine PUBLIC include/)
target_include_directories(CodexEngine PUBLIC src/)

target_include_directories(CodexEngine-static PUBLIC include/)
target_include_directories(CodexEngine-static PUBLIC src/)

# ============= # Version header # ============= #
set(CX_VERSION_HEADER_OUT
    "${CMAKE_CURRENT_BINARY_DIR}/generated/engine/core/public/version.h")

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

add_dependencies(CodexEngine CodexVersionBump)
add_dependencies(CodexEngine-static CodexVersionBump)

target_include_directories(CodexEngine
  PUBLIC "${CMAKE_CURRENT_BINARY_DIR}/generated")
target_include_directories(CodexEngine-static
  PUBLIC "${CMAKE_CURRENT_BINARY_DIR}/generated")

# Add the precompiled header
target_include_directories(CodexEngine PUBLIC src/pch)
target_precompile_headers(CodexEngine PRIVATE src/pch/sdafx.h)

target_include_directories(CodexEngine-static PUBLIC src/pch)
target_precompile_headers(CodexEngine-static PRIVATE src/pch/sdafx.h)

# Add the Engine and Platform folders
target_include_directories(CodexEngine PRIVATE src/engine)
target_include_directories(CodexEngine-static PRIVATE src/engine)
target_include_directories(CodexEngine PRIVATE ${ENGINE_PLATFORM_DIR})
target_include_directories(CodexEngine-static PRIVATE ${ENGINE_PLATFORM_DIR})

# Multicore compilation for MSVC if CX_PARALLEL is not 0.
if(DEFINED CX_COMPILER_MSVC AND NOT CX_PARALLEL EQUAL 0)
  target_compile_options(CodexEngine PRIVATE "/MP")
  target_compile_options(CodexEngine-static PRIVATE "/MP")
endif()

# Treat all warnings as errors for QUALITY
if(CX_COMPILER_MSVC)
  # target_compile_options(CodexEngine PRIVATE /W3 /WX)
else()
  # target_compile_options(CodexEngine PRIVATE -Wall -Wextra -Wno-error=extra
  # -Werror)
endif()

# Set this to be the startup project when generating a VS Solution and CEditor
# is disabled.
if(NOT DEFINED CX_ENABLE_CEDITOR)
  set_target_properties(CodexEngine PROPERTIES VS_STARTUP_PROJECT CodexEngine)
endif()
