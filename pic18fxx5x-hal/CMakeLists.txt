# PIC18F2455 family HAL, host simulation build.
#
#   cmake -B build -S pic18fxx5x-hal && cmake --build build
#   ./build/example_smoke
#
# Real PIC target builds use the XC8 toolchain. The MPLAB X project lives
# at mcu/pic18fxx5x-mplabx/; its Makefile produces a .hex via xc8-cc.
#
# The host vs. target split is done at build time, not with #ifdef: this
# build puts include/host ahead of include on the include path, so
# pic18_platform.h resolves to the memory-backed-SFR version, and links
# the host-side harness implementation. The XC8 Makefile does the mirror
# with include/target and the target implementations.
#
# This file is a thin caller of the shared epic_family.cmake helpers
# (epic-common/cmake/). Everything identical between families lives there;
# only what is genuinely PIC18F2455-specific stays here: the source list
# and the device list. (Phase 1: no peripheral sources yet, and no
# optional-peripheral gate; both grow in Phase 2.)

cmake_minimum_required(VERSION 3.16)
project(pic18fxx5x_hal C)

set(CMAKE_C_STANDARD 99)
set(CMAKE_C_STANDARD_REQUIRED ON)

# Shared layer: status codes, harness, CMake helpers.
set(PIC8_COMMON_DIR ${CMAKE_CURRENT_SOURCE_DIR}/../epic-common)
include(${PIC8_COMMON_DIR}/cmake/epic_family.cmake)

# Default device, can be overridden with -DPIC18FXX5X_DEVICE=... The default
# is PIC18F4550, the family's most-featured (40/44-pin) part.
if(NOT DEFINED PIC18FXX5X_DEVICE)
    set(PIC18FXX5X_DEVICE "PIC18F4550")
endif()

set(PIC18FXX5X_DEFAULT_DEVICE PIC18F4550)

# Determine whether the default device has the SPP (40/44-pin only). The
# CMake variable mirrors the C macro PIC18FXX5X_FAMILY_HAS_SPP so the SPP
# source and example can be gated here, the way pic16f87xa-hal gates PSP.
if(PIC18FXX5X_DEFAULT_DEVICE STREQUAL "PIC18F4455" OR
   PIC18FXX5X_DEFAULT_DEVICE STREQUAL "PIC18F4550")
    set(PIC18FXX5X_FAMILY_HAS_SPP 1)
else()
    set(PIC18FXX5X_FAMILY_HAS_SPP 0)
endif()

# Variables consumed by the shared epic_family.cmake helpers.
set(PIC8_EPIC_LIB              pic18fxx5x_hal)
set(PIC8_DEFAULT_DEVICE       ${PIC18FXX5X_DEFAULT_DEVICE})
set(PIC8_FAMILY_INCLUDE_DIR    ${CMAKE_CURRENT_SOURCE_DIR}/include)
set(PIC8_HOST_INCLUDE_DIR      ${CMAKE_CURRENT_SOURCE_DIR}/include/host)
set(PIC8_COMMON_INCLUDE_DIR    ${PIC8_COMMON_DIR}/include)
set(PIC8_FAMILY_DEVICES        PIC18F2455 PIC18F2550 PIC18F4455 PIC18F4550)

set(PIC8_EPIC_SOURCES
    src/sim/pic18_sim.c
    src/core/pic18_harness_sim.c
    src/core/pic18_irq.c
    src/core/pic18_irq_dispatch.c
    src/core/pic18fxx5x_wdt_sleep.c
    src/core/pic18fxx5x_wdt_sleep_sim.c
    src/peripherals/pic18fxx5x_gpio.c
    src/peripherals/pic18fxx5x_timer0.c
    src/peripherals/pic18fxx5x_timer1.c
    src/peripherals/pic18fxx5x_timer2.c
    src/peripherals/pic18fxx5x_timer3.c
    src/peripherals/pic18fxx5x_ccp.c
    src/peripherals/pic18fxx5x_ssp.c
    src/peripherals/pic18fxx5x_usart.c
    src/peripherals/pic18fxx5x_comp.c
    src/peripherals/pic18fxx5x_eeprom.c
    src/peripherals/pic18fxx5x_adc.c
)

# SPP is 40/44-pin only (4455/4550). The default device is PIC18F4550, which
# has it, so the library includes the SPP driver; 28-pin per-device examples
# just don't use it (and don't include the gated header).
if(PIC18FXX5X_FAMILY_HAS_SPP)
    list(APPEND PIC8_EPIC_SOURCES src/peripherals/pic18fxx5x_spp.c)
endif()

epic_add_hal_library(${PIC8_EPIC_LIB})

# Phase 2 MVP examples. example_blink is the canonical Timer0 + GPIO +
# interrupt smoke test; example_smoke remains the bare harness-seam test.
epic_add_example(example_blink  tests/example_blink.c)
epic_add_example(example_timer1 tests/example_timer1.c)
epic_add_example(example_timer2 tests/example_timer2.c)
epic_add_example(example_timer3 tests/example_timer3.c)
epic_add_example(example_ccp_pwm tests/example_ccp_pwm.c)
epic_add_example(example_ssp tests/example_ssp.c)
epic_add_example(example_usart tests/example_usart.c)
epic_add_example(example_comp tests/example_comp.c)
epic_add_example(example_eeprom tests/example_eeprom.c)
epic_add_example(example_adc tests/example_adc.c)
epic_add_example(example_rb_change tests/example_rb_change.c)
if(PIC18FXX5X_FAMILY_HAS_SPP)
    epic_add_example(example_spp tests/example_spp.c)
endif()
epic_add_example(example_smoke tests/example_smoke.c)

# Optional: the blink example for every device in the family, proving every
# part compiles and links against the shared contract.
epic_add_example_per_device(example_blink tests/example_blink.c)
