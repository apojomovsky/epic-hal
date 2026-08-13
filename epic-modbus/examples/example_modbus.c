/*
 * epic-modbus target example: an RTU slave (address 0x11, 9600 baud)
 * exposing a 4-holding-register map and 8 coils, with an RS-485
 * driver-enable pin on GPIOB0. holding_regs[0] carries the uptime in
 * seconds refreshed from epic-tick, so a master polling FC 03 sees
 * live data; the other registers and the coils are writable by the
 * master (FC 06 / FC 05 / FC 16 / FC 15).
 */

#include "epic_modbus.h"
#include "epic_tick.h"
#include "epic_hal.h"                /* EPIC_IRQ_Restore */

#include <stdint.h>

#ifndef FOSC_HZ
#define FOSC_HZ 48000000UL
#endif

#define SLAVE_ADDR 0x11u
#define BAUD       9600u

/* RS-485 driver-enable: DE/RE tied together on GPIOB0. */
#define RS485_PORT 1u
#define RS485_PIN  0u

static uint16_t holding_regs[4];
static uint8_t  coils[1];            /* 8 coils, bit-packed */

/** @brief RTU slave polling holding registers with RS-485 enable. */
int main(void)
{
    epic_tick_init(FOSC_HZ);

    static const epic_modbus_slave_map_t map = {
        .coils               = coils,
        .num_coils           = 8u,
        .discrete_inputs     = NULL,
        .num_discrete_inputs = 0u,
        .holding_regs        = holding_regs,
        .num_holding_regs    = 4u,
        .input_regs          = NULL,
        .num_input_regs      = 0u,
    };
    epic_modbus_slave_init(FOSC_HZ, BAUD, SLAVE_ADDR, &map);
    epic_modbus_slave_set_rs485_dir_pin(RS485_PORT, RS485_PIN);
    EPIC_IRQ_Restore(1);             /* UART RX/TX and Timer2 ISRs */

    uint32_t last_sec = 0u;
    for (;;) {
        uint32_t sec = epic_tick_get() / 1000u;
        if (sec != last_sec) {
            holding_regs[0] = (uint16_t)sec;   /* live uptime for FC 03 */
            last_sec = sec;
        }
        epic_modbus_slave_poll();
    }
}
