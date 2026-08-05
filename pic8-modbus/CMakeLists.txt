# pic8-modbus, a Modbus RTU slave for 8-bit PICs.
#
#   cmake -B build && cmake --build build
#   ctest --test-dir build --output-on-failure
#   ./build/example_modbus
#
# Builds on two sibling pic8-* modules: pic8-serial (the UART) and pic8-tick
# (the T3.5 silence timebase), the same sibling-link pattern pic8-debounce
# uses for pic8-tick alone. pic8-tick in turn pulls in a HAL family for its
# Timer2 host-sim backend; pic8-serial links the same family's USART. Select
# the family with -DEPIC_FAMILY=PIC18 (default PIC16). Real-target builds use
# the XC8 Makefiles under mcu/.

cmake_minimum_required(VERSION 3.16)
project(pic8_modbus C)

set(CMAKE_C_STANDARD 99)
set(CMAKE_C_STANDARD_REQUIRED ON)

set(EPIC_FAMILY "PIC16" CACHE STRING "HAL family to link: PIC16 or PIC18")

if(EPIC_FAMILY STREQUAL "PIC18")
    set(DEFAULT_DEVICE   PIC18F4550)
    set(DEFAULT_FOSC_HZ  48000000UL)
else()
    set(DEFAULT_DEVICE   PIC16F877A)
    set(DEFAULT_FOSC_HZ  20000000UL)
endif()

# Pull in pic8-serial and pic8-tick (each add_subdirectory's the chosen HAL
# family; guard with NOT TARGET so either can be pulled in first).
set(SERIAL_DIR ${CMAKE_CURRENT_SOURCE_DIR}/../pic8-serial
    CACHE PATH "Path to the pic8-serial source tree")
set(TICK_DIR ${CMAKE_CURRENT_SOURCE_DIR}/../pic8-tick
    CACHE PATH "Path to the pic8-tick source tree")

if(NOT TARGET pic8_serial)
    add_subdirectory(${SERIAL_DIR} ${CMAKE_CURRENT_BINARY_DIR}/serial)
endif()
if(NOT TARGET pic8_tick)
    add_subdirectory(${TICK_DIR} ${CMAKE_CURRENT_BINARY_DIR}/tick)
endif()

add_library(pic8_modbus STATIC src/pic8_modbus.c)
target_include_directories(pic8_modbus PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/include)
target_link_libraries(pic8_modbus PUBLIC pic8_serial pic8_tick)

# The example doubles as the host smoke test.
add_executable(example_modbus examples/example_modbus.c)
target_compile_definitions(example_modbus PRIVATE -D${DEFAULT_DEVICE} -DFOSC_HZ=${DEFAULT_FOSC_HZ})
target_link_libraries(example_modbus PRIVATE pic8_modbus)

enable_testing()
add_test(NAME example_modbus COMMAND example_modbus)
