# epic-sdcard, SD/MMC-over-SPI block storage for PIC18F2455/2550/4455/4550,
# wrapping the vendored M-Stack storage driver (third_party/m-stack-storage).
#
#   cmake -B build && cmake --build build
#   ctest --test-dir build --output-on-failure
#
# Unlike epic-usb, the vendored mmc.c/crc.c ARE compiled for the host build
# here -- they're portable C with no real hardware dependency (confirmed:
# grepped for SFR names/__delay_*/interrupt keywords, found none). The one
# thing that needed a small patch (see third_party/patches/) was mmc.c's
# own compiler allowlist, which didn't include a host build option.
#
# The host test binary links mmc.c/crc.c against tests/mock/mmc_config.h
# (a mock SD-over-SPI card), so tests exercise the actual shipped protocol
# logic, not a hand-written stand-in for it -- see
# epic-sdcard/docs/epic-sdcard-plan.md, "Host build story".
#
# The real target build goes entirely through the XC8 Makefile under
# mcu/pic18fxx5x-sdcard-mplabx/ (Phase 3, not written yet), which compiles
# mmc.c/crc.c against src/target/mmc_config.h instead.

cmake_minimum_required(VERSION 3.16)
project(epic_sdcard C)

set(CMAKE_C_STANDARD 99)
set(CMAKE_C_STANDARD_REQUIRED ON)

set(STORAGE_DIR ${CMAKE_CURRENT_SOURCE_DIR}/third_party/m-stack-storage)

add_library(epic_sdcard_hosttest STATIC
    ${STORAGE_DIR}/src/crc.c
    ${STORAGE_DIR}/src/mmc.c
    tests/mock/epic_sdcard_mock_spi.c)
target_include_directories(epic_sdcard_hosttest PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/tests/mock       # resolved first: mmc_config.h picks the mock
    ${STORAGE_DIR}/include
    ${CMAKE_CURRENT_SOURCE_DIR}/include)
target_compile_definitions(epic_sdcard_hosttest PUBLIC PIC8_SDCARD_HOST_BUILD)

enable_testing()
add_executable(test_epic_sdcard tests/test_epic_sdcard.c)
target_link_libraries(test_epic_sdcard PRIVATE epic_sdcard_hosttest)
add_test(NAME epic_sdcard_tests COMMAND test_epic_sdcard)
