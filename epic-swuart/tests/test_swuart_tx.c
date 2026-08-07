/**
 * @file    test_swuart_tx.c
 * @brief   TX-only host test, v2: pumps epic_harness_tick() until the
 *          real simulated Timer1 actually overflows (event-driven, not
 *          a fixed CYCLES_PER_TICK loop count), capturing the bit
 *          sequence driven onto the TX pin via *_sim_read_output.
 */
#include <assert.h>
#include <stdio.h>
#include "epic_swuart.h"
#include "core/epic_harness.h"

#if defined(PIC18F2455) || defined(PIC18F2550) || defined(PIC18F4455) || defined(PIC18F4550)
  #include "pic18fxx5x_sim.h"
  #define SIM_READ(port, pin) pic18_sim_read_output((port), (pin))
#elif defined(PIC16F1933) || defined(PIC16F1934) || defined(PIC16F1936) || \
      defined(PIC16F1937) || defined(PIC16F1938) || defined(PIC16F1939)
  #include "pic16f193x_sim.h"
  #define SIM_READ(port, pin) pic16f193x_sim_read_output((port), (pin))
#else
  #include "pic16f87xa_sim.h"
  #define SIM_READ(port, pin) pic16f87xa_sim_read_output((port), (pin))
#endif

static int g_fails = 0;
#define CHECK(c, m) do { if (!(c)) { epic_harness_log("FAIL: %s\n", m); g_fails++; } } while (0)

/* Runs ticks until n Timer1 overflows have happened, bounded so a bug
 * that stops the timer can't hang the test forever.
 *
 * A tick period elapsed once EPIC_TIMER1_ReadCounter wraps past where
 * it started (the scheduler re-arms Timer1 to a low countdown value
 * immediately on overflow, so the counter reading drops instead of
 * continuing to climb). A fixed generous-raw-cycle-count version was
 * tried first (round(FOSC_HZ/4/baud) rounded down, plus a margin), but
 * a throwaway probe against the sim showed that margin (5 cycles) is
 * itself enough to overshoot the real 521-cycle bit period every call,
 * drifting the sample point into the next bit within a couple of bit
 * periods. Detecting the actual wrap keeps every sample locked to its
 * real bit boundary regardless of FOSC_HZ/baud rounding. */
static void run_bit_periods(uint32_t n, uint32_t max_ticks)
{
    for (uint32_t i = 0; i < n; i++) {
        uint16_t prev = EPIC_TIMER1_ReadCounter();
        uint32_t t;
        for (t = 0; t < max_ticks; t++) {
            epic_harness_tick();
            uint16_t now = EPIC_TIMER1_ReadCounter();
            if (now < prev) break; /* wrapped: this bit period elapsed */
            prev = now;
        }
    }
}

int main(void)
{
    epic_harness_init(2000000UL);

    EPIC_SWUART_HandleTypeDef h;
    EPIC_StatusTypeDef st = EPIC_SWUART_Init(&h, GPIOB, GPIO_PIN_0,
                                              GPIOB, GPIO_PIN_4,
                                              FOSC_HZ, 9600u);
    CHECK(st == EPIC_OK, "init ok");

    size_t queued = EPIC_SWUART_Write(&h, (const uint8_t *)"A", 1);
    CHECK(queued == 1u, "queued one byte");

    /* 'A' = 0x41 = 0b01000001. LSB first: start=0, d0=1, d1..d5=0,
     * d6=1, d7=0, stop=1. */
    static const uint8_t expected[] = {0, 1, 0, 0, 0, 0, 0, 1, 0, 1};
    uint8_t observed[10];

    for (size_t bit = 0; bit < 10; bit++) {
        run_bit_periods(1, 2000);
        observed[bit] = SIM_READ('B', 0);
    }

    for (size_t bit = 0; bit < 10; bit++) {
        char msg[32];
        snprintf(msg, sizeof(msg), "bit %u", (unsigned)bit);
        CHECK(observed[bit] == expected[bit], msg);
    }

    epic_harness_log("swuart_tx: fails=%d\n", g_fails);
    return epic_harness_report(g_fails == 0);
}
