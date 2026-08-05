# pic8-debounce, vendor-agnostic digital-input debouncer.
#
#   cmake -B build && cmake --build build
#   ctest --test-dir build --output-on-failure
#   ./build/test_debounce && ./build/example_debounce_hal
#
# The debounce core (src/debounce.c) depends on pic8-tick for its timebase
# (pic8_tick_get / pic8_tick_elapsed_since). pic8-tick in turn pulls a HAL
# family for its Timer2 host-sim backend. Select the family with
# -DEPIC_FAMILY=PIC18 (default PIC16), same convention as pic8-fsm's optional
# taskmgr example. The public header debounce.h is dependency-free (only
# <stdint.h>/<stdbool.h>); only src/debounce.c includes pic8_tick.h.

cmake_minimum_required(VERSION 3.16)
project(pic8_debounce C)

set(CMAKE_C_STANDARD 99)
set(CMAKE_C_STANDARD_REQUIRED ON)

set(EPIC_FAMILY "PIC16" CACHE STRING "HAL family for the pic8-tick timebase: PIC16 or PIC18")

# Pull in pic8-tick (which add_subdirectory's the chosen HAL family).
set(TICK_DIR ${CMAKE_CURRENT_SOURCE_DIR}/../pic8-tick
    CACHE PATH "Path to the pic8-tick source tree")
if(NOT TARGET pic8_tick)
    add_subdirectory(${TICK_DIR} ${CMAKE_CURRENT_BINARY_DIR}/tick)
endif()

add_library(debounce STATIC src/debounce.c)
target_include_directories(debounce PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/include)
target_link_libraries(debounce PUBLIC pic8_tick)

# ---- tests: exercise the exact code that ships on-target ----
enable_testing()
add_executable(test_debounce tests/test_debounce.c)
target_link_libraries(test_debounce PRIVATE debounce)
add_test(NAME debounce_tests COMMAND test_debounce)

# ---- example: two instances on HAL GPIO pins, host-sim runnable ----
add_executable(example_debounce_hal examples/example_debounce_hal.c)
target_link_libraries(example_debounce_hal PRIVATE debounce)
