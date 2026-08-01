add_executable(ShippedRuntime "src/main.cpp")

set_property(TARGET ShippedRuntime PROPERTY CXX_STANDARD 20)

target_link_libraries(ShippedRuntime CodexEngine)
add_dependencies(ShippedRuntime CodexEngine)
target_include_directories(ShippedRuntime PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}/../common")

if(DEFINED CX_PLATFORM_UNIX)
    set_target_properties(ShippedRuntime PROPERTIES INSTALL_RPATH "$ORIGIN/../lib")
endif()

if(DEFINED CX_BUILD_TYPE_DEBUG)
    if(DEFINED CX_COMPILER_MSVC)
        target_compile_options(ShippedRuntime PRIVATE /W0)
    else()
        target_compile_options(ShippedRuntime PRIVATE -w)
    endif()
endif()

get_target_property(CODEX_CMAKE_SOURCE_DIR CodexEngine SOURCE_DIR)

target_precompile_headers(ShippedRuntime PRIVATE "${CODEX_CMAKE_SOURCE_DIR}/src/pch/sdafx.h")
target_include_directories(ShippedRuntime PRIVATE "${CODEX_CMAKE_SOURCE_DIR}/src/pch")

install(TARGETS ShippedRuntime DESTINATION bin)
install(DIRECTORY "${CX_ENGINE_ASSET_PATH}/" DESTINATION data)
