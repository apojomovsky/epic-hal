# epic-settings, EEPROM-backed settings blobs with CRC validation.
#
#   cmake -B build && cmake --build build
#   ctest --test-dir build --output-on-failure
#   ./build/test_settings && ./build/example_settings
#
# One family-agnostic implementation (src/epic_settings.c) builds against
# either HAL family. The public header is dependency-free; the implementation
# pulls the neutral HAL EEPROM API through epic_hal.h. Select the family with
# -DEPIC_FAMILY=PIC18 (default PIC16).

cmake_minimum_required(VERSION 3.16)
project(epic_settings C)

set(CMAKE_C_STANDARD 99)
set(CMAKE_C_STANDARD_REQUIRED ON)

set(EPIC_FAMILY "PIC16" CACHE STRING "HAL family to link: PIC16 or PIC18")

if(EPIC_FAMILY STREQUAL "PIC18")
    set(EPIC_DIR ${CMAKE_CURRENT_SOURCE_DIR}/../pic18fxx5x-hal
        CACHE PATH "Path to the pic18fxx5x-hal source tree")
    set(EPIC_LIB         pic18fxx5x_hal)
    set(DEFAULT_DEVICE  PIC18F4550)
else()
    set(EPIC_DIR ${CMAKE_CURRENT_SOURCE_DIR}/../pic16f87xa-hal
        CACHE PATH "Path to the pic16f87xa-hal source tree")
    set(EPIC_LIB         pic16f87xa_hal)
    set(DEFAULT_DEVICE  PIC16F877A)
endif()

if(NOT TARGET ${EPIC_LIB})
    add_subdirectory(${EPIC_DIR} ${CMAKE_CURRENT_BINARY_DIR}/hal)
endif()

add_library(epic_settings STATIC src/epic_settings.c)
target_include_directories(epic_settings PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/include)
target_link_libraries(epic_settings PUBLIC ${EPIC_LIB})
target_compile_definitions(epic_settings PRIVATE -D${DEFAULT_DEVICE})

enable_testing()
add_executable(test_settings tests/test_settings.c)
target_compile_definitions(test_settings PRIVATE -D${DEFAULT_DEVICE})
target_link_libraries(test_settings PRIVATE epic_settings)
add_test(NAME settings_tests COMMAND test_settings)

add_executable(example_settings examples/example_settings.c)
target_compile_definitions(example_settings PRIVATE -D${DEFAULT_DEVICE})
target_link_libraries(example_settings PRIVATE epic_settings)
