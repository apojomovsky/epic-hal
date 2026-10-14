# lib/debounce, digital-input debouncer on a lib/tick timebase.
#
#   cmake -B build && cmake --build build
#   ctest --test-dir build --output-on-failure
#
# lib/tick pulls a HAL family for its Timer2 host-sim backend.
# -DEPIC_FAMILY=PIC18 selects the family (default PIC16). debounce.h
# is dependency-free; only src/debounce.c includes epic_tick.h.

cmake_minimum_required(VERSION 3.16)
project(epic_debounce C)

set(CMAKE_C_STANDARD 99)
set(CMAKE_C_STANDARD_REQUIRED ON)

# Root anchor: walk up to the shared helpers, so moving this directory
# changes no path below. Cross-module paths hang off EPIC_ROOT.
if(NOT DEFINED EPIC_ROOT)
    set(_EPIC_PROBE "${CMAKE_CURRENT_SOURCE_DIR}")
    while(NOT EXISTS "${_EPIC_PROBE}/common/cmake/epic_family.cmake")
        get_filename_component(_EPIC_PARENT "${_EPIC_PROBE}/.." ABSOLUTE)
        if("${_EPIC_PARENT}" STREQUAL "${_EPIC_PROBE}")
            message(FATAL_ERROR "EPIC_ROOT not found above ${CMAKE_CURRENT_SOURCE_DIR}")
        endif()
        set(_EPIC_PROBE "${_EPIC_PARENT}")
    endwhile()
    set(EPIC_ROOT "${_EPIC_PROBE}")
    unset(_EPIC_PROBE)
    unset(_EPIC_PARENT)
endif()

set(EPIC_FAMILY "PIC16" CACHE STRING "HAL family for the lib/tick timebase: PIC16 or PIC18")

# Pull in lib/tick (which add_subdirectory's the chosen HAL family).
set(TICK_DIR ${EPIC_ROOT}/lib/tick
    CACHE PATH "Path to the lib/tick source tree")
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
