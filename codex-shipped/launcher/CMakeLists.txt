add_executable(CodexLauncher "src/main.cpp")

set_property(TARGET CodexLauncher PROPERTY CXX_STANDARD 20)
target_link_libraries(CodexLauncher CodexEngine)
add_dependencies(CodexLauncher CodexEngine ShippedRuntime)
target_include_directories(CodexLauncher PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}/../common")

if(DEFINED CX_PLATFORM_UNIX)
    set_target_properties(CodexLauncher PROPERTIES INSTALL_RPATH "$ORIGIN/../lib")
endif()

get_target_property(CODEX_CMAKE_SOURCE_DIR CodexEngine SOURCE_DIR)
target_precompile_headers(CodexLauncher PRIVATE "${CODEX_CMAKE_SOURCE_DIR}/src/pch/sdafx.h")
target_include_directories(CodexLauncher PRIVATE "${CODEX_CMAKE_SOURCE_DIR}/src/pch")

if(DEFINED CX_BUILD_TYPE_DEBUG)
    if(DEFINED CX_COMPILER_MSVC)
        target_compile_options(CodexLauncher PRIVATE /W0)
    else()
        target_compile_options(CodexLauncher PRIVATE -w)
    endif()
endif()

install(TARGETS CodexLauncher DESTINATION bin)
