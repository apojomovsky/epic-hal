# PIC16F5x HAL, host simulation build (cmake -B build, build, run).
# Real-target builds use XC8 via the manifest. Thin caller of
# epic-common/cmake/epic_family.cmake; only the source list and the
# device list are PIC16F5x-family-specific. The 12-bit baseline core:
# GPIO + polled Timer0 + the no-op IRQ stub + WDT/sleep; no shared
# pic14-midrange-core sources (different architecture).

cmake_minimum_required(VERSION 3.16)
project(pic16f5x_hal C)

set(CMAKE_C_STANDARD 99)
set(CMAKE_C_STANDARD_REQUIRED ON)

# Shared layer: status codes, harness, CMake helpers.
set(EPIC_COMMON_DIR ${CMAKE_CURRENT_SOURCE_DIR}/../epic-common)
include(${EPIC_COMMON_DIR}/cmake/epic_family.cmake)

# Default device (the canonical part; -DPIC16F5X_DEVICE=<part> picks
# another). Per-example device selection goes through EPIC_FAMILY_DEVICES.
if(NOT DEFINED PIC16F5X_DEVICE)
    set(PIC16F5X_DEVICE "PIC16F54")
endif()

set(PIC16F5X_DEFAULT_DEVICE PIC16F54)

# Variables consumed by the shared epic_family.cmake helpers.
set(EPIC_LIB              pic16f5x_hal)
set(EPIC_DEFAULT_DEVICE       ${PIC16F5X_DEFAULT_DEVICE})
set(EPIC_FAMILY_INCLUDE_DIR    ${CMAKE_CURRENT_SOURCE_DIR}/include)
set(EPIC_HOST_INCLUDE_DIR      ${CMAKE_CURRENT_SOURCE_DIR}/include/host)
set(EPIC_COMMON_INCLUDE_DIR    ${EPIC_COMMON_DIR}/include)
set(EPIC_FAMILY_DEVICES        PIC16F54 PIC16F57 PIC16F59 PIC16F505 PIC16F506)

set(EPIC_SOURCES
    src/peripherals/pic16f5x_gpio.c
    src/peripherals/pic16f5x_timer0.c
    src/core/pic16_irq_stub.c
    src/core/pic16_irq_dispatch.c
    src/sim/pic16f5x_wdt_sleep_sim.c
    src/sim/pic16_harness_sim.c
    src/sim/pic16f5x_sim.c
)

epic_add_hal_library(${EPIC_LIB})

epic_add_example(example_blink       tests/example_blink.c)
epic_add_example(example_control     tests/sim_control_probe.c)

# Optional: example for every device in the family (proves every part
# compiles and links against the canonical sources).
epic_add_example_per_device(example_blink tests/example_blink.c)
