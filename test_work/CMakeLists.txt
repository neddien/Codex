add_executable(TestWork "src/main.cpp")

set_property(TARGET TestWork PROPERTY CXX_STANDARD 20)

target_link_libraries(TestWork CodexEngine-static)
add_dependencies(TestWork CodexEngine-static)

get_target_property(CODEX_CMAKE_SOURCE_DIR CodexEngine-static SOURCE_DIR)

add_custom_command(
    TARGET TestWork
    POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy
        "${CODEX_CMAKE_SOURCE_DIR}/assets/gl_shaders/batch_renderer2d_quad.glsl"
        "$<TARGET_FILE_DIR:TestWork>/batch_renderer2d_quad.glsl"
    COMMAND ${CMAKE_COMMAND} -E copy
        "${CODEX_CMAKE_SOURCE_DIR}/assets/images/blue_pascal.png"
        "$<TARGET_FILE_DIR:TestWork>/blue_pascal.png"
    COMMENT ":: Copying TestWork assets"
)

install(TARGETS TestWork DESTINATION "${CX_INSTALL_BIN}")
install(FILES
    "${CODEX_CMAKE_SOURCE_DIR}/assets/gl_shaders/batch_renderer2d_quad.glsl"
    "${CODEX_CMAKE_SOURCE_DIR}/assets/images/blue_pascal.png"
    DESTINATION "${CX_INSTALL_BIN}")
