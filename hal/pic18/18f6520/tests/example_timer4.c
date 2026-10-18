/*
 * Timer4 match smoke, the §4 gate's host-sim and mdb half for the Timer4
 * driver. One source builds for host sim and real XC8 target via
 * `core/epic_harness.h`. Timer4 8-bit, 1:16 prescaler, 1:1 postscaler,
 * PR4=0x7F: a PR4 match (TMR4IF in PIR3) every 2048 cycles. The period
 * is long on purpose: a fast match makes the ISR re-fire continuously
 * and starves the main loop under MPLAB SIM (docs/adding-a-device.md §4).
 */

#include "pic18f6520_hal.h"
#include "pic18f6520_sfr.h"
#include "peripherals/pic18f6520_gpio.h"
#include "peripherals/pic18f6520_timer4.h"
#include "core/pic18_irq.h"
#include "core/epic_harness.h"

/** @brief Family-local harness extension: no-op on the host build,
 *  infinite loop on the mdb build (see example_irq.c). */
extern void pic18f6520_harness_halt(void);

/** Simulated run length (host only), also the bounded spin on target.
 * 2048 cycles per match, so 20000 cycles give ~9 matches. */
#define SIM_CYCLES  20000UL

/** Matches that must be seen before the timer is stopped and PASS is
 * driven. */
#define WANT_MATCHES  2U

/* Match count, the ISR is the only writer. */
static volatile uint32_t g_match_count = 0;

/** @brief  Timer4 match callback.
 *
 *          Runs in interrupt context (target) or the sim IRQ callback
 *          (host). Counts the match; on the last wanted one it stops
 *          Timer4 (so the ISR cannot starve the main loop) and drives
 *          the RA0 PASS marker directly, the ISR-driven-marker pattern
 *          from docs/adding-a-device.md §4 step 6.
 */
static void on_t4_match(void)
{
    g_match_count++;
    if (g_match_count >= WANT_MATCHES)
    {
        (void)EPIC_TIMER4_Stop();
        EPIC_GPIO_WritePin(GPIOA, GPIO_PIN_0, GPIO_PIN_SET);
    }
}

/**
 * @brief  Count Timer4 PR4 matches within a bounded spin.
 *
 *          Starts Timer4, reads back T4CON/PR4/PIE3 for the programmed
 *          values, then spins until the ISR has counted WANT_MATCHES
 *          matches or the bound runs out.
 */
int main(void)
{
    epic_harness_init(SIM_CYCLES);

    /* Timer4: 1:16 prescaler, 1:1 postscaler, PR4=0x7F, count each match. */
    TIMER4_HandleTypeDef h = TIMER4_HANDLE_DEFAULT;
    h.Prescaler        = TIMER4_PRESCALER_1_16;
    h.Postscaler       = TIMER4_POSTSCALER_1_1;
    h.Period           = 0x7FU;
    h.OverflowCallback = on_t4_match;
    EPIC_TIMER4_Init(&h);
    EPIC_TIMER4_Start(&h);

    /* High vector, the path the mdb-proven gates use (see example_irq.c);
     * then arm the masters (also sets IPEN). */
    EPIC_IRQ_SetPriority(PIC18_IRQ_TMR4, EPIC_IRQ_PRIORITY_HIGH);
    EPIC_IRQ_Restore(1);

    /* Expected image: T4CON = 0x06 (TMR4ON, 1:16, 1:1 post), PR4 = 0x7F
     * (not the 0xFF POR value, so the write is visible), PIE3<TMR4IE>. */
    uint8_t t4con_ok = (epic_sfr_read8(PIC_REG_T4CON) == 0x06U) ? 1U : 0U;
    uint8_t pr4_ok   = (epic_sfr_read8(PIC_REG_PR4)   == 0x7FU) ? 1U : 0U;
    uint8_t en_ok    =
        (epic_sfr_read8(PIC_REG_PIE3) & PIC_PIE3_TMR4IE) ? 1U : 0U;

    /* Bounded spin: on the host the harness pumps one sim cycle per
     * iteration, on target time passes on its own. */
    for (uint32_t i = 0; epic_harness_running(i); i++)
    {
        epic_harness_tick();
        if (g_match_count >= WANT_MATCHES) break;
    }

    uint8_t fired_ok = (g_match_count >= WANT_MATCHES) ? 1U : 0U;

    epic_harness_log("Timer4: t4con=%u pr4=%u en=%u matches=%u\n",
                     (unsigned)t4con_ok, (unsigned)pr4_ok,
                     (unsigned)en_ok, (unsigned)g_match_count);
    int rc = epic_harness_report(t4con_ok && pr4_ok && en_ok && fired_ok);

    /* Freeze so RA0 keeps the report value across the mdb readback. */
    pic18f6520_harness_halt();
    return rc;
}
