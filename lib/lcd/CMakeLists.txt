# lib/lcd, HD44780-compatible character LCD driver with configurable transport
#
#   cmake -B build && cmake --build build
#   ctest --test-dir build --output-on-failure
#
# Core test uses a mock transport (no HAL). The gpio8/SPI sources ship
# on target only; the transport tests below build them against the
# mock_hal recorder to assert their exact sequences without hardware.

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

# Transport tests: the real gpio8/SPI sources against the mock_hal
# recorder. tests/mock_hal shadows epic_hal.h/epic_tick.h on these
# targets only (listed first); the core test above is untouched.
add_library(epic_lcd_mock_hal STATIC tests/mock_hal/mock_hal.c)
target_include_directories(epic_lcd_mock_hal PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/tests/mock_hal)

add_executable(test_epic_lcd_gpio8
    tests/test_epic_lcd_gpio8.c
    src/epic_lcd.c
    src/epic_lcd_gpio8.c)
target_include_directories(test_epic_lcd_gpio8 PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/tests/mock_hal
    ${CMAKE_CURRENT_SOURCE_DIR}/include)
target_link_libraries(test_epic_lcd_gpio8 PRIVATE epic_lcd_mock_hal)
add_test(NAME epic_lcd_gpio8_tests COMMAND test_epic_lcd_gpio8)

add_executable(test_epic_lcd_spi
    tests/test_epic_lcd_spi.c
    src/epic_lcd.c
    src/epic_lcd_spi.c)
target_include_directories(test_epic_lcd_spi PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/tests/mock_hal
    ${CMAKE_CURRENT_SOURCE_DIR}/include)
target_link_libraries(test_epic_lcd_spi PRIVATE epic_lcd_mock_hal)
add_test(NAME epic_lcd_spi_tests COMMAND test_epic_lcd_spi)
