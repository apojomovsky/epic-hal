/**
 * Timer2/Timer4/Timer6 all run at once, each overflow ISR toggles a
 * distinct RC pin, exercising all three instance-dispatch branches in
 * one binary. Three independent T*CON configurations (different
 * prescaler, postscaler, and PR value each) so the three instances
 * overflow at visibly different rates, proving the driver's per-instance
 * state doesn't cross-contaminate:
 *
 *   Timer2: 1:1 prescaler, 1:1 postscaler, PR2=199  -> 200 cycles/flag
 *   Timer4: 1:4 prescaler, 1:1 postscaler, PR4=124  -> 500 cycles/flag
 *   Timer6: 1:16 prescaler, 1:4 postscaler, PR6=49  -> 3200 cycles/flag
 *
 * Expected register image (host sim, after Start() x3, before the main
 * loop runs):
 *   T2CON  = 0x04   (TOUTPS=0000, TMR2ON=1, T2CKPS=00)
 *   PR2    = 0xC7   (199)
 *   T4CON  = 0x05   (TOUTPS=0000, TMR4ON=1, T4CKPS=01)
 *   PR4    = 0x7C   (124)
 *   T6CON  = 0x1E   (TOUTPS=0011, TMR6ON=1, T6CKPS=10)
 *   PR6    = 0x31   (49)
 *   PIE1   = 0x02   (TMR2IE, bit 1)
 *   PIE3   = 0x0A   (TMR4IE bit 1 | TMR6IE bit 3)
 *   INTCON = 0xC0   (GIE=1, PEIE=1, TMR0IE=0)
 *   TRISC  = 0xF8   (RC0/RC1/RC2 output, rest input; PORTC has no
 *                    ANSELC on this family, all digital)
 *   LATC   = 0x00   (all three start low)
 *   TRISA  = 0xFE   (RA0 output, driven by the HARNESS=sim harness)
 *   LATA   = 0x00   (start low, flipped to 0x01 on PASS)
 *
 * Loop termination and the PASS marker: the loop exits early once all
 * three instances have overflowed at least MIN_OVERFLOWS times. On the
 * host sim the loop exits this way and epic_harness_report() drives RA0
 * high. On the MPLAB SIM target, three timers firing continuously
 * consume most of the simulated CPU in ISRs, so the bounded loop may
 * not reach epic_harness_report() within the 60s wall-clock `wait`; to
 * make the PASS marker reliable under the gpio gate regardless, the
 * slowest instance's ISR (Timer6) also drives RA0 high directly once
 * all three counts reach the threshold. Both paths set RA0=1 on PASS.
 *
 * SIM_CYCLES = 20000 gives approximately Timer2=100, Timer4=40,
 * Timer6=6 overflows on the host sim. Pass condition: all three toggle
 * counts >= MIN_OVERFLOWS (2).
 */

#include "pic16f193x.h"
#include "pic16f193x_sfr.h"
#include "peripherals/pic16f193x_gpio.h"
#include "peripherals/pic16f193x_timer246.h"
#include "core/pic16f193x_irq.h"
#include "core/pic16f193x_wdt_sleep.h"
#include "core/epic_harness.h"

/** Family-local harness extension (see example_timer1.c): no-op on the
 *  CMake host build, freezes in a tight loop on the mdb-under-MPLAB-SIM
 *  build so the HARNESS=sim marker's RA0 stays set across the mdb
 *  `print PORTA` readback. */
extern void pic16f193x_harness_halt(void);

#ifndef FOSC_HZ
#define FOSC_HZ 32000000UL
#endif

/** Simulated run length (host only). 20000 gives every instance several
 *  overflows within one bounded run (see the file header table for the
 *  per-instance cycles/flag math). */
#define SIM_CYCLES  20000UL

/** Minimum overflows per instance before the loop exits early and
 *  before the ISR-driven PASS marker fires. Matches the pass
 *  threshold. */
#define MIN_OVERFLOWS 2U

/* Toggle counts, each ISR is the only writer of its own slot. */
static volatile uint32_t g_toggle_count[3] = { 0, 0, 0 };

/* PASS marker driven from ISR context on the target so the gpio gate
 * sees RA0=1 even when the main loop is starved by the three
 * continuously-firing timer ISRs on MPLAB SIM (see file header). */
static volatile uint8_t g_pass_marker_set = 0U;

static void on_t2_overflow(void)
{
    EPIC_GPIO_TogglePin(GPIOC, GPIO_PIN_0);
    g_toggle_count[0]++;
}

static void on_t4_overflow(void)
{
    EPIC_GPIO_TogglePin(GPIOC, GPIO_PIN_1);
    g_toggle_count[1]++;
}

