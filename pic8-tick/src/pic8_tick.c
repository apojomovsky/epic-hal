/**
 * @file    pic8_tick.c
 * @brief   1 ms timebase on Timer2, family-agnostic.
 *
 * @details
 *   The tick ISR is the Timer2 handle's `OverflowCallback` -- the HAL already
 *   owns a strong `TIMER2_IRQHandler` that clears TMR2IF and calls the
 *   callback, so this module does NOT redefine the handler (that would be a
 *   multiple-definition error). The handle is `static` because the PIC16
 *   Timer2 driver stores the caller's pointer (not a copy), so it must
 *   outlive the ISR; the PIC18 driver copies the handle, so static is safe
 *   there too.
 *
 *   Period math: TMR2IF fires every `prescaler * (PR2+1) * postscaler`
 *   instruction cycles; one instruction cycle = Fosc/4. For a 1 ms tick we
 *   need `Fosc/4000` instruction cycles. `compute_period()` searches the
 *   prescaler {1,4,16} x postscaler {1..16} x (PR2+1) {1..256} space for the
 *   configuration whose product is closest to the target; for the common
 *   Fosc values it is exact (e.g. 20 MHz -> pre 1:4, PR2 249, post 1:5 =
 *   4*250*5 = 5000; 48 MHz -> pre 1:16, PR2 249, post 1:3 = 16*250*3 = 12000).
 *
 *   `pic8_tick_get()` disables interrupts around the 32-bit read (an 8-bit
 *   core reads it in 4 bytes; the ISR could update it mid-read). On the host
 *   sim the ISR fires synchronously inside `pic8_harness_tick()`, so the
 *   disable/restore is harmless there. `pic8_tick_delay_ms()` pumps
 *   `pic8_harness_tick()` while waiting so simulated time advances on host;
 *   on target that call is a no-op and the real Timer2 ISR advances the
 *   counter.
 */

#include "pic8_tick.h"
#include "peripherals/hal_timer2.h"   /* family-neutral shim -> pic*_timer2.h */
#include "core/hal_irq.h"             /* HAL_IRQ_Disable / Restore            */
#include "core/pic8_harness.h"        /* pic8_harness_tick (host sim pump)    */

static volatile uint32_t g_tick_ms = 0u;
static TIMER2_HandleTypeDef s_timer2 = TIMER2_HANDLE_DEFAULT;

static void pic8_tick_on_overflow(void)
{
    g_tick_ms++;
}

/* Pick the Timer2 configuration whose period is closest to 1 ms. */
static void compute_period(uint32_t fosc_hz, uint8_t *pr2,
                           TIMER2_PrescalerTypeDef *pre,
                           TIMER2_PostscalerTypeDef *post)
{
    uint32_t target = fosc_hz / 4000u;       /* instruction cycles per 1 ms */
    if (target == 0u) { target = 1u; }

    static const TIMER2_PrescalerTypeDef pre_enum[3] = {
        TIMER2_PRESCALER_1_16, TIMER2_PRESCALER_1_4, TIMER2_PRESCALER_1_1 };
    static const uint16_t pre_ratio[3] = { 16u, 4u, 1u };

    uint32_t best_err = 0xFFFFFFFFu;
    uint8_t best_pr2 = 0xFFu;
    TIMER2_PrescalerTypeDef best_pre = TIMER2_PRESCALER_1_1;
    TIMER2_PostscalerTypeDef best_post = TIMER2_POSTSCALER_1_1;

    for (int i = 0; i < 3; i++) {
        uint32_t p = pre_ratio[i];
        for (uint32_t q = 1u; q <= 16u; q++) {
            uint32_t pq = p * q;
            uint32_t n = target / pq;            /* n = PR2+1 */
            /* try n and n+1 (the floor and ceil of target/pq), clamped 1..256 */
            for (uint32_t k = 0u; k < 2u; k++) {
                uint32_t nn = n + k;
                if (nn < 1u || nn > 256u) { continue; }
                uint32_t cand = pq * nn;
                uint32_t err = (cand >= target) ? (cand - target) : (target - cand);
                if (err < best_err) {
                    best_err  = err;
                    best_pr2  = (uint8_t)(nn - 1u);
                    best_pre  = pre_enum[i];
                    best_post = (TIMER2_PostscalerTypeDef)(q - 1u);
                }
            }
        }
    }
    *pr2  = best_pr2;
    *pre  = best_pre;
    *post = best_post;
}

void pic8_tick_init(uint32_t fosc_hz)
{
    uint8_t pr2;
    TIMER2_PrescalerTypeDef pre;
    TIMER2_PostscalerTypeDef post;
    compute_period(fosc_hz, &pr2, &pre, &post);

    g_tick_ms = 0u;
    s_timer2 = (TIMER2_HandleTypeDef)TIMER2_HANDLE_DEFAULT;
    s_timer2.Prescaler        = pre;
    s_timer2.Postscaler       = post;
    s_timer2.Period           = pr2;
    s_timer2.OverflowCallback = pic8_tick_on_overflow;
    HAL_TIMER2_Init(&s_timer2);
    HAL_TIMER2_Start(&s_timer2);
    /* HAL_TIMER2_Init only arms Timer2's own source enable bit
     * (pic8-common/MANUAL.md §6-7); the chip's global interrupt enable
     * is separate and nothing else here ever turned it on. Without this,
     * the Timer2 ISR never fires on real hardware (or a real
     * instruction-accurate simulator), g_tick_ms never increments, and
     * pic8_tick_delay_ms spins forever. Invisible on the host build,
     * whose pic8_harness_tick() steps the simulator directly instead of
     * relying on a real interrupt; only surfaced once this module's
     * compiled firmware actually ran under MPLAB SIM for the first time
     * (docs/ci-plan.md Phase 4).
     */
    HAL_IRQ_Restore(1);
}

uint32_t pic8_tick_get(void)
{
    uint8_t prev = HAL_IRQ_Disable();          /* atomic 32-bit read          */
    uint32_t t = g_tick_ms;
    HAL_IRQ_Restore(prev);
    return t;
}

uint32_t pic8_tick_elapsed_since(uint32_t t0)
{
    return pic8_tick_get() - t0;               /* unsigned: wraparound-safe   */
}

void pic8_tick_delay_ms(uint32_t ms)
{
    uint32_t t0 = pic8_tick_get();
    while (pic8_tick_elapsed_since(t0) < ms) {
        pic8_harness_tick();                   /* host: pump sim; target: no-op */
    }
}
