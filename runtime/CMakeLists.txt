add_executable(ShippedRuntime "src/main.cpp")

set_property(TARGET ShippedRuntime PROPERTY CXX_STANDARD 20)

target_link_libraries(ShippedRuntime CodexEngine-static)
add_dependencies(ShippedRuntime CodexEngine-static)

if(DEFINED CX_BUILD_TYPE_DEBUG)
    if(DEFINED CX_COMPILER_MSVC)
        target_compile_options(ShippedRuntime PRIVATE /W0)
    else()
        target_compile_options(ShippedRuntime PRIVATE -w)
    endif()
endif()

get_target_property(CODEX_CMAKE_SOURCE_DIR CodexEngine-static SOURCE_DIR)

target_precompile_headers(ShippedRuntime PRIVATE "${CODEX_CMAKE_SOURCE_DIR}/src/pch/sdafx.h")
target_include_directories(ShippedRuntime PRIVATE "${CODEX_CMAKE_SOURCE_DIR}/src/pch")

add_custom_command(
    TARGET ShippedRuntime
    POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy
        "${CODEX_CMAKE_SOURCE_DIR}/assets/gl_shaders/batch_renderer2d_quad.glsl"
        "$<TARGET_FILE_DIR:ShippedRuntime>/batch_renderer2d_quad.glsl"
    COMMAND ${CMAKE_COMMAND} -E copy
        "${CODEX_CMAKE_SOURCE_DIR}/assets/images/blue_pascal.png"
        "$<TARGET_FILE_DIR:ShippedRuntime>/blue_pascal.png"
    COMMENT ":: Copying ShippedRuntime assets"
)
