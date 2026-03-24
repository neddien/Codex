# Fetch all the source and header files and the then add them automatically
file(GLOB_RECURSE SOURCES "src/*.cpp")
file(GLOB_RECURSE HEADERS "src/*.h")

add_library(NetNT STATIC ${HEADERS} ${SOURCES})

# Set the C++ Standard to 17 for this target
set_property(TARGET NetNT PROPERTY CXX_STANDARD 17)

# boost asio
set(ASIO_INCLUDE_DIRS vendor/asio/include)
target_include_directories(NetNT PUBLIC ${ASIO_INCLUDE_DIRS})

set(NETLIB_INCLUDE_DIRS src/)
target_include_directories(NetNT PUBLIC ${NETLIB_INCLUDE_DIRS})