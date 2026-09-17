# PIC16F818/819 HAL, host simulation build: cmake -B build, cmake --build
# build, then run ./build/example_* by hand (no HAL here registers a ctest
# test). Real-target builds go through the manifest and the XC8 toolchain;
# examples/epic-hal-demo-pic16f818_819.X is the MPLAB X reference project.
# Thin caller of epic-common/cmake/epic_family.cmake: the host-versus-real
# split is the include order (include/host before include here, versus
# include/target on the XC8 side), so only the source list and the device
# list are family-specific in this file.

cmake_minimum_required(VERSION 3.16)
project(pic16f818_819_hal C)

set(CMAKE_C_STANDARD 99)
set(CMAKE_C_STANDARD_REQUIRED ON)

# Shared layer: status codes, harness, CMake helpers.
set(EPIC_COMMON_DIR ${CMAKE_CURRENT_SOURCE_DIR}/../epic-common)
include(${EPIC_COMMON_DIR}/cmake/epic_family.cmake)

# Default device (the canonical part; -DPIC16F818_819_DEVICE=<part> picks
# another). Per-example device selection goes through EPIC_FAMILY_DEVICES.
if(NOT DEFINED PIC16F818_819_DEVICE)
    set(PIC16F818_819_DEVICE "PIC16F819")
endif()

set(PIC16F818_819_DEFAULT_DEVICE PIC16F819)

# Variables consumed by the shared epic_family.cmake helpers.
set(EPIC_LIB              pic16f818_819_hal)
set(EPIC_DEFAULT_DEVICE       ${PIC16F818_819_DEFAULT_DEVICE})
set(EPIC_FAMILY_INCLUDE_DIR    ${CMAKE_CURRENT_SOURCE_DIR}/include)
set(EPIC_CORE_INCLUDE_DIR      ${CMAKE_CURRENT_SOURCE_DIR}/../pic14-midrange-core/include)
set(EPIC_HOST_INCLUDE_DIR      ${CMAKE_CURRENT_SOURCE_DIR}/include/host)
set(EPIC_COMMON_INCLUDE_DIR    ${EPIC_COMMON_DIR}/include)
set(EPIC_FAMILY_DEVICES        PIC16F818 PIC16F819)

set(EPIC_SOURCES
    ../pic14-midrange-core/src/peripherals/pic14_gpio.c
    ../pic14-midrange-core/src/peripherals/pic14_timer0.c
    ../pic14-midrange-core/src/peripherals/pic14_timer1.c
    ../pic14-midrange-core/src/peripherals/pic14_timer2.c
    ../pic14-midrange-core/src/peripherals/pic14_ccp.c
    ../pic14-midrange-core/src/peripherals/pic14_ssp.c
    ../pic14-midrange-core/src/peripherals/pic14_adc.c
    ../pic14-midrange-core/src/peripherals/pic14_eeprom.c
    ../pic14-midrange-core/src/core/pic14_irq.c
    src/core/pic16_irq_table.c
    ../pic14-midrange-core/src/core/pic14_wdt_sleep.c
    ../pic14-midrange-core/src/sim/pic14_wdt_sleep_sim.c
    ../pic14-midrange-core/src/core/pic14_irq_dispatch.c
    src/sim/pic16_harness_sim.c
    src/sim/pic16f818_819_sim.c
)

epic_add_hal_library(${EPIC_LIB})

epic_add_example(example_blink       tests/example_blink.c)
epic_add_example(example_rb_change   tests/example_rb_change.c)
epic_add_example(example_eeprom      tests/example_eeprom.c)
epic_add_example(example_timer1      tests/example_timer1.c)
epic_add_example(example_timer2      tests/example_timer2.c)
epic_add_example(example_ccp         tests/example_ccp.c)
epic_add_example(example_ssp         tests/example_ssp.c)
epic_add_example(example_adc         tests/example_adc.c)
epic_add_example(example_wdt_sleep   tests/example_wdt_sleep.c)

# Optional: example for every device in the family.
epic_add_example_per_device(example_blink tests/example_blink.c)
