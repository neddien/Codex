project("CodexEngine")

# Enable fPIC on all linkable targets
set(CMAKE_POSITION_INDEPENDENT_CODE on)

# Build options for some of these libraries
set(BOX2D_BUILD_TESTBED
    OFF
    CACHE BOOL "" FORCE)
set(BOX2D_BUILD_TESTBED
    OFF
    CACHE BOOL "" FORCE)
set(BOX2D_BUILD_UNIT_TESTS
    OFF
    CACHE BOOL "" FORCE)
set(BOX2D_BUILD_DOCS
    OFF
    CACHE BOOL "" FORCE)

set(SDL_AUDIO
    OFF
    CACHE BOOL "" FORCE)
set(SDL_RENDER
    OFF
    CACHE BOOL "" FORCE)
set(SDL_SENSOR
    OFF
    CACHE BOOL "" FORCE)
set(SDL_POWER
    OFF
    CACHE BOOL "" FORCE)
set(SDL_LOCALE
    OFF
    CACHE BOOL "" FORCE)
set(SDL_FILESYSTEM
    OFF
    CACHE BOOL "" FORCE)
set(SDL_TEST
    OFF
    CACHE BOOL "" FORCE)
# For now we want SDL as a static library to free ourselves from absurd DLL problems on Windows
set(SDL_SHARED
    OFF
    CACHE BOOL "" FORCE)
set(SDL_STATIC
    ON
    CACHE BOOL "" FORCE)

# Fetch modules
FetchContent_Declare(
  logex
  GIT_REPOSITORY git@github.com:neddien/Logex
  GIT_TAG master)
FetchContent_Declare(
  sdl
  GIT_REPOSITORY git@github.com:libsdl-org/SDL
  GIT_TAG release-2.32.x)
FetchContent_Declare(
  box2d
  GIT_REPOSITORY git@github.com:erincatto/box2d
  GIT_TAG v2.4.2)
FetchContent_Declare(
  imgui
  GIT_REPOSITORY git@github.com:ocornut/imgui
  GIT_TAG a028c2d)
FetchContent_Declare(
  json
  GIT_REPOSITORY git@github.com:nlohmann/json
  GIT_TAG v3.12.0)
FetchContent_Declare(
  lz4
  GIT_REPOSITORY https://github.com/lz4/lz4
  GIT_TAG v1.9.4
  SOURCE_SUBDIR build/cmake)

set(LZ4_BUILD_CLI        OFF CACHE BOOL "" FORCE)
set(LZ4_BUILD_LEGACY_LZ4C OFF CACHE BOOL "" FORCE)

FetchContent_MakeAvailable(logex)
FetchContent_MakeAvailable(sdl)
FetchContent_MakeAvailable(box2d)
FetchContent_MakeAvailable(imgui)
FetchContent_MakeAvailable(json)
FetchContent_MakeAvailable(lz4)

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

# SDL
if(ANDROID)
  find_package(SDL2 REQUIRED)
  target_include_directories(CodexEngine PUBLIC ${SDL2_INCLUDE_DIR})
  target_link_libraries(CodexEngine PUBLIC ${SDL2_LIBRARIES})

  target_include_directories(CodexEngine-static PUBLIC ${SDL2_INCLUDE_DIR})
  target_link_libraries(CodexEngine-static PUBLIC ${SDL2_LIBRARIES})
else()
  if(SDL_STATIC AND NOT SDL_SHARED)
    set(SDL2_LIBRARIES SDL2-static SDL2main)
  else()
    set(SDL2_LIBRARIES SDL2 SDL2main)
  endif()

  target_link_libraries(CodexEngine PUBLIC ${SDL2_LIBRARIES})
  add_dependencies(CodexEngine ${SDL2_LIBRARIES})

  target_include_directories(CodexEngine-static PUBLIC ${SDL2_INCLUDE_DIR})
  target_link_libraries(CodexEngine-static PUBLIC ${SDL2_LIBRARIES})
endif()

# Logex
set(LOGEX_LIBRARIES logex-static)
target_link_libraries(CodexEngine PUBLIC ${LOGEX_LIBRARIES})
target_link_libraries(CodexEngine-static PUBLIC ${LOGEX_LIBRARIES})

# nlohmann json
set(JSON_INCLUDE_DIR ${json_SOURCE_DIR}/include)
set(JSON_LIBRARIES nlohmann_json)
target_include_directories(CodexEngine PUBLIC ${JSON_INCLUDE_DIR})
target_link_libraries(CodexEngine PRIVATE ${JSON_LIBRARIES})
target_include_directories(CodexEngine-static PUBLIC ${JSON_INCLUDE_DIR})
target_link_libraries(CodexEngine-static PRIVATE ${JSON_LIBRARIES})

# ImGui
set(IMGUI_INCLUDE_DIR ${imgui_SOURCE_DIR}/ ${imgui_SOURCE_DIR}/backends
                      ${imgui_SOURCE_DIR}/examples ${imgui_SOURCE_DIR}/misc ${imgui_SOURCE_DIR}/misc/cpp)
