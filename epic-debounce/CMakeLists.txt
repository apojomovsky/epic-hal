# epic-debounce, vendor-agnostic digital-input debouncer.
#
#   cmake -B build && cmake --build build
#   ctest --test-dir build --output-on-failure
#   ./build/test_debounce
#
# The debounce core (src/debounce.c) depends on epic-tick for its timebase
# (epic_tick_get / epic_tick_elapsed_since). epic-tick in turn pulls a HAL
# family for its Timer2 host-sim backend. Select the family with
# -DEPIC_FAMILY=PIC18 (default PIC16), same convention as epic-fsm's optional
# taskmgr example. The public header debounce.h is dependency-free (only
# <stdint.h>/<stdbool.h>); only src/debounce.c includes epic_tick.h.

cmake_minimum_required(VERSION 3.16)
project(epic_debounce C)

set(CMAKE_C_STANDARD 99)
set(CMAKE_C_STANDARD_REQUIRED ON)

set(EPIC_FAMILY "PIC16" CACHE STRING "HAL family for the epic-tick timebase: PIC16 or PIC18")

# Pull in epic-tick (which add_subdirectory's the chosen HAL family).
set(TICK_DIR ${CMAKE_CURRENT_SOURCE_DIR}/../epic-tick
    CACHE PATH "Path to the epic-tick source tree")
if(NOT TARGET epic_tick)
    add_subdirectory(${TICK_DIR} ${CMAKE_CURRENT_BINARY_DIR}/tick)
endif()

add_library(debounce STATIC src/debounce.c)
target_include_directories(debounce PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/include)
target_link_libraries(debounce PUBLIC epic_tick)

# ---- tests: exercise the exact code that ships on-target ----
enable_testing()
add_executable(test_debounce tests/test_debounce.c)
target_link_libraries(test_debounce PRIVATE debounce)
add_test(NAME debounce_tests COMMAND test_debounce)
