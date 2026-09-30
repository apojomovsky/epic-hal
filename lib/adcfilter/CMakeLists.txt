# epic-adcfilter, vendor-agnostic ADC oversampling + moving-average filter.
#
#   cmake -B build && cmake --build build
#   ctest --test-dir build --output-on-failure
#
# The core (src/epic_adcfilter.c) has ZERO hardware dependency: it includes
# only <stdint.h> and takes a read callback, so it compiles and tests on the
# host with no HAL family selection at all. The target-only example
# (examples/example_adcfilter.c) wires a real EPIC_ADC_* read callback and
# builds through the manifest-driven XC8 build, not this host CMake build.

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