file(
  GLOB_RECURSE
  IMGUI_HEADER_FILES
  "${imgui_SOURCE_DIR}/imconfig.h"
  "${imgui_SOURCE_DIR}/imgui.h"
  "${imgui_SOURCE_DIR}/imgui_internal.h"
  "${imgui_SOURCE_DIR}/imstb_rectpack.h"
  "${imgui_SOURCE_DIR}/imstb_textedit.h"
  "${imgui_SOURCE_DIR}/imstb_truetype.h"
  "${imgui_SOURCE_DIR}/misc/cpp/imgui_stdlib.h") 
file(
  GLOB_RECURSE
  IMGUI_SOURCE_FILES
  "${imgui_SOURCE_DIR}/imgui.cpp"
  "${imgui_SOURCE_DIR}/imgui_widgets.cpp"
  "${imgui_SOURCE_DIR}/imgui_draw.cpp"
  "${imgui_SOURCE_DIR}/imgui_demo.cpp"
  "${imgui_SOURCE_DIR}/imgui_tables.cpp"
  "${imgui_SOURCE_DIR}/backends/imgui_impl_opengl3.cpp"
  "${imgui_SOURCE_DIR}/backends/imgui_impl_sdl2.cpp"
  "${imgui_SOURCE_DIR}/misc/cpp/imgui_stdlib.cpp")
target_sources(CodexEngine PUBLIC ${IMGUI_HEADER_FILES} ${IMGUI_SOURCE_FILES})
target_sources(CodexEngine-static PUBLIC ${IMGUI_HEADER_FILES}
                                         ${IMGUI_SOURCE_FILES})

target_include_directories(CodexEngine PUBLIC ${IMGUI_INCLUDE_DIR})
target_include_directories(CodexEngine-static PUBLIC ${IMGUI_INCLUDE_DIR})

# glm
set(GLM_LIBRARIES glm)
target_link_libraries(CodexEngine PRIVATE ${GLM_LIBRARIES})
target_link_libraries(CodexEngine-static PRIVATE ${GLM_LIBRARIES})
target_include_directories(CodexEngine PUBLIC "vendor/glm")
target_include_directories(CodexEngine-static PUBLIC "vendor/glm")

# box2d Disable box2d's test sub-project, we don't need it.
set(CMAKE_POLICY_DEFAULT_CMP0077 NEW)
set(BOX2D_BUILD_TESTBED OFF)
set(BOX2D_INCLUDE_DIR ${box2d_SOURCE_DIR}/include)
set(BOX2D_LIBRARIES box2d)
target_include_directories(CodexEngine PUBLIC ${BOX2D_INCLUDE_DIR})
target_link_libraries(CodexEngine PRIVATE ${BOX2D_LIBRARIES})
target_include_directories(CodexEngine-static PUBLIC ${BOX2D_INCLUDE_DIR})
target_link_libraries(CodexEngine-static PRIVATE ${BOX2D_LIBRARIES})

# lz4
target_link_libraries(CodexEngine PUBLIC lz4_static)
target_link_libraries(CodexEngine-static PUBLIC lz4_static)

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

# FMOD Studio API 2.03.12
if(DEFINED CX_PLATFORM_LINUX)
  set(FMOD_DIR
      "${CMAKE_CURRENT_SOURCE_DIR}/vendor/fmod/fmodstudioapi20312linux")
  set(FMOD_CORE_LIB_DIR "${FMOD_DIR}/api/core/lib/${CX_CPU_ARCH}")
  set(FMOD_STUDIO_LIB_DIR "${FMOD_DIR}/api/studio/lib/${CX_CPU_ARCH}")

  # Debug builds use logging variants (L suffix).
  if(DEFINED CX_BUILD_TYPE_DEBUG)
    set(FMOD_CORE_LIB_FILE "libfmodL.so.14.12")
    set(FMOD_CORE_SONAME "libfmodL.so.14")
    set(FMOD_STUDIO_LIB_FILE "libfmodstudioL.so.14.12")
    set(FMOD_STUDIO_SONAME "libfmodstudioL.so.14")
  else()
    set(FMOD_CORE_LIB_FILE "libfmod.so.14.12")
    set(FMOD_CORE_SONAME "libfmod.so.14")
    set(FMOD_STUDIO_LIB_FILE "libfmodstudio.so.14.12")
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
  install(TARGETS SDL2-static SDL2main glm nlohmann_json CodexEngine DESTINATION bin)
  install(FILES "${FMOD_CORE_DLL_PATH}" "${FMOD_STUDIO_DLL_PATH}" DESTINATION bin)

  # Program Files (or same location as the binary) for unmodifiable file data.
  # App Data for modifidable file data.
elseif(DEFINED CX_PLATFORM_UNIX)
  install(
    TARGETS SDL2-static
            SDL2main
            glm
            nlohmann_json
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
