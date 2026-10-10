# PIC16F1508/1509 HAL, host simulation build.
#   cmake -B build && cmake --build build && ctest --test-dir build
#
# Real-target builds use the XC8 Makefile path (scripts/epic_build.py). The
# host vs target split is by include path (include/host ahead of include) and
# source selection, not #ifdef. Shared helpers: common/cmake/epic_family.cmake.

cmake_minimum_required(VERSION 3.16)
project(pic16f1508_hal C)

set(CMAKE_C_STANDARD 99)
set(CMAKE_C_STANDARD_REQUIRED ON)

set(EPIC_COMMON_DIR ${CMAKE_CURRENT_SOURCE_DIR}/../../../common)
include(${EPIC_COMMON_DIR}/cmake/epic_family.cmake)

set(PIC16F1508_DEFAULT_DEVICE PIC16F1509)

set(EPIC_LIB                  pic16f1508_hal)
set(EPIC_DEFAULT_DEVICE       ${PIC16F1508_DEFAULT_DEVICE})
set(EPIC_FAMILY_INCLUDE_DIR   ${CMAKE_CURRENT_SOURCE_DIR}/include)
set(EPIC_HOST_INCLUDE_DIR     ${CMAKE_CURRENT_SOURCE_DIR}/include/host)
set(EPIC_COMMON_INCLUDE_DIR   ${EPIC_COMMON_DIR}/include)
set(EPIC_FAMILY_DEVICES       PIC16F1508 PIC16F1509)

set(EPIC_SOURCES
    src/peripherals/pic16f1508_gpio.c
    src/core/pic16f1508_irq.c
    src/core/pic16f1508_irq_dispatch.c
    src/sim/pic16f1508_harness_sim.c
    src/sim/pic16f1508_sim.c
)

epic_add_hal_library(${EPIC_LIB})

epic_add_example(example_blink tests/example_blink.c)
epic_add_example(example_gpio tests/example_gpio.c)
epic_add_example(example_gpio_mdb tests/example_gpio_mdb.c)

enable_testing()
add_test(NAME gpio_host COMMAND example_gpio)
add_test(NAME gpio_mdb_host COMMAND example_gpio_mdb)
