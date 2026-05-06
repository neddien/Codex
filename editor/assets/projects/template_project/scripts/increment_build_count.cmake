# Reads BUILD_COUNT_FILE, increments the count, writes it back, then generates
# the version header at VERSION_HEADER_OUT.
# Required -D variables: BUILD_COUNT_FILE, VERSION_HEADER_OUT,
#                        CX_VER_MAJ, CX_VER_MIN, CX_VER_REV

if(EXISTS "${BUILD_COUNT_FILE}")
    file(READ "${BUILD_COUNT_FILE}" CX_BUILD_COUNT)
    string(STRIP "${CX_BUILD_COUNT}" CX_BUILD_COUNT)
else()
    set(CX_BUILD_COUNT 0)
endif()

math(EXPR CX_BUILD_COUNT "${CX_BUILD_COUNT} + 1")
file(WRITE "${BUILD_COUNT_FILE}" "${CX_BUILD_COUNT}\n")

get_filename_component(VERSION_HEADER_DIR "${VERSION_HEADER_OUT}" DIRECTORY)
file(MAKE_DIRECTORY "${VERSION_HEADER_DIR}")

file(WRITE "${VERSION_HEADER_OUT}"
"#pragma once

#define CX_VER_MAJ     ${CX_VER_MAJ}
#define CX_VER_MIN     ${CX_VER_MIN}
#define CX_VER_REV     ${CX_VER_REV}
#define CX_BUILD_COUNT ${CX_BUILD_COUNT}

#define CX_VERSION_STRING      \"${CX_VER_MAJ}.${CX_VER_MIN}.${CX_VER_REV}\"
#define CX_FULL_VERSION_STRING \"${CX_VER_MAJ}.${CX_VER_MIN}.${CX_VER_REV}+${CX_BUILD_COUNT}\"
")

message(STATUS "Codex version: ${CX_VER_MAJ}.${CX_VER_MIN}.${CX_VER_REV}+${CX_BUILD_COUNT}")
