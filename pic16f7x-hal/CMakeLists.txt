# PIC16F7x HAL, host simulation build (cmake -B build, build, run).
# Real-target builds use XC8 via the manifest + the reference .X project.
# Thin caller of epic-common/cmake/epic_family.cmake; only the source
# list and the device list are PIC16F7x-family-specific. The 7x has no
# data EEPROM, no comparator, no Vref, and an SPI-only SSP, so the
# shared-core EEPROM/comp/vref sources and the I2C-master SSP helpers
# are absent here.

cmake_minimum_required(VERSION 3.16)
project(pic16f7x_hal C)

set(CMAKE_C_STANDARD 99)
set(CMAKE_C_STANDARD_REQUIRED ON)

# Shared layer: status codes, harness, CMake helpers.
set(EPIC_COMMON_DIR ${CMAKE_CURRENT_SOURCE_DIR}/../epic-common)
include(${EPIC_COMMON_DIR}/cmake/epic_family.cmake)

# Default device (the canonical part; -DPIC16F7X_DEVICE=<part> picks
# another). Per-example device selection goes through EPIC_FAMILY_DEVICES.
if(NOT DEFINED PIC16F7X_DEVICE)
    set(PIC16F7X_DEVICE "PIC16F77")
endif()

set(PIC16F7X_DEFAULT_DEVICE PIC16F77)

# Variables consumed by the shared epic_family.cmake helpers.
set(EPIC_LIB              pic16f7x_hal)
set(EPIC_DEFAULT_DEVICE       ${PIC16F7X_DEFAULT_DEVICE})
set(EPIC_FAMILY_INCLUDE_DIR    ${CMAKE_CURRENT_SOURCE_DIR}/include)
set(EPIC_CORE_INCLUDE_DIR      ${CMAKE_CURRENT_SOURCE_DIR}/../pic14-midrange-core/include)
set(EPIC_HOST_INCLUDE_DIR      ${CMAKE_CURRENT_SOURCE_DIR}/include/host)
set(EPIC_COMMON_INCLUDE_DIR    ${EPIC_COMMON_DIR}/include)
set(EPIC_FAMILY_DEVICES        PIC16F72 PIC16F73 PIC16F74 PIC16F76
                               PIC16F77 PIC16F737 PIC16F747 PIC16F767 PIC16F777)

set(EPIC_SOURCES
    ../pic14-midrange-core/src/peripherals/pic14_gpio.c
    ../pic14-midrange-core/src/peripherals/pic14_timer0.c
    ../pic14-midrange-core/src/peripherals/pic14_timer1.c
    ../pic14-midrange-core/src/peripherals/pic14_timer2.c
    ../pic14-midrange-core/src/peripherals/pic14_ccp.c
    ../pic14-midrange-core/src/peripherals/pic14_adc.c
    ../pic14-midrange-core/src/peripherals/pic14_ssp.c
    ../pic14-midrange-core/src/peripherals/pic14_usart.c
    ../pic14-midrange-core/src/core/pic14_irq.c
    src/core/pic16_irq_table.c
    ../pic14-midrange-core/src/core/pic14_wdt_sleep.c
    ../pic14-midrange-core/src/sim/pic14_wdt_sleep_sim.c
    ../pic14-midrange-core/src/core/pic14_irq_dispatch.c
    src/sim/pic16_harness_sim.c
    src/sim/pic16f7x_sim.c
)

# The 16F77 default has PSP (PIC16F7X_FAMILY_HAS_PSP=1), and the shared
# IRQ dispatch references PSP_IRQHandler under PIC14MIDRANGE_HAS_PSP, so
# the family PSP driver must be linked for the default device build. The
# per-device blink loop below proves every variant links via its own
# HAS_PSP macro.
set(PIC16F7X_PSP_SRCS
    src/peripherals/pic16f7x_psp.c
)
list(APPEND EPIC_SOURCES ${PIC16F7X_PSP_SRCS})

epic_add_hal_library(${EPIC_LIB})

epic_add_example(example_blink       tests/example_blink.c)

# Optional: example for every device in the family.
epic_add_example_per_device(example_blink tests/example_blink.c)
