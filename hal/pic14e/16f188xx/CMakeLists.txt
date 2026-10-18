# PIC16F188xx HAL, host simulation build.
#   cmake -B build && cmake --build build && ctest --test-dir build
#
# Real-target builds use the XC8 Makefile path (scripts/build/epic_build.py). The
# host vs target split is by include path (include/host ahead of include) and
# source selection, not #ifdef. Shared helpers: common/cmake/epic_family.cmake.

cmake_minimum_required(VERSION 3.16)
project(pic16f188xx_hal C)

set(CMAKE_C_STANDARD 99)
set(CMAKE_C_STANDARD_REQUIRED ON)

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

set(PIC16F188XX_DEFAULT_DEVICE PIC16F18877)

set(EPIC_LIB                  pic16f188xx_hal)
set(EPIC_DEFAULT_DEVICE       ${PIC16F188XX_DEFAULT_DEVICE})
set(EPIC_FAMILY_INCLUDE_DIR   ${CMAKE_CURRENT_SOURCE_DIR}/include)
set(EPIC_HOST_INCLUDE_DIR     ${CMAKE_CURRENT_SOURCE_DIR}/include/host)
set(EPIC_COMMON_INCLUDE_DIR   ${EPIC_COMMON_DIR}/include)
set(EPIC_FAMILY_DEVICES       PIC16F18854 PIC16F18855 PIC16F18856 PIC16F18857 PIC16F18875 PIC16F18876 PIC16F18877)

set(EPIC_SOURCES
    src/peripherals/pic16f188xx_gpio.c
    src/peripherals/pic16f188xx_timer0.c
    src/peripherals/pic16f188xx_timer1.c
    src/peripherals/pic16f188xx_timer2.c
    src/peripherals/pic16f188xx_ccp.c
    src/peripherals/pic16f188xx_pwm.c
    src/peripherals/pic16f188xx_pps.c
    src/peripherals/pic16f188xx_usart.c
    src/peripherals/pic16f188xx_ssp.c
    src/peripherals/pic16f188xx_eeprom.c
    src/peripherals/pic16f188xx_nco.c
    src/peripherals/pic16f188xx_adc.c
    src/peripherals/pic16f188xx_dac.c
    src/peripherals/pic16f188xx_fvr.c
    src/peripherals/pic16f188xx_comp.c
    src/core/pic16f188xx_irq.c
    src/core/pic16f188xx_irq_dispatch.c
    src/core/pic16f188xx_wdt_sleep.c
    src/sim/pic16f188xx_harness_sim.c
    src/sim/pic16f188xx_sim.c
    src/sim/pic16f188xx_sim_serial.c
    src/sim/pic16f188xx_sim_sys.c
    src/sim/pic16f188xx_wdt_sleep_sim.c
    src/sim/pic16f188xx_sim_ccp.c
    src/sim/pic16f188xx_sim_analog.c
)

epic_add_hal_library(${EPIC_LIB})

epic_add_example(example_blink tests/example_blink.c)
epic_add_example(example_gpio tests/example_gpio.c)
epic_add_example(example_gpio_mdb tests/example_gpio_mdb.c)
epic_add_example(example_timer0 tests/example_timer0.c)
epic_add_example(example_timer1 tests/example_timer1.c)
epic_add_example(example_timer2 tests/example_timer2.c)
epic_add_example(example_ccp tests/example_ccp.c)
epic_add_example(example_pwm tests/example_pwm.c)
epic_add_example(example_usart tests/example_usart.c)
epic_add_example(example_mssp tests/example_mssp.c)
epic_add_example(example_pps tests/example_pps.c)
epic_add_example(example_eeprom tests/example_eeprom.c)
epic_add_example(example_nco tests/example_nco.c)
epic_add_example(example_adc tests/example_adc.c)
epic_add_example(example_dac tests/example_dac.c)
epic_add_example(example_fvr tests/example_fvr.c)
epic_add_example(example_comp tests/example_comp.c)
epic_add_example(example_wdt_sleep tests/example_wdt_sleep.c)
epic_add_example_per_device(example_blink tests/example_blink.c)

enable_testing()
add_test(NAME gpio_host COMMAND example_gpio)
add_test(NAME gpio_mdb_host COMMAND example_gpio_mdb)
add_test(NAME timer0_host COMMAND example_timer0)
add_test(NAME timer1_host COMMAND example_timer1)
add_test(NAME timer2_host COMMAND example_timer2)
add_test(NAME ccp_host COMMAND example_ccp)
add_test(NAME pwm_host COMMAND example_pwm)
add_test(NAME usart_host COMMAND example_usart)
add_test(NAME mssp_host COMMAND example_mssp)
add_test(NAME pps_host COMMAND example_pps)
add_test(NAME eeprom_host COMMAND example_eeprom)
add_test(NAME nco_host COMMAND example_nco)
add_test(NAME adc_host COMMAND example_adc)
add_test(NAME dac_host COMMAND example_dac)
add_test(NAME fvr_host COMMAND example_fvr)
add_test(NAME comp_host COMMAND example_comp)
add_test(NAME wdt_sleep_host COMMAND example_wdt_sleep)
