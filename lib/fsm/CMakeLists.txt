# lib/fsm, table-driven finite state machine engine, no HAL dependency.
#
#   cmake -B build && cmake --build build
#   ./build/test_fsm
#
# One implementation for host, PIC16, and PIC18 (only stdint/stdbool),
# so the core needs no EPIC_FAMILY selection. The target-only example
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
