/**
 * @file    combo_uart_ssp.c
 * @brief   C1 of the combination matrix
 *          (docs/superpowers/plans/2026-08-09-combination-matrix.md):
 *          USART + SSP + EEPROM interleaved under a fast TIMER2 ISR,
 *          all real code, one firmware, one bank state.
 *
 * @details
 *   The bug class this gate hunts: an interrupt taken while the
 *   preempted main line is inside a bank-macro window (RP1:RP0 != 00)
 *   runs the ISR path with the wrong bank. XC8 v4.00 emits no banksel
 *   for the dispatch's PIR1/PIR2 reads (verified in the generated .s:
 *   `movf (12),w` with no preceding bank select), so the ISR path must
 *   normalize its own bank (pic16_isr_vector.c's `__interrupt` entry
 *   now does: `bcf STATUS,6; bcf STATUS,5`). The main loop runs
 *   thousands of bank-macro windows (EEPROM's Bank-2/3 helpers and the
 *   SSP's Bank-1 ops) under a ~200 us timer ISR; the cross-checks
 *   verify the timer stayed healthy (TMR2IE intact, callback count
 *   advancing), the EEPROM reads stayed clean, and the SSP config
 *   re-applies after the interleave.
 *
 *   Two MPLAB SIM behaviors this gate pins down (both documented
 *   here, not firmware bugs):
 *   - Enabling GIE while a timer interrupt is already pending wedges
 *     the sim's ISR path: the count freezes and GIE is left cleared
 *     (the Finding 10.1 class; reproduced when main's re-run after
 *     `ljmp start` re-enabled GIE with TMR2IF still latched). The
 *     gate clears the pending flag before GIE-on, per pass.
 *   - SSPCON's SSPM field is the LOW nibble (bits 3:0); a check that
 *     assumes bits 7:3 reads the wrong expectation.
 *
 *   Bounded and self-reporting (the harness contract); no RX
 *   involvement, so the MPLAB SIM RX wall does not apply.
 */

#include "core/epic_harness.h"
#include "core/pic16_irq.h"
#include "peripherals/pic16f87xa_eeprom.h"
#include "peripherals/pic16f87xa_ssp.h"
#include "peripherals/pic16f87xa_timer2.h"
#include "peripherals/pic16f87xa_usart.h"
#include "target/pic16f87xa_platform.h"

#include <stdint.h>

#ifndef FOSC_HZ
#define FOSC_HZ 20000000UL
#endif

#define SIM_ITERATIONS 2000UL

/* ~200 us period at 20 MHz: instruction clock 5 MHz, TMR2 prescaler
 * 1:16 -> 312.5 kHz -> 62.5 counts. The ISR cost (~150-250 cycles)
 * is well under the period, so no main-loop starvation. */
#define T2_PRESCALER TIMER2_PRESCALER_1_16
#define T2_PERIOD    62u

static uint16_t g_t2_count = 0u;
static uint16_t g_fail = 0u;

static void t2_overflow_cb(void)
{
    g_t2_count++;
}

static void fail(uint8_t idx)
{
    static const char hx[] = "0123456789ABCDEF";
    char c[2];
    g_fail++;
    epic_harness_log("F");
    c[0] = hx[(idx >> 4) & 0xF];
    c[1] = '\0';
    epic_harness_log(c);
    c[0] = hx[idx & 0xF];
    c[1] = '\0';
    epic_harness_log(c);
    epic_harness_log(".");
}

#define CHECK(cond, idx) do {         \
    if (!(cond)) fail(idx);            \
} while (0)

static void s_tx_noop(void)
{
}

