# epic-taskmgr, cooperative task manager for the 8-bit PIC HAL.
#
#   cmake -B build && cmake --build build
#   ./build/example_multi_blink
#
# This build reuses the host simulation backend of a HAL family: it pulls
# the HAL in via add_subdirectory() so the family's static library (and its
# host platform header / harness) is built once and linked here. Real-target
# builds use the XC8 Makefile(s) under mcu/.
#
# The task manager is family-agnostic: it includes only the neutral HAL
# contract headers (epic_hal.h, core/hal_irq.h, peripherals/hal_timer0.h,
# core/hal_wdt_sleep.h) and the shared epic_harness.h, so the same
# epic_taskmgr.c/.h builds against any family. Select the family with
# -DEPIC_FAMILY=PIC18 (default PIC16); the build points at that family's
# tree and device list. This is the Phase 3 litmus test: pointing the task
# manager at pic18fxx5x-hal needs zero changes to epic_taskmgr.c/.h.
#
# The host/target split is done at build time, not with #ifdef, exactly as
# in the HAL: the CMake build puts include/host first on the HAL's include
# path (inherited from the family's HAL target) so the memory-backed SFR
# platform and the host harness link. No example or scheduler source needs a
# preprocessor branch for either build or either family.

cmake_minimum_required(VERSION 3.16)
project(epic_taskmgr C)

set(CMAKE_C_STANDARD 99)
set(CMAKE_C_STANDARD_REQUIRED ON)

# Which HAL family to link against. PIC16 (default) -> pic16f87xa-hal,
# PIC18 -> pic18fxx5x-hal. Override with -DEPIC_FAMILY=PIC18.
set(EPIC_FAMILY "PIC16" CACHE STRING "HAL family to link: PIC16 or PIC18")

if(EPIC_FAMILY STREQUAL "PIC18")
    set(EPIC_DIR ${CMAKE_CURRENT_SOURCE_DIR}/../pic18fxx5x-hal
        CACHE PATH "Path to the pic18fxx5x-hal source tree")
    set(EPIC_LIB          pic18fxx5x_hal)
    set(FAMILY_DEVICES   PIC18F2455 PIC18F2550 PIC18F4455 PIC18F4550)
    set(DEFAULT_DEVICE   PIC18F4550)
else()
    set(EPIC_DIR ${CMAKE_CURRENT_SOURCE_DIR}/../pic16f87xa-hal
        CACHE PATH "Path to the pic16f87xa-hal source tree")
    set(EPIC_LIB          pic16f87xa_hal)
    set(FAMILY_DEVICES   PIC16F873A PIC16F874A PIC16F876A PIC16F877A)
    set(DEFAULT_DEVICE   PIC16F877A)
endif()

# Build the HAL once into a subdirectory of our build tree so we get the
# family's HAL target (host platform + harness + sim backend) to link
# against. Its own examples build too; they are harmless and prove the HAL
# still compiles from this tree.
if(NOT TARGET ${EPIC_LIB})
    add_subdirectory(${EPIC_DIR} ${CMAKE_CURRENT_BINARY_DIR}/hal)
endif()

# The task manager itself, a small static library so other projects can
# link it the same way the example does.
add_library(epic_taskmgr STATIC src/epic_taskmgr.c)
target_include_directories(epic_taskmgr PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/include)
# The scheduler uses the HAL's Timer0, IRQ and harness APIs, and needs the
# same device selection. The HAL defaults to its family's default device
# when none is set.
target_link_libraries(epic_taskmgr PUBLIC ${EPIC_LIB})
target_compile_definitions(epic_taskmgr PRIVATE -D${DEFAULT_DEVICE})

# The example: four blinks on PORTB via the scheduler.
add_executable(example_multi_blink examples/example_multi_blink.c)
target_compile_definitions(example_multi_blink PRIVATE -D${DEFAULT_DEVICE})
target_link_libraries(example_multi_blink PRIVATE epic_taskmgr)

# Focused test: epic_taskmgr_reset re-arms a task from the full period.
add_executable(example_timer_reset examples/example_timer_reset.c)
target_compile_definitions(example_timer_reset PRIVATE -D${DEFAULT_DEVICE})
target_link_libraries(example_timer_reset PRIVATE epic_taskmgr)

# Both examples are bounded, self-reporting host-sim runs (the harness
# contract: epic_harness_report exit code), so they register as ctests
# the same way epic-tick's example_tick does. The multi-blink verifies
# four tasks at four distinct rates; timer_reset verifies epic_taskmgr_reset
# re-arms a task from the full period.
enable_testing()
add_test(NAME example_multi_blink COMMAND example_multi_blink)
add_test(NAME example_timer_reset COMMAND example_timer_reset)

# Host property test: random spawn/stop/start/reset/set_period
# sequences driven through the real scheduler API, fires verified
# against a model of the documented semantics (deterministic).
add_executable(test_taskmgr_fuzz tests/test_taskmgr_fuzz.c)
target_compile_definitions(test_taskmgr_fuzz PRIVATE -D${DEFAULT_DEVICE})
target_link_libraries(test_taskmgr_fuzz PRIVATE epic_taskmgr)
add_test(NAME taskmgr_fuzz COMMAND test_taskmgr_fuzz)

# Optional: the example for every device in the family. PORTB-only, so it
# runs on all parts; the scheduler's TASK_MGR_MAX_TASKS default scales to
# the family's RAM (6 slots on the 192 B PIC16 873A/874A, 8 on the 368 B
# 876A/877A; 8 on the 2048 B PIC18 family).
foreach(dev ${FAMILY_DEVICES})
    add_executable(example_multi_blink_${dev} examples/example_multi_blink.c)
    target_compile_definitions(example_multi_blink_${dev} PRIVATE -D${dev})
    target_link_libraries(example_multi_blink_${dev} PRIVATE epic_taskmgr)
endforeach()
