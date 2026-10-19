# PIC18F6520 host-sim build (foundation): cmake -B build, build, run
# ./build/example_smoke. Real-target XC8 builds come from the manifest
# (common/manifest/modules.toml); mcu/pic18f6520-mplabx/ covers that
# path. include/host leads the include path here (memory-backed SFRs);
# the XC8 build mirrors with include/target. Thin caller of
# common/cmake helpers; only the source/device lists are local.

cmake_minimum_required(VERSION 3.16)
project(pic18f6520_hal C)

set(CMAKE_C_STANDARD 99)
set(CMAKE_C_STANDARD_REQUIRED ON)

# Shared layer: status codes, harness, CMake helpers.
# Root anchor: walk up to the shared helpers, so moving this directory
# changes no path below. Cross-module paths hang off EPIC_ROOT.
if(NOT DEFINED EPIC_ROOT)
    set(_EPIC_PROBE "${CMAKE_CURRENT_SOURCE_DIR}")
    while(NOT EXISTS "${_EPIC_PROBE}/common/cmake/epic_family.cmake")
        get_filename_component(_EPIC_PARENT "${_EPIC_PROBE}/.." ABSOLUTE)
        if("${_EPIC_PARENT}" STREQUAL "${_EPIC_PROBE}")
            message(FATAL_ERROR "EPIC_ROOT not found above ${CMAKE_CURRENT_SOURCE_DIR}")
        endif()
        set(_EPIC_PROBE "${_EPIC_PARENT}")
    endwhile()
    set(EPIC_ROOT "${_EPIC_PROBE}")
    unset(_EPIC_PROBE)
    unset(_EPIC_PARENT)
endif()
include(${EPIC_ROOT}/common/cmake/epic_family.cmake)
set(EPIC_COMMON_DIR ${EPIC_ROOT}/common)

# Default device. The PIC18F6520 is the canonical part in this family.
if(NOT DEFINED PIC18F6520_DEVICE)
    set(PIC18F6520_DEVICE "PIC18F6520")
endif()
set(PIC18F6520_DEFAULT_DEVICE PIC18F6520)

# Variables consumed by the shared epic_family.cmake helpers.
set(EPIC_LIB              pic18f6520_hal)
set(EPIC_DEFAULT_DEVICE       ${PIC18F6520_DEFAULT_DEVICE})
set(EPIC_FAMILY_INCLUDE_DIR    ${CMAKE_CURRENT_SOURCE_DIR}/include)
set(EPIC_HOST_INCLUDE_DIR      ${CMAKE_CURRENT_SOURCE_DIR}/include/host)
set(EPIC_COMMON_INCLUDE_DIR    ${EPIC_COMMON_DIR}/include)
set(EPIC_FAMILY_DEVICES        PIC18F6620 PIC18F6720 PIC18F6585 PIC18F6680 PIC18F8525 PIC18F8585 PIC18F8621 PIC18F8680 PIC18F6525 PIC18F6621 PIC18F6520)

# GPIO + Timer0-4 + LVD drivers, the IRQ core and its dispatch, the
# WDT/Sleep helpers, and the host sim backend + harness. Every listed
# driver links on all eleven parts (absent CCP/USART/TMR4 instances
# rejected at Init; PORTH/J compiled out on the 64-pin parts).
set(EPIC_SOURCES
    src/sim/pic18_sim.c
    src/sim/pic18_harness_sim.c
    src/core/pic18_irq.c
    src/core/pic18_irq_dispatch.c
    src/core/pic18f6520_wdt_sleep.c
    src/sim/pic18f6520_wdt_sleep_sim.c
    src/peripherals/pic18f6520_gpio.c
    src/peripherals/pic18f6520_timer0.c
    src/peripherals/pic18f6520_timer1.c
    src/peripherals/pic18f6520_timer2.c
    src/peripherals/pic18f6520_timer3.c
    src/peripherals/pic18f6520_timer4.c
    src/peripherals/pic18f6520_ccp.c
    src/peripherals/pic18f6520_ssp.c
    src/peripherals/pic18f6520_usart.c
    src/peripherals/pic18f6520_adc.c
    src/peripherals/pic18f6520_comp.c
    src/peripherals/pic18f6520_eeprom.c
    src/peripherals/pic18f6520_lvd.c
)

epic_add_hal_library(${EPIC_LIB})

epic_add_example(example_blink tests/example_blink.c)
epic_add_example(example_irq   tests/example_irq.c)
epic_add_example(example_smoke tests/example_smoke.c)
epic_add_example(example_timer1 tests/example_timer1.c)
epic_add_example(example_timer2 tests/example_timer2.c)
epic_add_example(example_timer3 tests/example_timer3.c)
epic_add_example(example_timer4 tests/example_timer4.c)
epic_add_example(example_ccp   tests/example_ccp.c)
epic_add_example(example_ssp   tests/example_ssp.c)
epic_add_example(example_usart tests/example_usart.c)
epic_add_example(example_adc    tests/example_adc.c)
epic_add_example(example_comp   tests/example_comp.c)
epic_add_example(example_eeprom tests/example_eeprom.c)
epic_add_example(example_lvd    tests/example_lvd.c)
epic_add_example(example_wdt_sleep tests/example_wdt_sleep.c)
epic_add_example(sim_timer0_psa tests/sim_timer0_psa.c)

# example_wdt_sleep asserts the active-low RCON reset flags, so it runs
# under ctest while the demo examples stay build-only.
enable_testing()
add_test(NAME wdt_sleep_host COMMAND example_wdt_sleep)
add_test(NAME timer0_psa_host COMMAND sim_timer0_psa)
