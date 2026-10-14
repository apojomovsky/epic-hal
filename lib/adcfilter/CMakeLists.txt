# lib/adcfilter, ADC oversampling plus moving-average filter.
#
#   cmake -B build && cmake --build build
#   ctest --test-dir build --output-on-failure
#
# Zero hardware dependency (read callback only), so no HAL family
# selection. The target-only example wires a real EPIC_ADC read via
# the manifest-driven XC8 build.

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
