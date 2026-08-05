/**
 * @file    pic18_harness_sim_target.c
 * @brief   PIC18F2455-family sim-target implementation of the test harness
 *          (see core/epic_harness.h). Runs as real compiled firmware under
 *          MPLAB SIM, driven headlessly via mdb.
 *
 * @details
 *   Mirrors `pic16_harness_sim_target.c`; see that file's header for the
 *   full rationale (per-family placement, why `init`/`tick` stay no-ops,
 *   why `log()` is a raw byte writer). Only the EUSART surface differs:
 *   `USART_ComputeSPBRG` takes an extra BRG-width argument here.
 */

#include "core/epic_harness.h"
#include "core/pic18_irq.h"
#include "peripherals/pic18fxx5x_usart.h"

#include <stdint.h>

#ifndef FOSC_HZ
#define FOSC_HZ 48000000UL
#endif

#define EPIC_HARNESS_SIM_BAUD 9600UL

static uint32_t g_cycles = 0U;

static void s_tx_cplt(void)
{
    /* Exists only so EPIC_USART_Init's TXEN/TXIE gate sees a non-null
     * callback (see pic16_harness_sim_target.c's header comment).
     * Transmission below is polled, this is never actually called in a
     * way that matters. */
}

static void s_uart_putc(char c)
{
    while (!EPIC_USART_IsTxShiftRegisterEmpty()) {
        /* wait for the shift register to drain */
    }
    EPIC_USART_Transmit((uint8_t)c);
}

void epic_harness_init(uint32_t cycles)
{
    g_cycles = cycles;

    /* BRGH=LOW (divisor 64) at this file's 48 MHz FOSC_HZ needs SPBRG=77
     * for 9600 baud, comfortably in the 8-bit BRG's range; BRGH=HIGH
     * (divisor 16) would need SPBRG=311, which doesn't fit. */
    USART_HandleTypeDef h = USART_HANDLE_DEFAULT;
    h.BaudHigh = USART_BRGH_LOW;
    h.SPBRG = (uint8_t)USART_ComputeSPBRG(FOSC_HZ, EPIC_HARNESS_SIM_BAUD,
                                           USART_MODE_ASYNCHRONOUS,
                                           USART_BRGH_LOW,
                                           USART_BAUDGEN_8BIT);
    h.TxCpltCallback = s_tx_cplt;
    (void)EPIC_USART_Init(&h);
    /* Transmission here is polled; TXIE is only a side effect of the
     * TxCpltCallback workaround above, turn the source back off (TXEN
     * stays untouched). Same pattern as pic16_harness_sim_target.c. */
    EPIC_IRQ_DisableSrc(PIC18_IRQ_USART_TX);
}

void epic_harness_tick(void)
{
    /* Real time advances on its own under MPLAB SIM too, nothing to pump. */
}

int epic_harness_running(uint32_t iteration)
{
    return (iteration < g_cycles) ? 1 : 0;
}

void epic_harness_log(const char *fmt, ...)
{
    while (*fmt) {
        s_uart_putc(*fmt);
        fmt++;
    }
}
