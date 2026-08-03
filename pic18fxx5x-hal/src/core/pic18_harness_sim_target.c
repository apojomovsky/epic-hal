/**
 * @file    pic18_harness_sim_target.c
 * @brief   PIC18F2455-family sim-target implementation of the test
 *          harness (see core/pic8_harness.h). Runs as real compiled
 *          firmware under MPLAB SIM (driven headlessly via mdb, see
 *          docs/ci-plan.md Phase 2), not a host-side simulator like
 *          pic18_harness_sim.c.
 *
 * @details
 *   Mirrors pic16_harness_sim_target.c exactly; see that file's header
 *   comment for the full rationale (why this lives per-family rather
 *   than in pic8-common, why init/tick stay no-ops while running()
 *   becomes bounded, why log() is a raw byte writer and not a real
 *   printf, why the TxCpltCallback workaround is safe here). Only the
 *   EUSART API surface differs: pic18fxx5x_usart.h's USART_ComputeSPBRG
 *   takes an extra BRG-width argument (USART_BAUDGEN_8BIT here, same
 *   8-bit SPBRG range as the PIC16 driver, simplest choice for a fixed
 *   9600 baud marker line), and its handle has extra fields
 *   (USART_HANDLE_DEFAULT already zero-initializes all of them).
 */

#include "core/pic8_harness.h"
#include "core/pic18_irq.h"
#include "peripherals/pic18fxx5x_usart.h"

#include <stdint.h>

#ifndef FOSC_HZ
#define FOSC_HZ 48000000UL
#endif

#define PIC8_HARNESS_SIM_BAUD 9600UL

static uint32_t g_cycles = 0U;

static void s_tx_cplt(void)
{
    /* Exists only so HAL_USART_Init's TXEN/TXIE gate sees a non-null
     * callback (see pic16_harness_sim_target.c's header comment).
     * Transmission below is polled, this is never actually called in a
     * way that matters. */
}

static void s_uart_putc(char c)
{
    while (!HAL_USART_IsTxShiftRegisterEmpty()) {
        /* wait for the shift register to drain */
    }
    HAL_USART_Transmit((uint8_t)c);
}

void pic8_harness_init(uint32_t cycles)
{
    g_cycles = cycles;

    /* BRGH=HIGH (the USART_HANDLE_DEFAULT default, divisor 16) needs
     * SPBRG=311 for 9600 baud at this file's 48 MHz FOSC_HZ, which
     * doesn't fit the 8-bit BRG (max 255): USART_ComputeSPBRG correctly
     * returned its error sentinel (0xFFFF, truncating to SPBRG=255),
     * silently misconfiguring the baud rate and producing no captured
     * UART output at all (confirmed via a real-target mdb probe, not
     * theoretical; see docs/ci-plan.md Phase 4's PIC18 follow-up).
     * BRGH=LOW (divisor 64) needs only SPBRG=77, comfortably in range
     * (actual baud ~9615, ~0.16% error, fine for this marker line). */
    USART_HandleTypeDef h = USART_HANDLE_DEFAULT;
    h.BaudHigh = USART_BRGH_LOW;
    h.SPBRG = (uint8_t)USART_ComputeSPBRG(FOSC_HZ, PIC8_HARNESS_SIM_BAUD,
                                           USART_MODE_ASYNCHRONOUS,
                                           USART_BRGH_LOW,
                                           USART_BAUDGEN_8BIT);
    h.TxCpltCallback = s_tx_cplt;
    (void)HAL_USART_Init(&h);
    /* Same TXIE-storm risk as pic16_harness_sim_target.c, same fix: see
     * that file's header comment for the full account (confirmed
     * against a real mdb run, not theoretical). Transmission here is
     * polled, TXIE was never wanted, only a side effect of the TXEN
     * workaround; turn the source back off, TXEN stays untouched. */
    HAL_IRQ_DisableSrc(PIC18_IRQ_USART_TX);
}

void pic8_harness_tick(void)
{
    /* Real time advances on its own under MPLAB SIM too, nothing to pump. */
}

int pic8_harness_running(uint32_t iteration)
{
    return (iteration < g_cycles) ? 1 : 0;
}

void pic8_harness_log(const char *fmt, ...)
{
    while (*fmt) {
        s_uart_putc(*fmt);
        fmt++;
    }
}
