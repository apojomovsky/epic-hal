# PIC16F83/84/84A HAL, host simulation build (cmake -B build, build, run).
# Real-target builds use XC8 via the manifest + the reference .X project.
# Thin caller of common/cmake/epic_family.cmake; only the source
# list and the device list are PIC16F83_84-family-specific.

cmake_minimum_required(VERSION 3.16)
project(pic16f83_84_hal C)

set(CMAKE_C_STANDARD 99)
set(CMAKE_C_STANDARD_REQUIRED ON)

# Shared layer: status codes, harness, CMake helpers.
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
include(${EPIC_ROOT}/common/cmake/epic_family.cmake)
set(EPIC_COMMON_DIR ${EPIC_ROOT}/common)

# Default device (the canonical part; -DPIC16F83_84_DEVICE=<part> picks
# another). Per-example device selection goes through EPIC_FAMILY_DEVICES.
if(NOT DEFINED PIC16F83_84_DEVICE)
    set(PIC16F83_84_DEVICE "PIC16F84A")
endif()

set(PIC16F83_84_DEFAULT_DEVICE PIC16F84A)

# Variables consumed by the shared epic_family.cmake helpers.
set(EPIC_LIB              pic16f83_84_hal)
set(EPIC_DEFAULT_DEVICE       ${PIC16F83_84_DEFAULT_DEVICE})
set(EPIC_FAMILY_INCLUDE_DIR    ${CMAKE_CURRENT_SOURCE_DIR}/include)
set(EPIC_CORE_INCLUDE_DIR      ${EPIC_ROOT}/hal/pic14/core/include)
set(EPIC_HOST_INCLUDE_DIR      ${CMAKE_CURRENT_SOURCE_DIR}/include/host)
set(EPIC_COMMON_INCLUDE_DIR    ${EPIC_COMMON_DIR}/include)
set(EPIC_FAMILY_DEVICES        PIC16F83 PIC16F84 PIC16F84A)

set(EPIC_SOURCES
    ${EPIC_ROOT}/hal/pic14/core/src/peripherals/pic14_gpio.c
    ${EPIC_ROOT}/hal/pic14/core/src/peripherals/pic14_timer0.c
    ${EPIC_ROOT}/hal/pic14/core/src/peripherals/pic14_eeprom.c
    ${EPIC_ROOT}/hal/pic14/core/src/core/pic14_irq.c
    src/core/pic16_irq_table.c
    ${EPIC_ROOT}/hal/pic14/core/src/core/pic14_wdt_sleep.c
    ${EPIC_ROOT}/hal/pic14/core/src/sim/pic14_wdt_sleep_sim.c
    ${EPIC_ROOT}/hal/pic14/core/src/core/pic14_irq_dispatch.c
    src/sim/pic16_harness_sim.c
    src/sim/pic16f83_84_sim.c
)

epic_add_hal_library(${EPIC_LIB})

epic_add_example(example_blink       tests/example_blink.c)
epic_add_example(example_eeprom      tests/example_eeprom.c)
epic_add_example(example_wdt_sleep   tests/example_wdt_sleep.c)
epic_add_example(example_rb_change   tests/example_rb_change.c)

# Optional: example for every device in the family.
epic_add_example_per_device(example_blink tests/example_blink.c)