static void on_t6_overflow(void)
{
    EPIC_GPIO_TogglePin(GPIOC, GPIO_PIN_2);
    g_toggle_count[2]++;
    /* Timer6 is the slowest instance (3200 cycles/flag), so once it
     * has fired MIN_OVERFLOWS times, all three have. Drive RA0 high
     * here so the MODE=gpio gate sees PASS even if the main loop
     * cannot reach epic_harness_report within the MPLAB SIM wait
     * window while three timer ISRs fire continuously. */
    if (!g_pass_marker_set &&
        g_toggle_count[0] >= MIN_OVERFLOWS &&
        g_toggle_count[1] >= MIN_OVERFLOWS &&
        g_toggle_count[2] >= MIN_OVERFLOWS) {
        EPIC_GPIO_WritePin(GPIOA, GPIO_PIN_0, GPIO_PIN_SET);
        g_pass_marker_set = 1U;
    }
}

int main(void)
{
    epic_harness_init(SIM_CYCLES);

    /* 1. RA0 as digital output (PASS/FAIL marker pin, driven by the
     *    harness's log() on the host and by on_t6_overflow on the
     *    target). RC0/RC1/RC2 as digital outputs, start low. */
    EPIC_GPIO_Init(GPIOA, GPIO_PIN_0, GPIO_MODE_OUTPUT);
    EPIC_GPIO_WritePin(GPIOA, GPIO_PIN_0, GPIO_PIN_RESET);
    EPIC_GPIO_Init(GPIOC, GPIO_PIN_0 | GPIO_PIN_1 | GPIO_PIN_2, GPIO_MODE_OUTPUT);
    EPIC_GPIO_WritePin(GPIOC, GPIO_PIN_0 | GPIO_PIN_1 | GPIO_PIN_2, GPIO_PIN_RESET);

    /* 2. Timer2: 1:1 prescaler, 1:1 postscaler, PR2=199. */
    TIMER246_HandleTypeDef h2 = TIMER246_HANDLE_DEFAULT;
    h2.Instance         = TIMER246_INSTANCE_2;
    h2.Prescaler        = TIMER246_PRESCALER_1_1;
    h2.Postscaler       = TIMER246_POSTSCALER_1_1;
    h2.Period           = 199U;
    h2.OverflowCallback = on_t2_overflow;
    EPIC_TIMER246_Init(&h2);
    EPIC_TIMER246_Start(&h2);

    /* 3. Timer4: 1:4 prescaler, 1:1 postscaler, PR4=124. */
    TIMER246_HandleTypeDef h4 = TIMER246_HANDLE_DEFAULT;
    h4.Instance         = TIMER246_INSTANCE_4;
    h4.Prescaler        = TIMER246_PRESCALER_1_4;
    h4.Postscaler       = TIMER246_POSTSCALER_1_1;
    h4.Period           = 124U;
    h4.OverflowCallback = on_t4_overflow;
    EPIC_TIMER246_Init(&h4);
    EPIC_TIMER246_Start(&h4);

    /* 4. Timer6: 1:16 prescaler, 1:4 postscaler, PR6=49. */
    TIMER246_HandleTypeDef h6 = TIMER246_HANDLE_DEFAULT;
    h6.Instance         = TIMER246_INSTANCE_6;
    h6.Prescaler        = TIMER246_PRESCALER_1_16;
    h6.Postscaler       = TIMER246_POSTSCALER_1_4;
    h6.Period           = 49U;
    h6.OverflowCallback = on_t6_overflow;
    EPIC_TIMER246_Init(&h6);
    EPIC_TIMER246_Start(&h6);

    /* 5. Arm the global interrupt (each Init already enabled its own
     *    PIE bit since each handle has an OverflowCallback). */
    EPIC_IRQ_Restore(1);

    /* 6. Let time pass. Bounded on host, busy-spins refreshing the WDT
     *    on target. Exits early once all three instances have
     *    overflowed at least MIN_OVERFLOWS times; on the target the
     *    ISR-driven PASS marker (on_t6_overflow) may fire first. */
    for (uint32_t i = 0; epic_harness_running(i); i++) {
        epic_harness_tick();
        EPIC_WDT_Refresh();
        if (g_toggle_count[0] >= MIN_OVERFLOWS &&
            g_toggle_count[1] >= MIN_OVERFLOWS &&
            g_toggle_count[2] >= MIN_OVERFLOWS) {
            break;
        }
    }

    epic_harness_log("Timer2/4/6 toggled %u/%u/%u times.\n",
                      (unsigned)g_toggle_count[0],
                      (unsigned)g_toggle_count[1],
                      (unsigned)g_toggle_count[2]);
    int pass = (g_toggle_count[0] >= MIN_OVERFLOWS) &&
               (g_toggle_count[1] >= MIN_OVERFLOWS) &&
               (g_toggle_count[2] >= MIN_OVERFLOWS);
    int rc = epic_harness_report(pass);
    /* Freeze here so the HARNESS=sim marker's RA0 stays set across the
     * mdb `print PORTA` readback, same rationale as example_timer1.c. */
    pic16f193x_harness_halt();
    return rc;
}
