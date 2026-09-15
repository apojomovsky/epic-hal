# PIC18F1320 HAL, host simulation build.
#   cmake -B build -S pic18f1320-hal && cmake --build build
#   ./build/example_smoke
#
# Real-target builds are manifest-driven (scripts/epic_build.py), not a
# standalone Makefile. Thin caller of the shared epic_family.cmake
# helpers; only the source and device lists are family-specific.

cmake_minimum_required(VERSION 3.16)
project(pic18f1320_hal C)

set(CMAKE_C_STANDARD 99)
set(CMAKE_C_STANDARD_REQUIRED ON)

# Shared layer: status codes, harness, CMake helpers.
set(EPIC_COMMON_DIR ${CMAKE_CURRENT_SOURCE_DIR}/../epic-common)
include(${EPIC_COMMON_DIR}/cmake/epic_family.cmake)

# One device in this HAL: PIC18F1320 (18-pin, family B's exemplar part).
set(PIC18F1320_DEFAULT_DEVICE PIC18F1320)

# Variables consumed by the shared epic_family.cmake helpers.
set(EPIC_LIB                   pic18f1320_hal)
set(EPIC_DEFAULT_DEVICE        ${PIC18F1320_DEFAULT_DEVICE})
set(EPIC_FAMILY_INCLUDE_DIR    ${CMAKE_CURRENT_SOURCE_DIR}/include)
set(EPIC_HOST_INCLUDE_DIR      ${CMAKE_CURRENT_SOURCE_DIR}/include/host)
set(EPIC_COMMON_INCLUDE_DIR    ${EPIC_COMMON_DIR}/include)
set(EPIC_FAMILY_DEVICES        PIC18F1320)

set(EPIC_SOURCES
    src/sim/pic18_sim.c
    src/sim/pic18_harness_sim.c
    src/core/pic18_irq.c
    src/core/pic18_irq_dispatch.c
    src/core/pic18f1320_wdt_sleep.c
    src/sim/pic18f1320_wdt_sleep_sim.c
    src/peripherals/pic18f1320_gpio.c
    src/peripherals/pic18f1320_timer0.c
)

epic_add_hal_library(${EPIC_LIB})

# Foundation examples: example_blink is the canonical Timer0 + GPIO +
# interrupt smoke test; example_timer0_irq is the IRQ backend's own
# dedicated smoke (also the real-mdb gate, MODE=toggle via ci-target-sim.sh);
# example_smoke is the bare harness-seam test.
epic_add_example(example_blink tests/example_blink.c)
epic_add_example(example_timer0_irq tests/example_timer0_irq.c)
epic_add_example(example_smoke tests/example_smoke.c)

# Optional: the blink example for every device in the family (one, here),
# proving it compiles and links against the shared contract.
epic_add_example_per_device(example_blink tests/example_blink.c)
