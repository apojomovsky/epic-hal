# PIC16F628A HAL, host simulation build (cmake -B build, build, run).
# Real-target builds use XC8 via the manifest + the reference .X project.
# Thin caller of epic-common/cmake/epic_family.cmake; only the source
# list and the single device are PIC16F628A-specific.

cmake_minimum_required(VERSION 3.16)
project(pic16f628a_hal C)

set(CMAKE_C_STANDARD 99)
set(CMAKE_C_STANDARD_REQUIRED ON)

# Shared layer: status codes, harness, CMake helpers.
set(EPIC_COMMON_DIR ${CMAKE_CURRENT_SOURCE_DIR}/../epic-common)
include(${EPIC_COMMON_DIR}/cmake/epic_family.cmake)

# Default device (the family's only part).
if(NOT DEFINED PIC16F628A_DEVICE)
    set(PIC16F628A_DEVICE "PIC16F628A")
endif()

set(PIC16F628A_DEFAULT_DEVICE PIC16F628A)

# Variables consumed by the shared epic_family.cmake helpers.
set(EPIC_LIB              pic16f628a_hal)
set(EPIC_DEFAULT_DEVICE       ${PIC16F628A_DEFAULT_DEVICE})
set(EPIC_FAMILY_INCLUDE_DIR    ${CMAKE_CURRENT_SOURCE_DIR}/include)
set(EPIC_CORE_INCLUDE_DIR      ${CMAKE_CURRENT_SOURCE_DIR}/../pic14-midrange-core/include)
set(EPIC_HOST_INCLUDE_DIR      ${CMAKE_CURRENT_SOURCE_DIR}/include/host)
set(EPIC_COMMON_INCLUDE_DIR    ${EPIC_COMMON_DIR}/include)
set(EPIC_FAMILY_DEVICES        PIC16F628A)

set(EPIC_SOURCES
    ../pic14-midrange-core/src/peripherals/pic14_gpio.c
    ../pic14-midrange-core/src/peripherals/pic14_timer0.c
    ../pic14-midrange-core/src/peripherals/pic14_timer1.c
    ../pic14-midrange-core/src/peripherals/pic14_timer2.c
    ../pic14-midrange-core/src/peripherals/pic14_ccp.c
    ../pic14-midrange-core/src/peripherals/pic14_usart.c
    ../pic14-midrange-core/src/peripherals/pic14_comp.c
    ../pic14-midrange-core/src/peripherals/pic14_vref.c
    ../pic14-midrange-core/src/peripherals/pic14_eeprom.c
    ../pic14-midrange-core/src/core/pic14_irq.c
    src/core/pic16_irq_table.c
    ../pic14-midrange-core/src/core/pic14_wdt_sleep.c
    ../pic14-midrange-core/src/sim/pic14_wdt_sleep_sim.c
    ../pic14-midrange-core/src/core/pic14_irq_dispatch.c
    src/sim/pic16_harness_sim.c
    src/sim/pic16f628a_sim.c
)

epic_add_hal_library(${EPIC_LIB})

epic_add_example(example_blink       tests/example_blink.c)
epic_add_example(example_idle_blink  tests/example_idle_blink.c)
epic_add_example(example_timer1      tests/example_timer1.c)
epic_add_example(example_timer2      tests/example_timer2.c)
epic_add_example(example_ccp_pwm     tests/example_ccp_pwm.c)
epic_add_example(example_usart       tests/example_usart.c)
epic_add_example(example_eeprom      tests/example_eeprom.c)
epic_add_example(example_comp_vref   tests/example_comp_vref.c)
epic_add_example(example_wdt_sleep   tests/example_wdt_sleep.c)
epic_add_example(example_rb_change   tests/example_rb_change.c)

# Optional: example for every device in the family.
epic_add_example_per_device(example_blink tests/example_blink.c)
