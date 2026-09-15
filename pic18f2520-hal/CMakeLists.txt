# PIC18F2520 family HAL, host simulation build (foundation).
#
#   cmake -B build -S pic18f2520-hal && cmake --build build
#   ./build/example_smoke
#
# Real PIC target builds use the XC8 toolchain, driven by the manifest
# (epic-common/manifest/modules.toml); mcu/pic18f2520-mplabx/ documents
# that path. This CMake is the host-sim build only.
#
# The host vs. target split is done at build time, not with #ifdef: this
# build puts include/host ahead of include on the include path, so
# pic18_platform.h resolves to the memory-backed-SFR version, and links
# the host-side harness implementation. The XC8 build does the mirror
# with include/target and the target implementations.
#
# A thin caller of the shared epic_family.cmake helpers
# (epic-common/cmake/); only what is genuinely PIC18F2520-specific stays
# here: the source list and the device list.

cmake_minimum_required(VERSION 3.16)
project(pic18f2520_hal C)

set(CMAKE_C_STANDARD 99)
set(CMAKE_C_STANDARD_REQUIRED ON)

# Shared layer: status codes, harness, CMake helpers.
set(EPIC_COMMON_DIR ${CMAKE_CURRENT_SOURCE_DIR}/../epic-common)
include(${EPIC_COMMON_DIR}/cmake/epic_family.cmake)

# Default device. The PIC18F2520 is the only part in this family.
if(NOT DEFINED PIC18F2520_DEVICE)
    set(PIC18F2520_DEVICE "PIC18F2520")
endif()
set(PIC18F2520_DEFAULT_DEVICE PIC18F2520)

# Variables consumed by the shared epic_family.cmake helpers.
set(EPIC_LIB              pic18f2520_hal)
set(EPIC_DEFAULT_DEVICE       ${PIC18F2520_DEFAULT_DEVICE})
set(EPIC_FAMILY_INCLUDE_DIR    ${CMAKE_CURRENT_SOURCE_DIR}/include)
set(EPIC_HOST_INCLUDE_DIR      ${CMAKE_CURRENT_SOURCE_DIR}/include/host)
set(EPIC_COMMON_INCLUDE_DIR    ${EPIC_COMMON_DIR}/include)
set(EPIC_FAMILY_DEVICES        PIC18F2520)

# Foundation sources: GPIO + Timer0 drivers, the IRQ core and its
# dispatch, the WDT/Sleep helpers, and the host sim backend + harness.
# The dispatch strong-references only the handlers that exist this phase.
set(EPIC_SOURCES
    src/sim/pic18_sim.c
    src/sim/pic18_harness_sim.c
    src/core/pic18_irq.c
    src/core/pic18_irq_dispatch.c
    src/core/pic18f2520_wdt_sleep.c
    src/sim/pic18f2520_wdt_sleep_sim.c
    src/peripherals/pic18f2520_gpio.c
    src/peripherals/pic18f2520_timer0.c
)

epic_add_hal_library(${EPIC_LIB})

epic_add_example(example_blink tests/example_blink.c)
epic_add_example(example_irq   tests/example_irq.c)
epic_add_example(example_smoke tests/example_smoke.c)
