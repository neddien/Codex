# Fetch all the source and header files and the then add them automatically
file(GLOB_RECURSE SOURCES "src/*.cpp")
file(GLOB_RECURSE HEADERS "src/*.h")

add_library(CodexNative SHARED ${HEADERS} ${SOURCES})

# Set the C++ Standard to 17 for this target
set_property(TARGET CodexNative PROPERTY CXX_STANDARD 17)

target_include_directories(CodexNative PUBLIC src/)
target_link_libraries(CodexNative CodexEngine)