# lib/tick, a 1 ms timebase for 8-bit PICs.
#
#   cmake -B build && cmake --build build
#
# Family-agnostic (neutral contracts plus epic_tick.h) against any
# family's host sim backend via add_subdirectory. -DEPIC_FAMILY=PIC18
# selects the family (default PIC16); real-target builds use mcu/.

cmake_minimum_required(VERSION 3.16)
project(epic_tick C)

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

set(EPIC_FAMILY "PIC16" CACHE STRING "HAL family to link: PIC16 or PIC18")

if(EPIC_FAMILY STREQUAL "PIC18")
    set(EPIC_DIR ${EPIC_ROOT}/hal/pic18/18fxx5x
        CACHE PATH "Path to the hal/pic18/18fxx5x source tree")
    set(EPIC_LIB          pic18fxx5x_hal)
    set(FAMILY_DEVICES   PIC18F2455 PIC18F2550 PIC18F4455 PIC18F4550)
    set(DEFAULT_DEVICE   PIC18F4550)
    set(DEFAULT_FOSC_HZ  48000000UL)
elseif(EPIC_FAMILY STREQUAL "PIC16F88X")
    set(EPIC_DIR ${EPIC_ROOT}/hal/pic14/16f88x
        CACHE PATH "Path to the hal/pic14/16f88x source tree")
    set(EPIC_LIB          pic16f88x_hal)
    set(FAMILY_DEVICES   PIC16F882 PIC16F883 PIC16F884 PIC16F886 PIC16F887)
    set(DEFAULT_DEVICE   PIC16F887)
    set(DEFAULT_FOSC_HZ  20000000UL)
else()
    set(EPIC_DIR ${EPIC_ROOT}/hal/pic14/16f87xa
        CACHE PATH "Path to the hal/pic14/16f87xa source tree")
    set(EPIC_LIB          pic16f87xa_hal)
    set(FAMILY_DEVICES   PIC16F873A PIC16F874A PIC16F876A PIC16F877A)
    set(DEFAULT_DEVICE   PIC16F877A)
    set(DEFAULT_FOSC_HZ  20000000UL)
endif()

if(NOT TARGET ${EPIC_LIB})
    add_subdirectory(${EPIC_DIR} ${CMAKE_CURRENT_BINARY_DIR}/hal)
endif()

add_library(epic_tick STATIC src/epic_tick.c)
target_include_directories(epic_tick PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/include)
target_link_libraries(epic_tick PUBLIC ${EPIC_LIB})
target_compile_definitions(epic_tick PRIVATE -D${DEFAULT_DEVICE})

# The target-only example: compiled here as a host syntax/link gate for
# the default device. It is a real-silicon program (an LED blink, no
# stdout) so it is not run as a test.
add_executable(example_tick examples/example_tick.c)
target_compile_definitions(example_tick PRIVATE -D${DEFAULT_DEVICE} -DFOSC_HZ=${DEFAULT_FOSC_HZ})
target_link_libraries(example_tick PRIVATE epic_tick)
