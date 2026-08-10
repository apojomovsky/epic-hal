/**
 * @file    example_mcp23x17_target.c
 * @brief   epic-mcp23x17 on-target example: drives a real MCP23017 on
 *          the PIC's I2C bus (and the register configuration for the
 *          MCP23S17 SPI twin), proving the module links against the
 *          real HAL and that the transactions are framed correctly.
 *
 * @details
 *   The MSSP data path is unmodeled under MPLAB SIM (the same wall as
 *   the epic-bus gate: SEN latches, SSPIF never sets, so the default
 *   ops' SSPIF waits would block with no real device attached). This
 *   example therefore configures the bus and the expander handle but
 *   issues no MEM transaction at runtime; the transaction logic is
 *   host-tested against the mock device instead. On real silicon, the
 *   same calls with a device attached are the full bring-up path.
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

    /* MCP23017: I2C master, 100 kHz, device address 0b0100000 (A2A1A0
     * tied low). MCP23S17 users instead init the SPI master + CS pin
     * and use EPIC_MCP23X17_BUS_SPI. */
    epic_bus_i2c_init(FOSC_HZ, 100000UL);

    epic_mcp23x17_handle_t h;
    (void)EPIC_MCP23X17_Init(&h, EPIC_MCP23X17_BUS_I2C, 0x20u);

    /* The full bring-up sequence, host-verified against the mock:
     * all 16 pins as outputs, then drive a pattern. On a real board
     * the expander answers and the calls return the byte counts; the
     * results are deliberately not acted on here (no device on the
     * sim bus). */
    (void)EPIC_MCP23X17_SetDirectionAll(&h, 0x0000u);
    (void)EPIC_MCP23X17_WriteAll(&h, 0x00AAu);

    for (uint32_t i = 0; epic_harness_running(i); i++) {
        epic_harness_tick();
    }
    return 0;
}
