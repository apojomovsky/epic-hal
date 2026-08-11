/* epic-modbus on-target demo: a 4-holding-register RTU slave (address
 * 0x11, 9600 baud) that polls forever. Built by the XC8 Makefiles (the
 * host smoke test uses a host-sim RX-injection API not compiled on
 * target). Link/init smoke, not a live transaction. */

#include "epic_modbus.h"
#include "epic_tick.h"
#include "core/epic_harness.h"

#include <stddef.h>

#ifndef FOSC_HZ
#define FOSC_HZ 20000000UL
#endif

#define SLAVE_ADDR 0x11u
#define BAUD       9600u

static uint16_t holding_regs[4];

/** @brief On-target demo: a 4-holding-register RTU slave polling forever. */
int main(void)
{
    epic_harness_init(0UL);
    epic_tick_init(FOSC_HZ);

    static const epic_modbus_slave_map_t map = {
        .coils               = NULL,
        .num_coils           = 0,
        .discrete_inputs     = NULL,
        .num_discrete_inputs = 0,
        .holding_regs        = holding_regs,
        .num_holding_regs    = 4,
        .input_regs          = NULL,
        .num_input_regs      = 0,
    };
    epic_modbus_slave_init(FOSC_HZ, BAUD, SLAVE_ADDR, &map);

    for (uint32_t i = 0; epic_harness_running(i); i++) {
        epic_modbus_slave_poll();
    }
    return 0;
}
