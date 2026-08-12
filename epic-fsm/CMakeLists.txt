# epic-fsm, vendor-agnostic table-driven finite state machine engine.
#
#   cmake -B build && cmake --build build
#   ./build/test_fsm && ./build/example_traffic_light
#
# Unlike the HAL/task-manager modules in this repo, this library has no
# hardware dependency at all: fsm.c includes only <stdint.h>/<stdbool.h>,
# never a HAL header, and there is exactly one implementation for host,
# PIC16, and PIC18 alike (see the Makefiles under mcu/ for the real-target XC8
# builds, which compile this same src/fsm.c unchanged). Because of that,
# there is no EPIC_FAMILY selection needed for the core library or its
# tests — only the optional taskmgr-composition example below needs one,
# since it's the taskmgr (not the FSM engine) that is family-specific.

cmake_minimum_required(VERSION 3.16)
project(epic_fsm C)

set(CMAKE_C_STANDARD 99)
set(CMAKE_C_STANDARD_REQUIRED ON)

add_library(fsm STATIC src/fsm.c)
target_include_directories(fsm PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/include)

# Unit tests: exercise the exact code that ships on-target (no per-family
# split means no reference-vs-shipped gap to worry about).
enable_testing()
add_executable(test_fsm tests/test_fsm.c)
target_link_libraries(test_fsm PRIVATE fsm)
add_test(NAME fsm_unit_tests COMMAND test_fsm)

# Pure-host example: zero HAL dependency, proves the API reads at a glance
# with no other module in scope.
add_executable(example_traffic_light examples/example_traffic_light.c)
target_link_libraries(example_traffic_light PRIVATE fsm)

# Optional: the taskmgr-composition example. Off by default so this module
# never forces a HAL family choice on a consumer who only wants the FSM
# engine; turn it on to build the demonstration that an fsm_t composes with
# epic-taskmgr with zero special integration.
option(EPIC_FSM_BUILD_TASKMGR_EXAMPLE
       "Build examples/example_taskmgr_integration.c against epic-taskmgr" OFF)

if(EPIC_FSM_BUILD_TASKMGR_EXAMPLE)
    set(EPIC_FAMILY "PIC16" CACHE STRING "HAL family to link the taskmgr example against: PIC16 or PIC18")
    set(TASKMGR_DIR ${CMAKE_CURRENT_SOURCE_DIR}/../epic-taskmgr
        CACHE PATH "Path to the epic-taskmgr source tree")
    if(NOT TARGET epic_taskmgr)
        add_subdirectory(${TASKMGR_DIR} ${CMAKE_CURRENT_BINARY_DIR}/taskmgr)
    endif()

    add_executable(example_taskmgr_integration examples/example_taskmgr_integration.c)
    target_link_libraries(example_taskmgr_integration PRIVATE fsm epic_taskmgr)
endif()
