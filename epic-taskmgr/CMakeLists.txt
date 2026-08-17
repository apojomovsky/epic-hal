# epic-taskmgr, cooperative task manager for the 8-bit PIC HAL.
#
#   cmake -B build && cmake --build build
#   ctest --test-dir build --output-on-failure
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
    set(DEFAULT_FOSC_HZ  48000000UL)
elseif(EPIC_FAMILY STREQUAL "PIC16F88X")
    set(EPIC_DIR ${CMAKE_CURRENT_SOURCE_DIR}/../pic16f88x-hal
        CACHE PATH "Path to the pic16f88x-hal source tree")
    set(EPIC_LIB          pic16f88x_hal)
    set(FAMILY_DEVICES   PIC16F882 PIC16F883 PIC16F884 PIC16F886 PIC16F887)
    set(DEFAULT_DEVICE   PIC16F887)
    set(DEFAULT_FOSC_HZ  20000000UL)
else()
    set(EPIC_DIR ${CMAKE_CURRENT_SOURCE_DIR}/../pic16f87xa-hal
        CACHE PATH "Path to the pic16f87xa-hal source tree")
    set(EPIC_LIB          pic16f87xa_hal)
    set(FAMILY_DEVICES   PIC16F873A PIC16F874A PIC16F876A PIC16F877A)
    set(DEFAULT_DEVICE   PIC16F877A)
    set(DEFAULT_FOSC_HZ  20000000UL)
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

# The target-only example: compiled here as a host syntax/link gate for
# the default device. It is a real-silicon program (two periodic tasks
# driving LEDs, no stdout) so it is not run as a test.
add_executable(example_taskmgr examples/example_taskmgr.c)
target_compile_definitions(example_taskmgr PRIVATE -D${DEFAULT_DEVICE} -DFOSC_HZ=${DEFAULT_FOSC_HZ})
target_link_libraries(example_taskmgr PRIVATE epic_taskmgr)

# Host property test: random spawn/stop/start/reset/set_period
# sequences driven through the real scheduler API, fires verified
# against a model of the documented semantics (deterministic).
enable_testing()
add_executable(test_taskmgr_fuzz tests/test_taskmgr_fuzz.c)
target_compile_definitions(test_taskmgr_fuzz PRIVATE -D${DEFAULT_DEVICE})
target_link_libraries(test_taskmgr_fuzz PRIVATE epic_taskmgr)
add_test(NAME taskmgr_fuzz COMMAND test_taskmgr_fuzz)
