/* End-to-end smoke test for the Timer1 driver on the sim backend.
 * Timer1 counts Fosc/4 at a 1:1 prescaler, so TMR1IF fires every 65536
 * instruction cycles (DS39598F §7.0). Expected register image after
 * Init + Start: T1CON = 0x01 (TMR1ON set, T1CKPS = 00, T1OSCEN = 0,
 * TMR1CS = 0, and this die has no TMR1GE/T1GINV) and TMR1H:TMR1L =
 * 0x0000, the handle's reload value (Register 7-1). */

#include "pic16f818_819_hal.h"
#include "pic16f818_819_sim.h"
#include "pic16f818_819_sfr.h"
#include "peripherals/hal_timer1.h"
#include "core/pic16_irq.h"
#include <stdio.h>

static int g_pass = 0, g_fail = 0;
#define CHECK(c, m) do { if (c) { g_pass++; } else { printf("FAIL: %s\n", m); g_fail++; } } while (0)

#define EXPECTED_OVERFLOWS  3U
/** Cycles between overflows at 1:1 prescaler = 0x10000 = 65536. */
#define OVERFLOW_CYCLES     65536UL
#define SIM_BUDGET          ((OVERFLOW_CYCLES * EXPECTED_OVERFLOWS) + 1024UL)

static volatile uint32_t overflows = 0;

/**
 * @brief Count Timer1 overflows.
 */
static void on_t1_overflow(void)
{
    overflows++;
}

/**
 * @brief Verify the Timer1 register image and overflow cadence on the
 *        sim backend.
 */
int main(void)
{
    pic16f818_819_sim_reset();
    pic16f818_819_sim_set_irq_callback(TIMER1_IRQHandler);

    TIMER1_HandleTypeDef h = TIMER1_HANDLE_DEFAULT;
    h.Prescaler        = TIMER1_PRESCALER_1_1;
    h.ClockSource      = TIMER1_CLOCK_INTERNAL;
    h.ReloadValue      = 0x0000U;
    h.OverflowCallback = on_t1_overflow;

    EPIC_StatusTypeDef st = EPIC_TIMER1_Init(&h);
    CHECK(st == EPIC_OK, "Init returned error");
    EPIC_TIMER1_Start(&h);

    CHECK(EPIC_REG8(PIC_REG_T1CON) == 0x01U, "T1CON != 0x01 after Start");
    CHECK(EPIC_TIMER1_ReadCounter() == 0x0000U, "TMR1H:L != reload value 0x0000");

    for (uint32_t i = 0; i < SIM_BUDGET; i++)
    {
        pic16f818_819_sim_step(1);
        if (overflows >= EXPECTED_OVERFLOWS) break;
    }
    CHECK(overflows >= EXPECTED_OVERFLOWS, "too few overflows inside the budget");

    printf("example_timer1: %d passed, %d failed\n", g_pass, g_fail);
    return (g_fail == 0) ? 0 : 1;
}
