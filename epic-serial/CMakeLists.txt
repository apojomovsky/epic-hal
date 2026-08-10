# epic-serial, interrupt-driven ring-buffered UART + printf retarget.
#
#   cmake -B build && cmake --build build
#   ctest --test-dir build --output-on-failure
#   ./build/example_serial
#
# Reuses a HAL family's host sim backend via add_subdirectory (as epic-taskmgr
# / epic-tick do). The serial module is family-agnostic: it includes only the
# neutral contracts (epic_hal.h, core/epic_harness.h) and the public
# epic_serial.h, so the same src/epic_serial.c builds against any family.
# Select the family with -DEPIC_FAMILY=PIC18 (default PIC16). Real-target
# builds use the XC8 Makefiles under mcu/.

cmake_minimum_required(VERSION 3.16)
project(epic_serial C)

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

add_library(epic_serial STATIC src/epic_serial.c)
target_include_directories(epic_serial PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/include)
target_link_libraries(epic_serial PUBLIC ${EPIC_LIB})
target_compile_definitions(epic_serial PRIVATE -D${DEFAULT_DEVICE})

add_executable(example_serial examples/example_serial.c)
target_compile_definitions(example_serial PRIVATE -D${DEFAULT_DEVICE} -DFOSC_HZ=${DEFAULT_FOSC_HZ})
target_link_libraries(example_serial PRIVATE epic_serial)

enable_testing()
add_test(NAME example_serial COMMAND example_serial)

# Host property test: randomized producer/consumer stress through the
# real public API over the sim's USART model, byte-exact against an
# independent model plus the full/empty ring boundaries.
add_executable(test_serial_stress tests/test_serial_stress.c)
target_compile_definitions(test_serial_stress PRIVATE -D${DEFAULT_DEVICE} -DFOSC_HZ=${DEFAULT_FOSC_HZ})
target_link_libraries(test_serial_stress PRIVATE epic_serial)
add_test(NAME serial_stress COMMAND test_serial_stress)
