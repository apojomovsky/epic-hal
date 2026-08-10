# epic-mcp23x17, MCP23017 (I2C) / MCP23S17 (SPI) 16-bit I/O expander
# driver, on top of epic-bus.
#
#   cmake -B build && cmake --build build
#   ctest --test-dir build --output-on-failure
#   ./build/test_mcp23x17 && ./build/example_mcp23x17
#
# Reuses a HAL family's host sim backend via add_subdirectory (as the
# other epic-* modules do) and epic-bus for the transport. The driver
# itself is family-agnostic; the host test injects a mock MCP23017
# device through the epic-bus ops seam (the host sim has no SSP slave
# model), so the full module + epic-bus + mock-device stack runs
# host-side. Select the family with -DEPIC_FAMILY=PIC18 (default
# PIC16). Real-target builds use the XC8 Makefiles under mcu/.

cmake_minimum_required(VERSION 3.16)
project(epic_mcp23x17 C)

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

if(NOT TARGET epic_bus)
    add_subdirectory(${CMAKE_CURRENT_SOURCE_DIR}/../epic-bus
        ${CMAKE_CURRENT_BINARY_DIR}/bus)
endif()

add_library(epic_mcp23x17 STATIC src/epic_mcp23x17.c)
target_include_directories(epic_mcp23x17 PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/include)
target_link_libraries(epic_mcp23x17 PUBLIC epic_bus)
target_compile_definitions(epic_mcp23x17 PRIVATE -D${DEFAULT_DEVICE})

enable_testing()

add_executable(test_mcp23x17 tests/test_mcp23x17.c)
target_compile_definitions(test_mcp23x17 PRIVATE -D${DEFAULT_DEVICE} -DFOSC_HZ=${DEFAULT_FOSC_HZ})
target_link_libraries(test_mcp23x17 PRIVATE epic_mcp23x17)
add_test(NAME mcp23x17_host COMMAND test_mcp23x17)

add_executable(example_mcp23x17 examples/example_mcp23x17.c)
target_compile_definitions(example_mcp23x17 PRIVATE -D${DEFAULT_DEVICE} -DFOSC_HZ=${DEFAULT_FOSC_HZ})
target_link_libraries(example_mcp23x17 PRIVATE epic_mcp23x17)
add_test(NAME mcp23x17_example COMMAND example_mcp23x17)
