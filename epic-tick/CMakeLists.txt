# epic-tick, a 1 ms timebase (HAL_GetTick/HAL_Delay equivalent) for 8-bit PICs.
#
#   cmake -B build && cmake --build build
#   ctest --test-dir build --output-on-failure
#   ./build/example_tick
#
# This build reuses the host simulation backend of a HAL family (pulled in via
# add_subdirectory) exactly as epic-taskmgr does. The tick module is
# family-agnostic: it includes only the neutral contracts
# (peripherals/hal_timer2.h, core/hal_irq.h, core/epic_harness.h) and the
# public epic_tick.h, so the same src/epic_tick.c builds against any family.
# Select the family with -DEPIC_FAMILY=PIC18 (default PIC16). Real-target
# builds use the XC8 Makefiles under mcu/.

cmake_minimum_required(VERSION 3.16)
project(epic_tick C)

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

add_library(epic_tick STATIC src/epic_tick.c)
target_include_directories(epic_tick PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/include)
target_link_libraries(epic_tick PUBLIC ${EPIC_LIB})
target_compile_definitions(epic_tick PRIVATE -D${DEFAULT_DEVICE})

# The example doubles as the host smoke test.
add_executable(example_tick examples/example_tick.c)
target_compile_definitions(example_tick PRIVATE -D${DEFAULT_DEVICE} -DFOSC_HZ=${DEFAULT_FOSC_HZ})
target_link_libraries(example_tick PRIVATE epic_tick)

enable_testing()
add_test(NAME example_tick COMMAND example_tick)
