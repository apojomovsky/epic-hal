# epic-modbus, a Modbus RTU slave for 8-bit PICs.
#
#   cmake -B build && cmake --build build
#
# Builds on two sibling epic-* modules: epic-serial (the UART) and epic-tick
# (the T3.5 silence timebase), the same sibling-link pattern epic-debounce
# uses for epic-tick alone. epic-tick in turn pulls in a HAL family for its
# Timer2 host-sim backend; epic-serial links the same family's USART. The
# target-only example (examples/example_modbus.c) is compiled by the
# manifest's xc8 matrix, and the framing/T3.5 test runs under MPLAB SIM
# (tests/sim_modbus.c, via `make mdb-test MODULE=epic-modbus MCU=18F4550
# DEVICE=PIC18F4550`). Select the family with -DEPIC_FAMILY=PIC18 (default
# PIC16).

cmake_minimum_required(VERSION 3.16)
project(epic_modbus C)

set(CMAKE_C_STANDARD 99)
set(CMAKE_C_STANDARD_REQUIRED ON)

set(EPIC_FAMILY "PIC16" CACHE STRING "HAL family to link: PIC16 or PIC18")

if(EPIC_FAMILY STREQUAL "PIC18")
    set(DEFAULT_DEVICE   PIC18F4550)
    set(DEFAULT_FOSC_HZ  48000000UL)
elseif(EPIC_FAMILY STREQUAL "PIC16F88X")
    set(DEFAULT_DEVICE   PIC16F887)
    set(DEFAULT_FOSC_HZ  20000000UL)
else()
    set(DEFAULT_DEVICE   PIC16F877A)
    set(DEFAULT_FOSC_HZ  20000000UL)
endif()

# Pull in epic-serial and epic-tick (each add_subdirectory's the chosen HAL
# family; guard with NOT TARGET so either can be pulled in first).
set(SERIAL_DIR ${CMAKE_CURRENT_SOURCE_DIR}/../epic-serial
    CACHE PATH "Path to the epic-serial source tree")
set(TICK_DIR ${CMAKE_CURRENT_SOURCE_DIR}/../epic-tick
    CACHE PATH "Path to the epic-tick source tree")

if(NOT TARGET epic_serial)
    add_subdirectory(${SERIAL_DIR} ${CMAKE_CURRENT_BINARY_DIR}/serial)
endif()
if(NOT TARGET epic_tick)
    add_subdirectory(${TICK_DIR} ${CMAKE_CURRENT_BINARY_DIR}/tick)
endif()

add_library(epic_modbus STATIC src/epic_modbus.c)
target_include_directories(epic_modbus PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/include)
target_link_libraries(epic_modbus PUBLIC epic_serial epic_tick)
