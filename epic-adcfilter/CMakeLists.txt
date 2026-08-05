# epic-adcfilter, vendor-agnostic ADC oversampling + moving-average filter.
#
#   cmake -B build && cmake --build build
#   ctest --test-dir build --output-on-failure
#
# The core (src/epic_adcfilter.c) has ZERO hardware dependency: it includes
# only <stdint.h> and takes a read callback, so it compiles and tests on the
# host with no HAL family selection at all. Only the optional HAL example
# (examples/example_adcfilter_hal.c) pulls in a HAL family for the real ADC
# read, exactly the way epic-fsm's taskmgr-integration example is the one
# thing in that module needing a family choice.
#
#   cmake -B build -DEPIC_ADCFILTER_BUILD_EPIC_EXAMPLE=ON   # builds the example

cmake_minimum_required(VERSION 3.16)
project(epic_adcfilter C)

set(CMAKE_C_STANDARD 99)
set(CMAKE_C_STANDARD_REQUIRED ON)

add_library(adcfilter STATIC src/epic_adcfilter.c)
target_include_directories(adcfilter PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/include)

enable_testing()
add_executable(test_adcfilter tests/test_adcfilter.c)
target_link_libraries(test_adcfilter PRIVATE adcfilter)
add_test(NAME adcfilter_tests COMMAND test_adcfilter)

# Optional: HAL example (links a HAL family for the ADC read callback).
option(EPIC_ADCFILTER_BUILD_EPIC_EXAMPLE
       "Build examples/example_adcfilter_hal.c against a real HAL ADC" OFF)

if(EPIC_ADCFILTER_BUILD_EPIC_EXAMPLE)
    set(EPIC_FAMILY "PIC16" CACHE STRING "HAL family: PIC16 or PIC18")
    if(EPIC_FAMILY STREQUAL "PIC18")
        set(EPIC_DIR ${CMAKE_CURRENT_SOURCE_DIR}/../pic18fxx5x-hal CACHE PATH "")
        set(EPIC_LIB pic18fxx5x_hal)
        set(DEFAULT_DEVICE PIC18F4550)
    else()
        set(EPIC_DIR ${CMAKE_CURRENT_SOURCE_DIR}/../pic16f87xa-hal CACHE PATH "")
        set(EPIC_LIB pic16f87xa_hal)
        set(DEFAULT_DEVICE PIC16F877A)
    endif()
    if(NOT TARGET ${EPIC_LIB})
        add_subdirectory(${EPIC_DIR} ${CMAKE_CURRENT_BINARY_DIR}/hal)
    endif()
    add_executable(example_adcfilter_hal examples/example_adcfilter_hal.c)
    target_compile_definitions(example_adcfilter_hal PRIVATE -D${DEFAULT_DEVICE})
    target_link_libraries(example_adcfilter_hal PRIVATE adcfilter ${EPIC_LIB})
endif()
