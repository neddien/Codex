project("CEditor")

# Fetch modules
FetchContent_Declare(
  imguizmo
  GIT_REPOSITORY git@github.com:CedricGuillemet/ImGuizmo
  GIT_TAG ba662b1)
FetchContent_MakeAvailable(imguizmo)

# For running Python scripts from CMake.
find_package(
  Python3
  COMPONENTS Interpreter
  REQUIRED)

file(GLOB_RECURSE CEDITOR_SOURCE_FILES "src/*.cpp")
file(GLOB_RECURSE CEDITOR_HEADER_FILES "src/*.h")
file(GLOB_RECURSE CEDITOR_MODULE_FILES "src/*.ixx")

add_executable(CodexEditor ${CEDITOR_HEADER_FILES} ${CEDITOR_SOURCE_FILES}
                           ${CEDITOR_MODULE_FILES})

# Set the C++ Standard to 20 for this target
set_property(TARGET CodexEditor PROPERTY CXX_STANDARD 20)

target_link_libraries(CodexEditor CodexEngine-static)
add_dependencies(CodexEditor CodexEngine-static)

get_target_property(CODEX_BUILD_DIR CodexEngine BINARY_DIR)

get_target_property(CODEX_CMAKE_SOURCE_DIR CodexEngine SOURCE_DIR)

if(DEFINED CX_BUILD_TYPE_RELEASE OR DEFINED CX_BUILD_TYPE_SHIPPING)
  if(DEFINED CX_COMPILER_CLANG OR DEFINED CX_COMPILER_GNUC)
    target_compile_options(CodexEngine PRIVATE -Wall -Wextra -Werror -pedantic)
    target_compile_options(CodexEngine-static PRIVATE -Wall -Wextra -Werror
                                                      -pedantic)
  elseif(DEFINED CX_COMPILER_MSVC)

  endif()
endif()

# tinyfiledialogs
set(TFDS_INCLUDE_DIRS vendor/tinyfiledialogs)
set(TFDS_SOURCE_FILES vendor/tinyfiledialogs/tinyfiledialogs.c)

target_include_directories(CodexEditor PUBLIC ${TFDS_INCLUDE_DIRS})
target_sources(CodexEditor PRIVATE ${TFDS_SOURCE_FILES})

# ImGuizmo
set(IMGUIZMO_INCLUDE_DIRS ${imguizmo_SOURCE_DIR})
set(IMGUIZMO_SOURCE_FILES ${imguizmo_SOURCE_DIR}/ImGuizmo.cpp)

# Export the installation path to the engine.
if(DEFINED CX_COMPILER_GCC OR DEFINED CX_COMPILER_CLANG)
  add_definitions(-DCE_INSTALL_DIR="${CMAKE_INSTALL_PREFIX}")
elseif(DEFINED CX_COMPILER_MSVC)
  add_definitions(/DCE_INSTALL_DIR="${CMAKE_INSTALL_PREFIX}")
endif()

# Export CEditor's install directory to the Environmennt for dap-mode.
set(ENV{CE_INSTALL_DIR} "${CMAKE_INSTALL_PREFIX}")
get_target_property(CE_BIN_NAME CodexEditor NAME)
set(ENV{CE_BIN_NAME} "${CE_BIN_NAME}")
message(":: CE_INSTALL_DIR: $ENV{CE_INSTALL_DIR}")

# === Dependencies ===
if(DEFINED CX_PLATFORM_WINDOWS)
  install(TARGETS CodexEditor DESTINATION bin)
  install(DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}/assets/" DESTINATION bin)
  install(DIRECTORY "${CX_ENGINE_ASSET_PATH}/" DESTINATION bin)
elseif(DEFINED CX_PLATFORM_UNIX)
  install(TARGETS CodexEditor DESTINATION bin)
  install(DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}/assets/"
          DESTINATION share/${PROJECT_NAME})
  install(DIRECTORY "${CX_ENGINE_ASSET_PATH}/"
          DESTINATION share/${PROJECT_NAME})
