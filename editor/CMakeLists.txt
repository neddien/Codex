project("CEditor" LANGUAGES C CXX)

# For running Python scripts from CMake.
find_package(Python3 COMPONENTS Interpreter REQUIRED)

# Fetch modules
FetchContent_Declare(
  imguizmo
  GIT_REPOSITORY https://github.com/CedricGuillemet/ImGuizmo
  GIT_TAG ba662b1)

# nativefiledialog-extended (Wayland-compatible file dialogs via xdg-desktop-portal)
set(NFD_PORTAL OFF CACHE BOOL "Use xdg-desktop-portal instead of GTK")
FetchContent_Declare(
  nativefiledialog-extended
  GIT_REPOSITORY https://github.com/btzy/nativefiledialog-extended
  GIT_TAG v1.2.1)
FetchContent_MakeAvailable(nativefiledialog-extended)
FetchContent_MakeAvailable(imguizmo)

# Retrieve neccessary variables from our engine's target
get_target_property(cx_engine_src_dir CodexEngine SOURCE_DIR)
get_target_property(cx_engine_assets_dir CodexEngine CX_ASSET_DIR)
get_target_property(cx_imgui_backends_dir CodexEngine CX_IMGUI_BACKENDS_DIR)

file(GLOB_RECURSE CEDITOR_SOURCES "src/*.cpp" "src/*.h")
set(CEDITOR_THIRDPARTY_SOURCES
  "${imguizmo_SOURCE_DIR}/ImGuizmo.cpp"
  # imgui_stdlib isn't exported from the engine so we compile our own here.
  "${cx_imgui_backends_dir}/imgui_stdlib.cpp"
)

# Disable warnings on these third-party files, also, I hate generator expressions
set_source_files_properties(${CEDITOR_THIRDPARTY_SOURCES} PROPERTIES COMPILE_OPTIONS "$<IF:$<BOOL:${MSVC}>,/w,-w>")

# Define our editor target.
add_executable(CodexEditor "${CEDITOR_SOURCES}" "${CEDITOR_THIRDPARTY_SOURCES}")
set_property(TARGET CodexEditor PROPERTY CXX_STANDARD 20)
target_include_directories(CodexEditor PRIVATE "src/" "${CMAKE_SOURCE_DIR}/codex-shipped/common")
target_include_directories(CodexEditor SYSTEM PRIVATE "${imguizmo_SOURCE_DIR}")
target_link_libraries(CodexEditor PRIVATE CodexEngine codex_warnings nfd) # Link NFD and the warnings iface

# Installation
if(WIN32)
    set(cx_editor_data_dir "${CX_INSTALL_BIN}/data")
else()
    set(cx_editor_data_dir "${CX_INSTALL_ROOT}share/CEditor")
endif()
install(TARGETS CodexEditor RUNTIME DESTINATION "${CX_INSTALL_BIN}")
install(DIRECTORY "assets/" "${cx_engine_assets_dir}/" DESTINATION "${cx_editor_data_dir}")
if(WIN32)
    # Copy the dependency DLLs, Windows only
    install(FILES "$<TARGET_RUNTIME_DLLS:CodexEditor>" DESTINATION "${CX_INSTALL_BIN}")
endif()

add_custom_target(engine_header_gen COMMENT ":: Generate engine headers for template projects.")
add_dependencies(engine_header_gen CodexEngine)
add_dependencies(CodexEditor engine_header_gen)

file(GLOB cx_project_dirs LIST_DIRECTORIES true "${CMAKE_CURRENT_SOURCE_DIR}/assets/projects/*")
foreach(project_dir IN LISTS cx_project_dirs)
  if(NOT IS_DIRECTORY "${project_dir}")
      continue()
  endif()

  add_custom_command(TARGET engine_header_gen POST_BUILD
      COMMAND "${Python3_EXECUTABLE}" "${CMAKE_SOURCE_DIR}/scripts/generate-template-project.py"
      "--src-dir=${cx_engine_src_dir}"
      "--dest-dir=${project_dir}"
      "--root-dir=${CMAKE_SOURCE_DIR}"
      "--copy-specs=include:engine/include:public,src/engine:engine/src/engine:public,src/pch:engine/src/pch:headers,src/platform:engine/src/platform:headers")
endforeach()

# This is exclusively for VS2022, install and debug the Editor
set(cx_editor_install_bin "${CMAKE_INSTALL_PREFIX}/${CX_INSTALL_BIN}")
add_custom_target(CodexEditor-installed
  COMMAND "${CMAKE_COMMAND}" --install "${CMAKE_BINARY_DIR}" --config "$<CONFIG>"
  COMMENT "Installing $<CONFIG> for debugging"
  VERBATIM)

add_dependencies(CodexEditor-installed CodexEditor ShippedRuntime CodexLauncher)
if(TARGET TestWork)
    add_dependencies(CodexEditor-installed TestWork)
endif()

set_target_properties(CodexEditor-installed CodexEditor PROPERTIES
  VS_DEBUGGER_COMMAND "${cx_editor_install_bin}/$<TARGET_FILE_NAME:CodexEditor>"
  VS_DEBUGGER_COMMAND_DIRECTORY "${cx_editor_install_bin}")

target_precompile_headers(CodexEditor PRIVATE <sdafx.h>)
