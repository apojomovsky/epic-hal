/**
 * @file    example_modbus_target.c
 * @brief   pic8-modbus on-target demo: a 4-holding-register RTU slave.
 *
 * @details
 *   The XC8 Makefiles build this (the host smoke test uses a host-sim
 *   RX-injection API not compiled on target). Initializes the tick
 *   timebase and a 4-holding-register Modbus slave (address 0x11, 9600
 *   baud), then polls forever. This is a link/init smoke, not a live
 *   transaction, connect a real RTU master to exercise it for real.
 */

#include "pic8_modbus.h"
#include "pic8_tick.h"
#include "core/pic8_harness.h"

#include <stddef.h>

#ifndef FOSC_HZ
#define FOSC_HZ 20000000UL
#endif

#define SLAVE_ADDR 0x11u
#define BAUD       9600u

static uint16_t holding_regs[4];

int main(void)
{
    pic8_harness_init(0UL);
    pic8_tick_init(FOSC_HZ);

    static const pic8_modbus_slave_map_t map = {
        .coils               = NULL,
        .num_coils           = 0,
        .discrete_inputs     = NULL,
        .num_discrete_inputs = 0,
        .holding_regs        = holding_regs,
        .num_holding_regs    = 4,
        .input_regs          = NULL,
        .num_input_regs      = 0,
    };
    pic8_modbus_slave_init(FOSC_HZ, BAUD, SLAVE_ADDR, &map);

    for (uint32_t i = 0; pic8_harness_running(i); i++) {
        pic8_modbus_slave_poll();
    }
    return 0;
}
