/**
 * @file    pic16_harness_sim_target.c
 * @brief   PIC16F87XA sim-target implementation of the test harness (see
 *          core/epic_harness.h). Runs as real compiled firmware under
 *          MPLAB SIM (driven headlessly via mdb, see docs/ci-plan.md
 *          Phase 2), not a host-side simulator like pic16_harness_sim.c.
 *
 * @details
 *   Not in epic-common: unlike epic_harness_target.c's four no-ops,
 *   this needs real USART access for epic_harness_report's marker line
 *   to reach mdb's UART capture. running() is bounded by `cycles` so a
 *   run terminates on its own. epic_harness_log writes fmt's raw
 *   bytes only, ignoring variadic args (only the fixed PASS/FAIL
 *   marker needs to be correct).
 */

#include "core/epic_harness.h"
#include "core/pic16_irq.h"
#include "peripherals/pic16f87xa_usart.h"

#include <stdint.h>

#ifndef FOSC_HZ
#define FOSC_HZ 20000000UL
#endif

#define EPIC_HARNESS_SIM_BAUD 9600UL

static uint32_t g_cycles = 0U;

static void s_tx_cplt(void)
{
    /* Non-null so EPIC_USART_Init arms TXEN (pic16f87xa_usart.c); never
     * actually called, transmission below is polled. */
}

static void s_uart_putc(char c)
{
    while (!EPIC_USART_IsTxShiftRegisterEmpty()) {
        /* wait for the shift register to drain */
    }
    EPIC_USART_Transmit((uint8_t)c);
}

/* static: EPIC_USART_Init stores this pointer for the ISR's whole
 * lifetime (pic16f87xa_usart.c's g_usart), so a local here would be a
 * dangling-pointer hazard once this function returns.
 *
 * Pinned to bank 1 (0xA0, 7 bytes) because the USART ISR's deref of
 * g_usart bakes `bcf STATUS,7` (IRP=0, banks 0/1 only), the opposite
 * window from the Timer0 ISR's baked `bsf`. Verified 2026-08-11 by
 * disassembly (epic-serial 16F877A); the unpinned placement scattered
 * to bank 3 in some builds, where the ISR's constant IRP=0 read
 * silently addressed the wrong RAM. */
static USART_HandleTypeDef s_usart_handle EPIC_PLACE(0xA0);

void epic_harness_init(uint32_t cycles)
{
    g_cycles = cycles;

    s_usart_handle = (USART_HandleTypeDef)USART_HANDLE_DEFAULT;
    s_usart_handle.SPBRG = (uint8_t)USART_ComputeSPBRG(
        FOSC_HZ, EPIC_HARNESS_SIM_BAUD, USART_MODE_ASYNCHRONOUS,
        USART_BRGH_HIGH);
    s_usart_handle.TxCpltCallback = s_tx_cplt;
    (void)EPIC_USART_Init(&s_usart_handle);
    /* A non-null TxCpltCallback also enables the TX interrupt source
     * (TXIE), not just TXEN. TXIF is pending immediately after reset
     * and only clears on a TXREG write, so once anything enables GIE
     * (epic_tick_init does), TXIE+pending TXIF fires the ISR forever.
     * Transmission here is polled, never interrupt-driven, so turn the
     * source back off right after Init. */
    EPIC_IRQ_DisableSrc(PIC16_IRQ_USART_TX);
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
