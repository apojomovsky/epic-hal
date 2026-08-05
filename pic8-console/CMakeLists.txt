# pic8-console, line-based serial command dispatcher over pic8-serial.
#
#   cmake -B build && cmake --build build
#   ctest --test-dir build --output-on-failure
#   ./build/test_console && ./build/example_console

cmake_minimum_required(VERSION 3.16)
project(pic8_console C)

set(CMAKE_C_STANDARD 99)
set(CMAKE_C_STANDARD_REQUIRED ON)

set(EPIC_FAMILY "PIC16" CACHE STRING "HAL family to link through pic8-serial: PIC16 or PIC18")

set(SERIAL_DIR ${CMAKE_CURRENT_SOURCE_DIR}/../pic8-serial
    CACHE PATH "Path to the pic8-serial source tree")
if(NOT TARGET pic8_serial)
    add_subdirectory(${SERIAL_DIR} ${CMAKE_CURRENT_BINARY_DIR}/serial)
endif()

if(EPIC_FAMILY STREQUAL "PIC18")
    set(DEFAULT_DEVICE PIC18F4550)
else()
    set(DEFAULT_DEVICE PIC16F877A)
endif()

add_library(pic8_console STATIC src/pic8_console.c)
target_include_directories(pic8_console PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/include)
target_link_libraries(pic8_console PUBLIC pic8_serial)

enable_testing()
add_executable(test_console tests/test_console.c)
target_compile_definitions(test_console PRIVATE -D${DEFAULT_DEVICE})
target_link_libraries(test_console PRIVATE pic8_console)
add_test(NAME console_tests COMMAND test_console)

add_executable(example_console examples/example_console.c)
target_compile_definitions(example_console PRIVATE -D${DEFAULT_DEVICE})
target_link_libraries(example_console PRIVATE pic8_console)
