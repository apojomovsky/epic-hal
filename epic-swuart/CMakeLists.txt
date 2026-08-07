# epic-swuart, bit-banged full-duplex UART, any GPIO pin, two channels
# at once. Zero per-family branching: every HAL call this module makes
# (EPIC_GPIO_*, EPIC_TIMER1_*, EPIC_IRQ_Restore) is already identical
# across all three families, so src/epic_swuart.c is one file. Select
# the family with -DEPIC_FAMILY=PIC16, PIC18, or PIC16F193X (default
# PIC16). Real-target builds use the manifest (epic_build.py), not a
# Makefile under mcu/: this module was never a per-module mcu/ tree.

cmake_minimum_required(VERSION 3.16)
project(epic_swuart C)

set(CMAKE_C_STANDARD 99)
set(CMAKE_C_STANDARD_REQUIRED ON)

set(EPIC_FAMILY "PIC16" CACHE STRING "HAL family to link: PIC16, PIC18, or PIC16F193X")

if(EPIC_FAMILY STREQUAL "PIC18")
    set(EPIC_DIR ${CMAKE_CURRENT_SOURCE_DIR}/../pic18fxx5x-hal
        CACHE PATH "Path to the pic18fxx5x-hal source tree")
    set(EPIC_LIB          pic18fxx5x_hal)
    set(DEFAULT_DEVICE    PIC18F4550)
    set(DEFAULT_FOSC_HZ   48000000UL)
elseif(EPIC_FAMILY STREQUAL "PIC16F193X")
    set(EPIC_DIR ${CMAKE_CURRENT_SOURCE_DIR}/../pic16f193x-hal
        CACHE PATH "Path to the pic16f193x-hal source tree")
    set(EPIC_LIB          pic16f193x_hal)
    set(DEFAULT_DEVICE    PIC16F1937)
    set(DEFAULT_FOSC_HZ   32000000UL)
else()
    set(EPIC_DIR ${CMAKE_CURRENT_SOURCE_DIR}/../pic16f87xa-hal
        CACHE PATH "Path to the pic16f87xa-hal source tree")
    set(EPIC_LIB          pic16f87xa_hal)
    set(DEFAULT_DEVICE    PIC16F877A)
    set(DEFAULT_FOSC_HZ   20000000UL)
endif()

if(NOT TARGET ${EPIC_LIB})
    add_subdirectory(${EPIC_DIR} ${CMAKE_CURRENT_BINARY_DIR}/hal)
endif()

add_library(epic_swuart STATIC src/epic_swuart.c)
target_include_directories(epic_swuart PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/include)
target_link_libraries(epic_swuart PUBLIC ${EPIC_LIB})
target_compile_definitions(epic_swuart PRIVATE -D${DEFAULT_DEVICE})

enable_testing()
add_executable(test_swuart_tx tests/test_swuart_tx.c)
target_compile_definitions(test_swuart_tx PRIVATE -D${DEFAULT_DEVICE} -DFOSC_HZ=${DEFAULT_FOSC_HZ})
target_link_libraries(test_swuart_tx PRIVATE epic_swuart)
add_test(NAME swuart_tx COMMAND test_swuart_tx)

add_executable(test_swuart_rx tests/test_swuart_rx.c)
target_compile_definitions(test_swuart_rx PRIVATE -D${DEFAULT_DEVICE} -DFOSC_HZ=${DEFAULT_FOSC_HZ})
target_link_libraries(test_swuart_rx PRIVATE epic_swuart)
add_test(NAME swuart_rx COMMAND test_swuart_rx)

add_executable(test_swuart_dual tests/test_swuart_dual.c)
target_compile_definitions(test_swuart_dual PRIVATE -D${DEFAULT_DEVICE} -DFOSC_HZ=${DEFAULT_FOSC_HZ})
target_link_libraries(test_swuart_dual PRIVATE epic_swuart)
add_test(NAME swuart_dual COMMAND test_swuart_dual)

add_executable(test_swuart_errors tests/test_swuart_errors.c)
target_compile_definitions(test_swuart_errors PRIVATE -D${DEFAULT_DEVICE} -DFOSC_HZ=${DEFAULT_FOSC_HZ})
target_link_libraries(test_swuart_errors PRIVATE epic_swuart)
add_test(NAME swuart_errors COMMAND test_swuart_errors)

add_executable(test_swuart_deinit tests/test_swuart_deinit.c)
target_compile_definitions(test_swuart_deinit PRIVATE -D${DEFAULT_DEVICE} -DFOSC_HZ=${DEFAULT_FOSC_HZ})
target_link_libraries(test_swuart_deinit PRIVATE epic_swuart)
add_test(NAME swuart_deinit COMMAND test_swuart_deinit)

add_executable(test_swuart_dual_deinit tests/test_swuart_dual_deinit.c)
target_compile_definitions(test_swuart_dual_deinit PRIVATE -D${DEFAULT_DEVICE} -DFOSC_HZ=${DEFAULT_FOSC_HZ})
target_link_libraries(test_swuart_dual_deinit PRIVATE epic_swuart)
add_test(NAME swuart_dual_deinit COMMAND test_swuart_dual_deinit)
