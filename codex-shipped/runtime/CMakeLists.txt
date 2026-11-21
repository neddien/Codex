add_executable(ShippedRuntime "src/main.cpp")

set_property(TARGET ShippedRuntime PROPERTY CXX_STANDARD 20)

target_link_libraries(ShippedRuntime CodexEngine)
add_dependencies(ShippedRuntime CodexEngine)
target_include_directories(ShippedRuntime PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}/../common")

if(UNIX)
    set_target_properties(ShippedRuntime PROPERTIES INSTALL_RPATH "${ORIGIN}/../lib")
endif()

get_target_property(CODEX_CMAKE_SOURCE_DIR CodexEngine SOURCE_DIR)

target_precompile_headers(ShippedRuntime PRIVATE "${CODEX_CMAKE_SOURCE_DIR}/src/pch/sdafx.h")
target_include_directories(ShippedRuntime PRIVATE "${CODEX_CMAKE_SOURCE_DIR}/src/pch")

install(TARGETS ShippedRuntime DESTINATION "${CX_INSTALL_BIN}")
