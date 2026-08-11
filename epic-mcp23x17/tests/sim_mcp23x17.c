/**
 * HARNESS=sim build for epic-mcp23x17: the module's real `mdb` gate
 * on a 16F877A. MPLAB SIM has no I2C/SPI slave to inject (SEN
 * latches, SSPIF never sets), so the gate proves what the sim CAN:
 * the module + epic-bus + HAL SSP link and initialize under the real
 * toolchain. Transaction and register semantics are covered by the
 * host tests against the mock device.
 */

#include "epic_mcp23x17.h"

#include "epic_bus.h"
#include "core/epic_harness.h"

#ifndef FOSC_HZ
#define FOSC_HZ 20000000UL
#endif

int main(void)
{
    epic_harness_init(0UL);

    /* Real SSP configuration through the real API (the I2C master at
     * 100 kHz), then bind the expander handle exactly as a target
     * program would. */
    epic_bus_i2c_init(FOSC_HZ, 100000UL);
    epic_mcp23x17_handle_t h;
    int ok = 1;
    if (EPIC_MCP23X17_Init(&h, EPIC_MCP23X17_BUS_I2C, 0x20u) != 0) {
        epic_harness_log("mcp23x17 sim: init failed\n");
        ok = 0;
    }

    epic_harness_log(ok ? "mcp23x17 sim: linked and initialized ok\n"
                        : "mcp23x17 sim: sequence failed\n");
    (void)epic_harness_report(ok);
    for (;;) {
        epic_harness_tick();
    }
}