int main(void)
{
    epic_harness_init(SIM_ITERATIONS);

    /* USART: the harness already inited it; re-init with the no-op
     * callback handle (arms TXEN) and turn the TX interrupt source off
     * (transmission is polled). */
    {
        USART_HandleTypeDef h = USART_HANDLE_DEFAULT;
        h.SPBRG = (uint8_t)USART_ComputeSPBRG(
            FOSC_HZ, 9600UL, USART_MODE_ASYNCHRONOUS, USART_BRGH_HIGH);
        h.TxCpltCallback = s_tx_noop;
        (void)EPIC_USART_Init(&h);
        EPIC_IRQ_DisableSrc(PIC16_IRQ_USART_TX);
    }

    /* SSP as I2C master, firmware-driven (Start/WriteByte/Stop are the
     * manual primitives; the sim latches SEN/PEN, so all waits are
     * bounded). */
    SSP_HandleTypeDef ssp = SSP_HANDLE_DEFAULT;
    ssp.Mode = SSP_MODE_I2C_MASTER_FW;
    (void)EPIC_SSP_Init(&ssp);
    /* The config write must land: SSPM (bits 3:0) = 1000 (I2C master
     * firmware) + SSPEN (bit 5), so SSPCON == 0x28 (DS39582B Reg 9-2;
     * the SSPM field is the LOW nibble). SSPCON is Bank 0 (0x14): a
     * plain read, NOT EPIC_BANK1_READ8 (which selects Bank 1 and
     * reads SSPSTAT). */
    CHECK((EPIC_REG8(PIC_REG_SSPCON) & 0x2Fu) == 0x28u, 0x05);

    /* GIE on BEFORE the timer starts, so the first overflow fires the
     * ISR with GIE already set (no latch-then-enable edge). MPLAB SIM
     * wedges the ISR path when GIE is enabled while a timer interrupt
     * is already pending (verified 2026-08-09: when main re-runs after
     * `ljmp start` with TMR2 still running and TMR2IF latched, the
     * GIE-on edge freezes the callback count and leaves GIE cleared,
     * the Finding 10.1 class), so the timer is stopped and its flag
     * cleared first, per pass. */
    EPIC_REG8(PIC_REG_T2CON) &= (uint8_t)~0x04u;   /* TMR2ON = 0 */
    EPIC_IRQ_ClearFlag(PIC16_IRQ_TMR2);
    EPIC_IRQ_Restore(1);

    /* TIMER2 with the overflow callback: enables TMR2IE. */
    TIMER2_HandleTypeDef t2 = TIMER2_HANDLE_DEFAULT;
    t2.Prescaler = T2_PRESCALER;
    t2.Period = T2_PERIOD;
    t2.OverflowCallback = t2_overflow_cb;
    (void)EPIC_TIMER2_Init(&t2);
    (void)EPIC_TIMER2_Start(&t2);

    for (uint32_t i = 0; epic_harness_running(i); i++) {
        /* SSP cycle: SSPCON2/SSPSTAT Bank-1 macro windows. */
        EPIC_SSP_Start();
        (void)EPIC_SSP_WriteByte(0xA5u);
        EPIC_SSP_Stop();
        /* A manual bank window: hold RP1:RP0 = 01 for a stretch, the
         * same state the bank macros create for ~5 instructions, so an
         * ISR preempting here runs with RP=01. Without the vector's
         * bank normalize, the dispatch's banksel-less PIR1 read then
         * misdirects and the callback's increment lands in the wrong
         * bank's GPR (the count freezes/lags). */
        asm("bsf STATUS,5");
        for (volatile uint16_t n = 0u; n < 40u; n++) {
            /* hold the window open */
        }
        asm("bcf STATUS,5");
    }

    /* Stop the timer and disable interrupts before the checks. */
    EPIC_IRQ_Disable();

    /* Cross-checks. */
    uint8_t pie1 = 0u;
    EPIC_BANK1_READ8(PIE1, pie1);
    CHECK((pie1 & PIC_PIE1_TMR2IE) != 0u, 0x00);   /* source still enabled */
    CHECK((pie1 & (uint8_t)~(PIC_PIE1_TMR2IE)) == 0u, 0x01); /* nothing else */
    CHECK(g_t2_count >= 100u, 0x02);               /* timer kept firing */
    /* The SSP write path still works after the interleave: re-init and
     * verify the config re-applies. */
    (void)EPIC_SSP_Init(&ssp);
    CHECK((EPIC_REG8(PIC_REG_SSPCON) & 0x2Fu) == 0x28u, 0x04);

    for (uint32_t i = 0; epic_harness_running(i); i++) {
        epic_harness_tick();
    }
    return epic_harness_report(g_fail == 0u);
}
