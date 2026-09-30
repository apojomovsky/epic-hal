# epic-lcd, HD44780-compatible character LCD driver with configurable transport
#
#   cmake -B build && cmake --build build
#   ctest --test-dir build --output-on-failure
#
# The host test links the core logic against a mock transport (no HAL needed).
# The GPIO and SPI transports depend on the HAL and only build on target
# (via mcu/ Makefiles, not this CMakeLists.txt).

cmake_minimum_required(VERSION 3.16)
project(epic_lcd C)

set(CMAKE_C_STANDARD 99)
set(CMAKE_C_STANDARD_REQUIRED ON)

# Host test: core logic + mock transport, no HAL
add_library(epic_lcd_core STATIC src/epic_lcd.c)
target_include_directories(epic_lcd_core PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/include)

add_library(epic_lcd_mock STATIC tests/mock/epic_lcd_mock_transport.c)
target_include_directories(epic_lcd_mock PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/include
    ${CMAKE_CURRENT_SOURCE_DIR}/tests/mock)

enable_testing()
add_executable(test_epic_lcd tests/test_epic_lcd.c)
target_link_libraries(test_epic_lcd PRIVATE epic_lcd_core epic_lcd_mock)
add_test(NAME epic_lcd_tests COMMAND test_epic_lcd)
