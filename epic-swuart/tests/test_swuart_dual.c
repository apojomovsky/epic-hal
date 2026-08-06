/**
 * @file    test_swuart_dual.c
 * @brief   Two channels active at once: channel A transmits while
 *          channel B receives a synthesised byte, in the same shared
 *          Timer1 tick, proving the registry loop in shared_tick()
 *          services both without cross-talk.
 */
#include <stdio.h>
#include "epic_swuart.h"
#include "core/epic_harness.h"

#if defined(PIC18F2455) || defined(PIC18F2550) || defined(PIC18F4455) || defined(PIC18F4550)
  #include "pic18fxx5x_sim.h"
  #define SIM_DRIVE(port, pin, lvl) pic18_sim_drive_input((port), (pin), (lvl))
  #define SIM_READ(port, pin) pic18_sim_read_output((port), (pin))
#elif defined(PIC16F1933) || defined(PIC16F1934) || defined(PIC16F1936) || \
      defined(PIC16F1937) || defined(PIC16F1938) || defined(PIC16F1939)
  #include "pic16f193x_sim.h"
  #define SIM_DRIVE(port, pin, lvl) pic16f193x_sim_drive_input((port), (pin), (lvl))
  #define SIM_READ(port, pin) pic16f193x_sim_read_output((port), (pin))
#else
  #include "pic16f87xa_sim.h"
  #define SIM_DRIVE(port, pin, lvl) pic16f87xa_sim_drive_input((port), (pin), (lvl))
  #define SIM_READ(port, pin) pic16f87xa_sim_read_output((port), (pin))
#endif

#define OVERSAMPLE_N 3u
#define BAUD 9600u

/* epic_harness_tick() advances one instruction cycle per call (see
 * core/epic_harness.h); a real Timer1 tick takes CYCLES_PER_TICK such
 * cycles, the same value epic_swuart.c's compute_reload() computes
 * internally. run_ticks() below advances in units of software UART
 * ticks, not raw cycles: see test_swuart_tx.c's version of this same
 * comment for why the distinction matters. */
#define CYCLES_PER_TICK \
    ((uint32_t)((FOSC_HZ / 4u + ((uint32_t)BAUD * OVERSAMPLE_N) / 2u) \
                / ((uint32_t)BAUD * OVERSAMPLE_N)))

static int g_fails = 0;
#define CHECK(c, m) do { if (!(c)) { epic_harness_log("FAIL: %s\n", m); g_fails++; } } while (0)

static void run_ticks(uint32_t software_ticks)
{
    for (uint32_t i = 0; i < software_ticks * CYCLES_PER_TICK; i++) epic_harness_tick();
}

int main(void)
{
    epic_harness_init(2000000UL);

    EPIC_SWUART_HandleTypeDef chan_a, chan_b;
    /* Channel A: TX on RB0, RX on RB2 (unused here). */
    EPIC_SWUART_Init(&chan_a, GPIOB, GPIO_PIN_0, GPIOB, GPIO_PIN_2, FOSC_HZ, 9600u);
    /* Channel B: TX on RB1 (unused here), RX on RB3. */
    EPIC_SWUART_Init(&chan_b, GPIOB, GPIO_PIN_1, GPIOB, GPIO_PIN_3, FOSC_HZ, 9600u);

    /* Drive unused RX pin (channel A's) high to avoid spurious start-bit detection. */
    SIM_DRIVE('B', 2, 1);  /* channel A's unused RX: set to idle (mark) */

    EPIC_SWUART_Write(&chan_a, (const uint8_t *)"Z", 1); /* 0x5A */

    /* 'A' = 0x41, same bit pattern test_swuart_rx.c uses, driven onto
     * channel B's RX pin (RB3) while channel A transmits. */
    static const uint8_t bits[] = {0, 1, 0, 0, 0, 0, 0, 1, 0, 1};
    for (size_t i = 0; i < 10; i++) {
        SIM_DRIVE('B', 3, bits[i]);
        run_ticks(OVERSAMPLE_N);
    }
    run_ticks(OVERSAMPLE_N * 3u); /* let A's TX and B's stop-bit finish */

    uint8_t rx_buf[4] = {0};
    int n = EPIC_SWUART_Read(&chan_b, rx_buf, sizeof(rx_buf));
    CHECK(n == 1, "channel B received one byte");
    CHECK(rx_buf[0] == 0x41u, "channel B byte == 'A'");
    CHECK(chan_a.tx_count == 0u, "channel A finished transmitting");
    CHECK(EPIC_SWUART_GetErrorCount(&chan_a) == 0u, "channel A no errors");
    CHECK(EPIC_SWUART_GetErrorCount(&chan_b) == 0u, "channel B no errors");

    epic_harness_log("swuart_dual: fails=%d\n", g_fails);
    return epic_harness_report(g_fails == 0);
}