endif()

target_include_directories(CodexEditor PUBLIC ${IMGUIZMO_INCLUDE_DIRS})
target_sources(CodexEditor PRIVATE ${IMGUIZMO_SOURCE_FILES})

# === Misc ===
# Run the python script responsible for creating the template project for all
# projects inside Projects/
add_custom_target(
  engine_header_gen
  COMMENT ":: Generate engine headers for template projects."
  WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
  VERBATIM)

# Iterate over all subdirectories in assets/Projects and apply the generation
# and copying steps
file(
  GLOB PROJECT_DIRS
  RELATIVE "${CMAKE_CURRENT_SOURCE_DIR}/assets/Projects"
  "${CMAKE_CURRENT_SOURCE_DIR}/assets/Projects/*")

# Iterate over each project directory in Projects/
foreach(PROJECT_DIR ${PROJECT_DIRS})
  if(NOT PROJECT_DIR STREQUAL "*"
     AND IS_DIRECTORY
         "${CMAKE_CURRENT_SOURCE_DIR}/assets/Projects/${PROJECT_DIR}")

    set(CX_ENGINE_COPY_SPECS 
        "include:Engine/include:public"
        "src/Engine:Engine/src/Engine:public"
        "src/PCH:Engine/src/PCH:headers"
        "src/Platform:Engine/src/Platform:headers"
    )
    string(REPLACE ";" "," CX_ENGINE_COPY_SPECS_PROC "${CX_ENGINE_COPY_SPECS}")

    # Run the python script for each project
    add_custom_command(
      TARGET engine_header_gen
      POST_BUILD
      COMMAND
        ${Python3_EXECUTABLE}
        "${CX_ROOT_DIR}/Scripts/generate-template-project.py"
        "--src-dir=${CX_ENGINE_ROOT_DIR}"
        "--dest-dir=${CMAKE_CURRENT_SOURCE_DIR}/assets/Projects/${PROJECT_DIR}"
        "--root-dir=${CX_ROOT_DIR}"
        "--copy-specs=${CX_ENGINE_COPY_SPECS_PROC}"
      COMMENT ":: Generating engine headers for ${PROJECT_DIR}...")

    # If we're on Windows, copy the libraries for each project
    if(DEFINED CX_PLATFORM_WINDOWS)
      add_custom_command(
        TARGET engine_header_gen
        POST_BUILD
        COMMAND
          ${CMAKE_COMMAND} -E make_directory
          "${CMAKE_CURRENT_SOURCE_DIR}/assets/Projects/${PROJECT_DIR}/lib/"
        COMMAND
          ${CMAKE_COMMAND} -E copy
          "$<TARGET_FILE_DIR:CodexEngine>/CodexEngine.lib"
          "${CMAKE_CURRENT_SOURCE_DIR}/assets/Projects/${PROJECT_DIR}/lib/CodexEngine.lib"
        COMMAND
          ${CMAKE_COMMAND} -E copy "$<TARGET_FILE:fmt>"
          "${CMAKE_CURRENT_SOURCE_DIR}/assets/Projects/${PROJECT_DIR}/lib/"
        COMMAND
          ${CMAKE_COMMAND} -E copy "$<TARGET_FILE:Logex-static>"
          "${CMAKE_CURRENT_SOURCE_DIR}/assets/Projects/${PROJECT_DIR}/lib/"
        COMMENT ":: Copying object libraries to ${PROJECT_DIR}/lib/...")
    endif()

  endif()
endforeach()

add_dependencies(CodexEditor engine_header_gen)
target_include_directories(CodexEditor PRIVATE "src/")

# Multicore compilation for MSVC if CX_PARALLEL is not 0.
if(DEFINED CX_COMPILER_MSVC AND NOT CX_PARALLEL EQUAL 0)
  target_compile_options(CodexEditor PRIVATE "/MP")
endif()

# Set default startup project to be this one when generating a VS Solution.
set_target_properties(CodexEditor PROPERTIES VS_STARTUP_PROJECT CEditor)
