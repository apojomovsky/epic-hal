# PIC18F2520 host-sim build (foundation): cmake -B build, build, run
# ./build/example_smoke. Real-target XC8 builds come from the manifest
# (epic-common/manifest/modules.toml); mcu/pic18f2520-mplabx/ covers that
# path. include/host leads the include path here (memory-backed SFRs);
# the XC8 build mirrors with include/target. Thin caller of
# epic-common/cmake helpers; only the source/device lists are local.

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

# Foundation + Timer1 sources: GPIO + Timer0/1 drivers, the IRQ core and its
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
    src/peripherals/pic18f2520_timer1.c
    src/peripherals/pic18f2520_timer2.c
    src/peripherals/pic18f2520_timer3.c
)

epic_add_hal_library(${EPIC_LIB})

epic_add_example(example_blink tests/example_blink.c)
epic_add_example(example_irq   tests/example_irq.c)
epic_add_example(example_smoke tests/example_smoke.c)
epic_add_example(example_timer1 tests/example_timer1.c)
epic_add_example(example_timer2 tests/example_timer2.c)
epic_add_example(example_timer3 tests/example_timer3.c)
