# epic-bus, I2C/SPI "MEM" register-access idiom (HAL_I2C_Mem_Read/Write equiv).
#
#   cmake -B build && cmake --build build
#
# Reuses a HAL family's host sim backend via add_subdirectory (as the other
# epic-* modules do). The MEM transaction logic is family-agnostic; the
# default ops wrap the HAL SSP driver (target), and the mock-MEM test now
# lives under MPLAB SIM (tests/sim_bus.c, run via `make mdb-test MODULE=
# epic-bus MCU=16F877A DEVICE=PIC16F877A`). The target-only example
# (examples/example_bus.c) is compiled by the manifest's xc8 matrix, not
# by this host CMake. Select the family with -DEPIC_FAMILY=PIC18 (default
# PIC16).

cmake_minimum_required(VERSION 3.16)
project(epic_bus C)

set(CMAKE_C_STANDARD 99)
set(CMAKE_C_STANDARD_REQUIRED ON)

set(EPIC_FAMILY "PIC16" CACHE STRING "HAL family to link: PIC16 or PIC18")

if(EPIC_FAMILY STREQUAL "PIC18")
    set(EPIC_DIR ${CMAKE_CURRENT_SOURCE_DIR}/../pic18fxx5x-hal
        CACHE PATH "Path to the pic18fxx5x-hal source tree")
    set(EPIC_LIB          pic18fxx5x_hal)
    set(FAMILY_DEVICES   PIC18F2455 PIC18F2550 PIC18F4455 PIC18F4550)
    set(DEFAULT_DEVICE   PIC18F4550)
    set(DEFAULT_FOSC_HZ  48000000UL)
else()
    set(EPIC_DIR ${CMAKE_CURRENT_SOURCE_DIR}/../pic16f87xa-hal
        CACHE PATH "Path to the pic16f87xa-hal source tree")
    set(EPIC_LIB          pic16f87xa_hal)
    set(FAMILY_DEVICES   PIC16F873A PIC16F874A PIC16F876A PIC16F877A)
    set(DEFAULT_DEVICE   PIC16F877A)
    set(DEFAULT_FOSC_HZ  20000000UL)
endif()

if(NOT TARGET ${EPIC_LIB})
    add_subdirectory(${EPIC_DIR} ${CMAKE_CURRENT_BINARY_DIR}/hal)
endif()

add_library(epic_bus STATIC src/epic_bus.c)
target_include_directories(epic_bus PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/include)
target_link_libraries(epic_bus PUBLIC ${EPIC_LIB})
target_compile_definitions(epic_bus PRIVATE -D${DEFAULT_DEVICE})
