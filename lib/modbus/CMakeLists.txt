# lib/modbus, a Modbus RTU slave for 8-bit PICs.
#
#   cmake -B build && cmake --build build
#
# Sibling links: lib/serial (UART) and lib/tick (T3.5 timebase), which
# pull the HAL family for USART/Timer2 backends. Framing test under
# MPLAB SIM (tests/sim_modbus.c); example via manifest matrix.
# -DEPIC_FAMILY=PIC18 selects the family (default PIC16).

cmake_minimum_required(VERSION 3.16)
project(epic_modbus C)

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
    set(DEFAULT_DEVICE   PIC18F4550)
    set(DEFAULT_FOSC_HZ  48000000UL)
elseif(EPIC_FAMILY STREQUAL "PIC16F88X")
    set(DEFAULT_DEVICE   PIC16F887)
    set(DEFAULT_FOSC_HZ  20000000UL)
else()
    set(DEFAULT_DEVICE   PIC16F877A)
    set(DEFAULT_FOSC_HZ  20000000UL)
endif()

# Pull in lib/serial and lib/tick (each add_subdirectory's the chosen HAL
# family; guard with NOT TARGET so either can be pulled in first).
set(SERIAL_DIR ${EPIC_ROOT}/lib/serial
    CACHE PATH "Path to the lib/serial source tree")
set(TICK_DIR ${EPIC_ROOT}/lib/tick
    CACHE PATH "Path to the lib/tick source tree")

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

# Host test: RTU framing, CRC and dispatch through the real slave core
# over the host-sim USART model (RX injection, host tick for T3.5).
enable_testing()
add_executable(test_modbus_host tests/test_modbus_host.c)
target_compile_definitions(test_modbus_host PRIVATE -D${DEFAULT_DEVICE} -DFOSC_HZ=${DEFAULT_FOSC_HZ})
target_link_libraries(test_modbus_host PRIVATE epic_modbus)
add_test(NAME modbus_host COMMAND test_modbus_host)
