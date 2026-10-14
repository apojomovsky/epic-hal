# lib/console, line-based serial command dispatcher over epic-serial.
#
#   cmake -B build && cmake --build build
#   ctest --test-dir build --output-on-failure
#   ./build/test_console

cmake_minimum_required(VERSION 3.16)
project(epic_console C)

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

set(EPIC_FAMILY "PIC16" CACHE STRING "HAL family to link through lib/serial: PIC16 or PIC18")

set(SERIAL_DIR ${EPIC_ROOT}/lib/serial
    CACHE PATH "Path to the lib/serial source tree")
if(NOT TARGET epic_serial)
    add_subdirectory(${SERIAL_DIR} ${CMAKE_CURRENT_BINARY_DIR}/serial)
endif()

if(EPIC_FAMILY STREQUAL "PIC18")
    set(DEFAULT_DEVICE PIC18F4550)
else()
    set(DEFAULT_DEVICE PIC16F877A)
endif()

add_library(epic_console STATIC src/epic_console.c)
target_include_directories(epic_console PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/include)
target_link_libraries(epic_console PUBLIC epic_serial)

enable_testing()
add_executable(test_console tests/test_console.c)
target_compile_definitions(test_console PRIVATE -D${DEFAULT_DEVICE})
target_link_libraries(test_console PRIVATE epic_console)
add_test(NAME console_tests COMMAND test_console)

# Host fuzz test: random byte streams through the line state machine;
# the parser must never wedge, must echo byte-exactly per the modeled
# edit behavior, and must recover so a sentinel command dispatches.
add_executable(test_console_fuzz tests/test_console_fuzz.c)
target_compile_definitions(test_console_fuzz PRIVATE -D${DEFAULT_DEVICE})
target_link_libraries(test_console_fuzz PRIVATE epic_console)
add_test(NAME console_fuzz COMMAND test_console_fuzz)
