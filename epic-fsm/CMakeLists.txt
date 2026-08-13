# epic-fsm, vendor-agnostic table-driven finite state machine engine.
#
#   cmake -B build && cmake --build build
#   ./build/test_fsm
#
# Unlike the HAL/task-manager modules in this repo, this library has no
# hardware dependency at all: fsm.c includes only <stdint.h>/<stdbool.h>,
# never a HAL header, and there is exactly one implementation for host,
# PIC16, and PIC18 alike (see the Makefiles under mcu/ for the real-target XC8
# builds, which compile this same src/fsm.c unchanged). Because of that,
# there is no EPIC_FAMILY selection needed for the core library or its
# tests. The module's example (examples/example_fsm.c) is target-only and
# builds through the manifest's example slot, not this host build.

cmake_minimum_required(VERSION 3.16)
project(epic_fsm C)

set(CMAKE_C_STANDARD 99)
set(CMAKE_C_STANDARD_REQUIRED ON)

add_library(fsm STATIC src/fsm.c)
target_include_directories(fsm PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/include)

# Unit tests: exercise the exact code that ships on-target (no per-family
# split means no reference-vs-shipped gap to worry about).
enable_testing()
add_executable(test_fsm tests/test_fsm.c)
target_link_libraries(test_fsm PRIVATE fsm)
add_test(NAME fsm_unit_tests COMMAND test_fsm)
