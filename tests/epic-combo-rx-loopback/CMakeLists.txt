# epic-combo-rx-loopback, task 8 of the quality roadmap: the RX wall,
# via a target-in-the-loop harness. Unlike the other combos this
# module carries a host CMake
# build: MPLAB SIM cannot inject RX, so the firmware's echo path is
# verified on the host sim, which CAN (pic16f87xa_sim_drive_usart_rx),
# through the same dispatch and callback the real interrupt vector
# runs. The mdb gate (scripts/ci-target-sim.sh) builds the same
# firmware under XC8 and checks the arm state + boot banner.
#
#   cmake -B build && cmake --build build
#   ctest --test-dir build --output-on-failure

cmake_minimum_required(VERSION 3.16)
project(epic_combo_rx_loopback C)

set(CMAKE_C_STANDARD 99)
set(CMAKE_C_STANDARD_REQUIRED ON)

set(EPIC_DIR ${CMAKE_CURRENT_SOURCE_DIR}/../../pic16f87xa-hal
    CACHE PATH "Path to the pic16f87xa-hal source tree")
set(EPIC_LIB pic16f87xa_hal)

if(NOT TARGET ${EPIC_LIB})
    add_subdirectory(${EPIC_DIR} ${CMAKE_CURRENT_BINARY_DIR}/hal)
endif()

# The host test links the firmware source itself (with
# RX_LOOPBACK_HOST_TEST, which drops main() and the target polled-TX
# implementation; the test provides rx_loopback_tx as a byte recorder)
# so the injected bytes walk the real dispatch + callback path.
add_executable(test_rx_loopback
    tests/test_rx_loopback.c
    tests/combo_rx_loopback.c)
target_compile_definitions(test_rx_loopback PRIVATE
    -DPIC16F877A -DFOSC_HZ=20000000UL -DRX_LOOPBACK_HOST_TEST)
target_link_libraries(test_rx_loopback PRIVATE ${EPIC_LIB})

enable_testing()
add_test(NAME rx_loopback COMMAND test_rx_loopback)
