# PIC16F193X HAL, host simulation build.
#
#   cmake -B build && cmake --build build
#   ./build/example_blink
#
# Real PIC target builds use the XC8 toolchain. The MPLAB X project
# lives at mcu/pic16f193x-mplabx/; its Makefile produces a .hex via xc8-cc.
# The real-target build needs the Microchip.PIC12-16F1xxx_DFP (not
# bundled with XC8, not installed locally), so it is run once that pack
# is present; the host build here has no such dependency.
#
# The host vs. target split is done at build time, not with #ifdef: this
# build puts include/host ahead of include on the include path, so
# pic16f193x_platform.h resolves to the memory-backed-SFR version, and
# links the host-side harness/wdt_sleep implementations. The XC8 Makefile
# does the mirror with include/target and the target implementations.
#
# This file is a thin caller of the shared pic8_family.cmake helpers
# (pic8-common/cmake/). Everything identical between families, the
# host-include-first convention, the per-target device define, the
# example/link boilerplate, the per-device example loop, lives there;
# only what is genuinely PIC16F193X-specific stays here: the source list,
# the device list, and the PORTD/PORTE gate.

cmake_minimum_required(VERSION 3.16)
project(pic16f193x_hal C)

set(CMAKE_C_STANDARD 99)
set(CMAKE_C_STANDARD_REQUIRED ON)

# Shared layer: status codes, harness, CMake helpers.
set(PIC8_COMMON_DIR ${CMAKE_CURRENT_SOURCE_DIR}/../pic8-common)
include(${PIC8_COMMON_DIR}/cmake/pic8_family.cmake)

# Default device, can be overridden with -DPIC16F193X_DEVICE=1936.
if(NOT DEFINED PIC16F193X_DEVICE)
    set(PIC16F193X_DEVICE "PIC16F1937")
endif()

# The device-select macro is set per-target so each example can pick its
# own part without polluting other targets.
set(PIC16F193X_DEFAULT_DEVICE PIC16F1937)

# PORTD/PORTE are 40/44-pin only (PIC16F1934/1937/1939).
if(PIC16F193X_DEFAULT_DEVICE STREQUAL "PIC16F1934" OR
   PIC16F193X_DEFAULT_DEVICE STREQUAL "PIC16F1937" OR
   PIC16F193X_DEFAULT_DEVICE STREQUAL "PIC16F1939")
    set(PIC16F193X_FAMILY_HAS_PORTD 1)
    set(PIC16F193X_FAMILY_HAS_PORTE 1)
else()
    set(PIC16F193X_FAMILY_HAS_PORTD 0)
    set(PIC16F193X_FAMILY_HAS_PORTE 0)
endif()

# Variables consumed by the shared pic8_family.cmake helpers.
set(PIC8_HAL_LIB              pic16f193x_hal)
set(PIC8_DEFAULT_DEVICE       ${PIC16F193X_DEFAULT_DEVICE})
set(PIC8_FAMILY_INCLUDE_DIR   ${CMAKE_CURRENT_SOURCE_DIR}/include)
set(PIC8_HOST_INCLUDE_DIR     ${CMAKE_CURRENT_SOURCE_DIR}/include/host)
set(PIC8_COMMON_INCLUDE_DIR   ${PIC8_COMMON_DIR}/include)
set(PIC8_FAMILY_DEVICES       PIC16F1933 PIC16F1934 PIC16F1936 PIC16F1937 PIC16F1938 PIC16F1939)

set(PIC8_HAL_SOURCES
    src/peripherals/pic16f193x_gpio.c
    src/peripherals/pic16f193x_timer0.c
    src/core/pic16f193x_irq.c
    src/core/pic16f193x_wdt_sleep.c
    src/core/pic16f193x_wdt_sleep_sim.c
    src/core/pic16f193x_irq_dispatch.c
    src/core/pic16f193x_harness_sim.c
    src/sim/pic16f193x_sim.c
)

pic8_add_hal_library(${PIC8_HAL_LIB})

pic8_add_example(example_blink tests/example_blink.c)
pic8_add_example(example_gpio tests/example_gpio.c)

# Optional: example for every device in the family.
pic8_add_example_per_device(example_blink tests/example_blink.c)
