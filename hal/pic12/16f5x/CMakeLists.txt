# PIC16F5x HAL, host simulation build (cmake -B build, build, run).
# Real-target builds use XC8 via the manifest. Thin caller of
# common/cmake/epic_family.cmake; only the source list and the
# device list are PIC16F5x-family-specific. The 12-bit baseline core:
# GPIO + polled Timer0 + the no-op IRQ stub + WDT/sleep; no shared
# pic14-midrange-core sources (different architecture).

cmake_minimum_required(VERSION 3.16)
project(pic16f5x_hal C)

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

# 16F506-only comparator/ADC block (DS41268D): a second library built
# with -DPIC16F506 carries the drivers plus the analog-aware sim, so
# the canonical 54-built lib keeps its budget and sources. The comp
# and ADC probes link this one.
set(_EPIC_SAVED_SOURCES ${EPIC_SOURCES})
set(_EPIC_SAVED_DEFAULT ${EPIC_DEFAULT_DEVICE})
set(EPIC_SOURCES ${EPIC_SOURCES} src/peripherals/pic16f5x_comp.c src/peripherals/pic16f5x_adc.c)
set(EPIC_DEFAULT_DEVICE PIC16F506)
epic_add_hal_library(pic16f5x_hal_506)
set(EPIC_SOURCES ${_EPIC_SAVED_SOURCES})
set(EPIC_DEFAULT_DEVICE ${_EPIC_SAVED_DEFAULT})
add_executable(example_comp tests/example_comp.c)
target_compile_definitions(example_comp PRIVATE -DPIC16F506)
target_link_libraries(example_comp PRIVATE pic16f5x_hal_506)
add_executable(example_adc tests/example_adc.c)
target_compile_definitions(example_adc PRIVATE -DPIC16F506)
target_link_libraries(example_adc PRIVATE pic16f5x_hal_506)
enable_testing()
add_test(NAME comp_host COMMAND example_comp)
add_test(NAME adc_host COMMAND example_adc)

# Optional: example for every device in the family (proves every part
# compiles and links against the canonical sources).
epic_add_example_per_device(example_blink tests/example_blink.c)
