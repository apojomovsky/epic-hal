/**
 * @file    test_swuart_tx.c
 * @brief   TX-only host test: enqueue a byte, pump ticks, capture the
 *          bit sequence driven onto the TX pin via the family sim's
 *          `*_sim_read_output`, check it against 8N1 framing for 'A'
 *          (0x41 = 0b01000001, LSB first: start=0, 1,0,0,0,0,0,1,0,
 *          stop=1).
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

/* N=3, per docs/superpowers/plans/probe-swuart-isr-budget.md: the
 * straight-line probe measured N=4 as technically reachable (122/130
 * cycles, 6.2% margin) but that margin was measured on a minimal
 * snapshot without ring-buffer or parameter-passing overhead. N=3 has
 * a comfortable 29.9% margin (122/174) and is the production default. */
#define OVERSAMPLE_N 3u

static int g_fails = 0;
#define CHECK(c, m) do { if (!(c)) { epic_harness_log("FAIL: %s\n", m); g_fails++; } } while (0)

int main(void)
{
    epic_harness_init(2000000UL);

    EPIC_SWUART_HandleTypeDef h;
    EPIC_StatusTypeDef st = EPIC_SWUART_Init(&h, GPIOB, GPIO_PIN_0,
                                              GPIOB, GPIO_PIN_2,
                                              FOSC_HZ, 9600u);
    CHECK(st == EPIC_OK, "init ok");

    size_t queued = EPIC_SWUART_Write(&h, (const uint8_t *)"A", 1);
    CHECK(queued == 1u, "queued one byte");

    /* 'A' = 0x41 = 0b01000001. LSB first over the wire: bit0=1, bit1=0,
     * bit2=0, bit3=0, bit4=0, bit5=0, bit6=1, bit7=0. Ten sampled bit
     * periods, in order: start, d0..d7, stop. The first epic_harness_tick()
     * already fires the IDLE-to-first-bit transition (tx_ticks_left starts
     * at 0), so iteration 0 below samples the start bit, not idle. */
    static const uint8_t expected[] = {0, 1, 0, 0, 0, 0, 0, 1, 0, 1};
    uint8_t observed[10];

    for (size_t bit = 0; bit < 10; bit++) {
        /* Sample mid-bit: run half the bit's ticks, sample, run the rest. */
        for (uint32_t t = 0; t < OVERSAMPLE_N / 2u; t++) epic_harness_tick();
        observed[bit] = SIM_READ('B', 0);
        for (uint32_t t = OVERSAMPLE_N / 2u; t < OVERSAMPLE_N; t++) epic_harness_tick();
    }

    for (size_t bit = 0; bit < 10; bit++) {
        char msg[32];
        snprintf(msg, sizeof(msg), "bit %u", (unsigned)bit);
        CHECK(observed[bit] == expected[bit], msg);
    }

    epic_harness_log("swuart_tx: fails=%d\n", g_fails);
    return epic_harness_report(g_fails == 0);
}
