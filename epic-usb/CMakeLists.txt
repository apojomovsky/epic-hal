# epic-usb, USB CDC-ACM device for PIC18F2455/2550/4455/4550, wrapping the
# vendored M-Stack USB stack (third_party/m-stack).
#
#   cmake -B build && cmake --build build
#   ctest --test-dir build --output-on-failure
#
# Unlike every other epic-* module, this one has no HAL-family split and no
# host build of the real implementation: epic_usb.c depends directly on
# M-Stack's usb.h/usb_cdc.h (a real PIC18 SIE peripheral, not something a
# host build links against). The host build here links
# epic_usb_host_stub.c instead -- a deliberately separate, independent test
# double, not a compile-time swap of the same source (see
# epic-usb/docs/epic-usb-plan.md, "Host build story", and
# epic_usb_host_stub.c's own file header for why).
#
# The real target build goes entirely through the XC8 Makefile under
# mcu/pic18fxx5x-usb-mplabx/ (Phase 3 of the plan, not wired up yet), not
# through this CMakeLists.txt at all.

cmake_minimum_required(VERSION 3.16)
project(epic_usb C)

set(CMAKE_C_STANDARD 99)
set(CMAKE_C_STANDARD_REQUIRED ON)

add_library(epic_usb_hoststub STATIC src/epic_usb_host_stub.c)
target_include_directories(epic_usb_hoststub PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/include)

enable_testing()
add_executable(test_epic_usb tests/test_epic_usb.c)
target_link_libraries(test_epic_usb PRIVATE epic_usb_hoststub)
add_test(NAME epic_usb_tests COMMAND test_epic_usb)

# ---- example: builds against the host stub for a bounded smoke run; the
# same source is meant to also build against the real target's epic_usb.c
# via mcu/pic18fxx5x-usb-mplabx/Makefile once that exists (Phase 3) ----
add_executable(example_usb_echo examples/example_usb_echo.c)
target_compile_definitions(example_usb_echo PRIVATE EPIC_USB_EXAMPLE_HOST_ITERS=4)
target_link_libraries(example_usb_echo PRIVATE epic_usb_hoststub)
add_test(NAME epic_usb_example_echo COMMAND example_usb_echo)
