/**
 * @file    pic16_harness_sim_target.c
 * @brief   PIC16F87XA sim-target implementation of the test harness (see
 *          core/pic8_harness.h). Runs as real compiled firmware under
 *          MPLAB SIM (driven headlessly via mdb, see docs/ci-plan.md
 *          Phase 2), not a host-side simulator like pic16_harness_sim.c.
 *
 * @details
 *   Deliberately NOT in pic8-common alongside pic8_harness_target.c: that
 *   file is genuinely architecture-blind (four no-ops), this one is not,
 *   it needs real USART SFR access to make pic8_harness_report's marker
 *   line (see core/pic8_harness.h) reach mdb's uart1io capture. Same
 *   split this family already uses for its host build's harness
 *   (pic16_harness_sim.c lives here, not in pic8-common, for the same
 *   reason: it has to touch this family's simulator model).
 *
 *   init/tick are still no-ops, same as the plain real-target build:
 *   MPLAB SIM executes real instructions, hardware time still advances
 *   on its own, there is nothing to pump. pic8_harness_running is
 *   bounded by the `cycles` argument (like the host build), not always 1
 *   (like the plain target build): a sim-target run has to actually
 *   terminate on its own for mdb's script to get a result without
 *   relying on its `wait` timeout as the only signal (docs/ci-plan.md
 *   Phase 3's validation checklist).
 *
 *   pic8_harness_log here is a byte-for-byte writer, not a real printf:
 *   it walks `fmt` and transmits its raw characters, ignoring any
 *   variadic arguments entirely. Deliberate, not an oversight: a real
 *   vsnprintf-over-USART would cost flash on every module that opts into
 *   HARNESS=sim, to serve a wire format that only ever needs to carry
 *   pic8_harness_report's own fixed, no-format marker line reliably.
 *   Existing example code that logs with real conversion specifiers
 *   (e.g. "tick: delay(10) -> %lu ms\n") still compiles and runs, but
 *   the captured UART stream shows the literal unexpanded format string
 *   for those calls, not substituted values; only the harness's own
 *   PASS/FAIL marker is guaranteed correct, which is the only thing
 *   CI's grep depends on.
 *
 *   HAL_USART_Init's TXEN (and, here, also TXIE) bits only get set when
 *   a non-null TxCpltCallback is supplied (pic16f87xa_usart.c, a known,
 *   separately-documented bug, see docs/ci-plan.md's open questions).
 *   Worked around with a genuinely no-op callback. TXIE ending up
 *   enabled is harmless: this file never calls HAL_IRQ_GlobalEnable, and
 *   even if a later peripheral init does (pic8_tick_init, for this
 *   pilot module), the resulting ISR just calls the no-op callback and
 *   returns (see pic16f87xa_usart.c's USART_TX_IRQHandler). Transmission
 *   itself is polled (HAL_USART_IsTxShiftRegisterEmpty), not
 *   interrupt-driven, so none of this actually matters for correctness,
 *   only for not being surprised by an ISR firing.
 */

#include "core/pic8_harness.h"
#include "peripherals/pic16f87xa_usart.h"

#include <stdint.h>

#ifndef FOSC_HZ
#define FOSC_HZ 20000000UL
#endif

#define PIC8_HARNESS_SIM_BAUD 9600UL

static uint32_t g_cycles = 0U;

static void s_tx_cplt(void)
{
    /* Exists only so HAL_USART_Init's TXEN/TXIE gate sees a non-null
     * callback (see this file's header comment). Transmission below is
     * polled, this is never actually called in a way that matters. */
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

    USART_HandleTypeDef h = USART_HANDLE_DEFAULT;
    h.SPBRG = (uint8_t)USART_ComputeSPBRG(FOSC_HZ, PIC8_HARNESS_SIM_BAUD,
                                           USART_MODE_ASYNCHRONOUS,
                                           USART_BRGH_HIGH);
    h.TxCpltCallback = s_tx_cplt;
    (void)HAL_USART_Init(&h);
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
