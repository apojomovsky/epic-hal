# epic-console, line-based serial command dispatcher over epic-serial.
#
#   cmake -B build && cmake --build build
#   ctest --test-dir build --output-on-failure
#   ./build/test_console && ./build/example_console

cmake_minimum_required(VERSION 3.16)
project(epic_console C)

set(CMAKE_C_STANDARD 99)
set(CMAKE_C_STANDARD_REQUIRED ON)

set(EPIC_FAMILY "PIC16" CACHE STRING "HAL family to link through epic-serial: PIC16 or PIC18")

set(SERIAL_DIR ${CMAKE_CURRENT_SOURCE_DIR}/../epic-serial
    CACHE PATH "Path to the epic-serial source tree")
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

add_executable(example_console examples/example_console.c)
target_compile_definitions(example_console PRIVATE -D${DEFAULT_DEVICE})
target_link_libraries(example_console PRIVATE epic_console)
