# pic8-encoder, interrupt-driven x4 quadrature decoder for 8-bit PICs.
#
#   cmake -B build && cmake --build build
#   ctest --test-dir build --output-on-failure
#   ./build/test_encoder && ./build/example_encoder_hal && ./build/example_encoder_pid_loop
#
# This build reuses the host simulation backend of a HAL family (pulled in via
# add_subdirectory) exactly as pic8-tick does: encoder_get_position() reads a
# volatile int32_t the ISR writes asynchronously, so it wraps the read in
# EPIC_IRQ_Disable()/EPIC_IRQ_Restore() -- the one HAL call this module makes.
# The optional glitch gate also calls pic8_tick_get()/pic8_tick_elapsed_since(),
# so pic8-encoder links pic8-tick, which in turn links the HAL family. Select
# the family with -DEPIC_FAMILY=PIC18 (default PIC16), same convention as
# pic8-tick. Real-target builds use the XC8 Makefiles under mcu/.
#
# encoder_update() itself is pure C with no HAL dependency (same as
# fsm_dispatch); the family dimension lives entirely in the atomic-read
# helper + the XC8 Makefiles. One src/encoder.c builds against any family.
# Only src/encoder.c includes the family-neutral shims (core/hal_irq.h,
# pic8_tick.h); the public encoder.h is dependency-free (only <stdint.h>),
# matching pic8-debounce's header/.c split.

cmake_minimum_required(VERSION 3.16)
project(pic8_encoder C)

set(CMAKE_C_STANDARD 99)
set(CMAKE_C_STANDARD_REQUIRED ON)

set(EPIC_FAMILY "PIC16" CACHE STRING "HAL family to link: PIC16 or PIC18")

if(EPIC_FAMILY STREQUAL "PIC18")
    set(EPIC_DIR ${CMAKE_CURRENT_SOURCE_DIR}/../pic18fxx5x-hal
        CACHE PATH "Path to the pic18fxx5x-hal source tree")
    set(EPIC_LIB          pic18fxx5x_hal)
    set(FAMILY_DEVICES   PIC18F2455 PIC18F2550 PIC18F4455 PIC18F4550)
    set(DEFAULT_DEVICE   PIC18F4550)
    set(DEFAULT_FOSC_HZ  48000000UL)
else()
    set(EPIC_DIR ${CMAKE_CURRENT_SOURCE_DIR}/../pic16f87xa-hal
        CACHE PATH "Path to the pic16f87xa-hal source tree")
    set(EPIC_LIB          pic16f87xa_hal)
    set(FAMILY_DEVICES   PIC16F873A PIC16F874A PIC16F876A PIC16F877A)
    set(DEFAULT_DEVICE   PIC16F877A)
    set(DEFAULT_FOSC_HZ  20000000UL)
endif()

if(NOT TARGET ${EPIC_LIB})
    add_subdirectory(${EPIC_DIR} ${CMAKE_CURRENT_BINARY_DIR}/hal)
endif()

# pic8-tick for the glitch-gate timebase (also pulls the HAL family in, but
# the guard above already did; this just brings in pic8_tick + its includes).
set(TICK_DIR ${CMAKE_CURRENT_SOURCE_DIR}/../pic8-tick
    CACHE PATH "Path to the pic8-tick source tree")
if(NOT TARGET pic8_tick)
    add_subdirectory(${TICK_DIR} ${CMAKE_CURRENT_BINARY_DIR}/tick)
endif()

# pic8-pid for example_encoder_pid_loop (the direct demonstration of this
# module's purpose: encoder_get_position() fed into pid_update()). pic8-pid
# is itself HAL-free (it links only pic8-math), and pulls pic8-math in itself.
set(PID_DIR ${CMAKE_CURRENT_SOURCE_DIR}/../pic8-pid
    CACHE PATH "Path to the pic8-pid source tree")
if(NOT TARGET pid)
    add_subdirectory(${PID_DIR} ${CMAKE_CURRENT_BINARY_DIR}/pid)
endif()

add_library(encoder STATIC src/encoder.c)
target_include_directories(encoder PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/include)
target_link_libraries(encoder PUBLIC pic8_tick ${EPIC_LIB})
target_compile_definitions(encoder PRIVATE -D${DEFAULT_DEVICE})

# ---- tests: exercise the exact code that ships on-target ----
enable_testing()
add_executable(test_encoder tests/test_encoder.c)
target_link_libraries(test_encoder PRIVATE encoder)
add_test(NAME encoder_tests COMMAND test_encoder)

# ---- examples ----
add_executable(example_encoder_hal examples/example_encoder_hal.c)
target_compile_definitions(example_encoder_hal PRIVATE -D${DEFAULT_DEVICE} -DFOSC_HZ=${DEFAULT_FOSC_HZ})
target_link_libraries(example_encoder_hal PRIVATE encoder)

add_executable(example_encoder_pid_loop examples/example_encoder_pid_loop.c)
target_compile_definitions(example_encoder_pid_loop PRIVATE -D${DEFAULT_DEVICE} -DFOSC_HZ=${DEFAULT_FOSC_HZ})
target_link_libraries(example_encoder_pid_loop PRIVATE encoder pid)
