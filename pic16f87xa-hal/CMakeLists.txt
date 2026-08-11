# PIC16F87XA HAL, host simulation build.
#
#   cmake -B build && cmake --build build
#   ./build/example_blink
#
# Real PIC target builds use the XC8 toolchain; the manifest selects the
# real-target sources and the bundle's epicurus.mk (or the reference
# project examples/epicurus-demo-pic16f87xa.X) drives xc8-cc to a .hex.
#
# The host vs. target split is done at build time, not with #ifdef: this
# build puts include/host ahead of include on the include path, so
# pic16f87xa_platform.h resolves to the memory-backed-SFR version, and
# links the host-side harness/wdt_sleep implementations. The XC8 Makefile
# does the mirror with include/target and the target implementations.
#
# This file is a thin caller of the shared epic_family.cmake helpers
# (epic-common/cmake/). Everything identical between families, the
# host-include-first convention, the per-target device define, the
# example/link boilerplate, the per-device example loop — lives there;
# only what is genuinely PIC16F87XA-specific stays here: the source list,
# the device list, and the PSP gate.

cmake_minimum_required(VERSION 3.16)
project(pic16f87xa_hal C)

set(CMAKE_C_STANDARD 99)
set(CMAKE_C_STANDARD_REQUIRED ON)

# Shared layer: status codes, harness, CMake helpers.
set(EPIC_COMMON_DIR ${CMAKE_CURRENT_SOURCE_DIR}/../epic-common)
include(${EPIC_COMMON_DIR}/cmake/epic_family.cmake)

# Default device, can be overridden with -DPIC16F87XA_DEVICE=873A.
if(NOT DEFINED PIC16F87XA_DEVICE)
    set(PIC16F87XA_DEVICE "PIC16F877A")
endif()

# The device-select macro is set per-target so each example can pick its
# own part (PIC16F873A / 874A / 876A / 877A) without polluting other targets.
set(PIC16F87XA_DEFAULT_DEVICE PIC16F877A)

# Determine whether the default device has the PSP (40/44-pin only).
if(PIC16F87XA_DEFAULT_DEVICE STREQUAL "PIC16F874A" OR
   PIC16F87XA_DEFAULT_DEVICE STREQUAL "PIC16F877A")
    set(PIC16F87XA_FAMILY_HAS_PSP 1)
else()
    set(PIC16F87XA_FAMILY_HAS_PSP 0)
endif()

# Variables consumed by the shared epic_family.cmake helpers.
set(EPIC_LIB              pic16f87xa_hal)
set(EPIC_DEFAULT_DEVICE       ${PIC16F87XA_DEFAULT_DEVICE})
set(EPIC_FAMILY_INCLUDE_DIR    ${CMAKE_CURRENT_SOURCE_DIR}/include)
set(EPIC_HOST_INCLUDE_DIR      ${CMAKE_CURRENT_SOURCE_DIR}/include/host)
set(EPIC_COMMON_INCLUDE_DIR    ${EPIC_COMMON_DIR}/include)
set(EPIC_FAMILY_DEVICES        PIC16F873A PIC16F874A PIC16F876A PIC16F877A)

set(EPIC_SOURCES
    src/peripherals/pic16f87xa_gpio.c
    src/peripherals/pic16f87xa_timer0.c
    src/peripherals/pic16f87xa_timer1.c
    src/peripherals/pic16f87xa_timer2.c
    src/peripherals/pic16f87xa_ccp.c
    src/peripherals/pic16f87xa_usart.c
    src/peripherals/pic16f87xa_ssp.c
    src/peripherals/pic16f87xa_adc.c
    src/peripherals/pic16f87xa_comp.c
    src/peripherals/pic16f87xa_vref.c
    src/peripherals/pic16f87xa_eeprom.c
    src/core/pic16_irq.c
    src/core/pic16f87xa_wdt_sleep.c
    src/sim/pic16f87xa_wdt_sleep_sim.c
    src/core/pic16_irq_dispatch.c
    src/sim/pic16_harness_sim.c
    src/sim/pic16f87xa_sim.c
)

# PSP is only on 40/44-pin parts. PIC16F877A is the default.
if(PIC16F87XA_FAMILY_HAS_PSP)
    list(APPEND EPIC_SOURCES src/peripherals/pic16f87xa_psp.c)
endif()

epic_add_hal_library(${EPIC_LIB})

epic_add_example(example_blink       tests/example_blink.c)
epic_add_example(example_idle_blink  tests/example_idle_blink.c)
epic_add_example(example_timer1      tests/example_timer1.c)
epic_add_example(example_timer2      tests/example_timer2.c)
epic_add_example(example_ccp_pwm     tests/example_ccp_pwm.c)
epic_add_example(example_usart       tests/example_usart.c)
epic_add_example(example_ssp         tests/example_ssp.c)
epic_add_example(example_adc         tests/example_adc.c)
epic_add_example(example_comp_vref   tests/example_comp_vref.c)
epic_add_example(example_eeprom      tests/example_eeprom.c)
epic_add_example(example_wdt_sleep   tests/example_wdt_sleep.c)
epic_add_example(example_rb_change   tests/example_rb_change.c)
if(PIC16F87XA_FAMILY_HAS_PSP)
    epic_add_example(example_psp     tests/example_psp.c)
endif()

# Optional: example for every device in the family.
epic_add_example_per_device(example_blink tests/example_blink.c)
