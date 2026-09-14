# PIC16F63x/67x/68x HAL, host simulation build (cmake -B build, build, run).
# Real-target builds use XC8 via the manifest + the reference .X project.
# Thin caller of epic-common/cmake/epic_family.cmake; only the source
# list and the device list are family-specific.

cmake_minimum_required(VERSION 3.16)
project(pic16f63x_67x_68x_hal C)

set(CMAKE_C_STANDARD 99)
set(CMAKE_C_STANDARD_REQUIRED ON)

# Shared layer: status codes, harness, CMake helpers.
set(EPIC_COMMON_DIR ${CMAKE_CURRENT_SOURCE_DIR}/../epic-common)
include(${EPIC_COMMON_DIR}/cmake/epic_family.cmake)

# Default device (the canonical part; -DPIC16F63X_67X_68X_DEVICE=<part>
# picks another). Per-example device selection goes through
# EPIC_FAMILY_DEVICES.
if(NOT DEFINED PIC16F63X_67X_68X_DEVICE)
    set(PIC16F63X_67X_68X_DEVICE "PIC16F631")
endif()

set(PIC16F63X_67X_68X_DEFAULT_DEVICE PIC16F631)

# Variables consumed by the shared epic_family.cmake helpers.
set(EPIC_LIB              pic16f63x_67x_68x_hal)
set(EPIC_DEFAULT_DEVICE       ${PIC16F63X_67X_68X_DEFAULT_DEVICE})
set(EPIC_FAMILY_INCLUDE_DIR    ${CMAKE_CURRENT_SOURCE_DIR}/include)
set(EPIC_CORE_INCLUDE_DIR      ${CMAKE_CURRENT_SOURCE_DIR}/../pic14-midrange-core/include)
set(EPIC_HOST_INCLUDE_DIR      ${CMAKE_CURRENT_SOURCE_DIR}/include/host)
set(EPIC_COMMON_INCLUDE_DIR    ${EPIC_COMMON_DIR}/include)
set(EPIC_FAMILY_DEVICES        PIC16F631 PIC16F630 PIC16F676 PIC16F639 PIC16F684 PIC16F685 PIC16F688 PIC16F677)

set(EPIC_SOURCES
    src/peripherals/pic16f63x_67x_68x_gpio.c
    ../pic14-midrange-core/src/peripherals/pic14_timer0.c
    ../pic14-midrange-core/src/peripherals/pic14_timer1.c
    src/peripherals/pic16f63x_67x_68x_comp.c
    ../pic14-midrange-core/src/peripherals/pic14_eeprom.c
    src/peripherals/pic16f63x_67x_68x_osc.c
    ../pic14-midrange-core/src/core/pic14_irq.c
    src/core/pic16_irq_table.c
    ../pic14-midrange-core/src/core/pic14_wdt_sleep.c
    ../pic14-midrange-core/src/sim/pic14_wdt_sleep_sim.c
    ../pic14-midrange-core/src/core/pic14_irq_dispatch.c
    src/sim/pic16_harness_sim.c
    src/sim/pic16f63x_67x_68x_sim.c
)

epic_add_hal_library(${EPIC_LIB})

epic_add_example(example_blink       tests/example_blink.c)
epic_add_example(example_timer1      tests/example_timer1.c)
epic_add_example(example_comp        tests/example_comp.c)
epic_add_example(example_eeprom      tests/example_eeprom.c)
epic_add_example(example_wdt_sleep   tests/example_wdt_sleep.c)
epic_add_example(example_rb_change   tests/example_rb_change.c)

# Optional: example for every device in the family. The host library
# builds once (default device), so per-device examples only recompile
# the example source: they smoke-test device selection, not
# device-specific register paths (ANSELH lives in the target-only mdb
# probe for that reason).
epic_add_example_per_device(example_blink tests/example_blink.c)
