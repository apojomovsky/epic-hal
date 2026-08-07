/**
 * @file    test_swuart_dual_deinit.c
 * @brief   Regression for the registry-compaction double-service bug:
 *          init two channels, confirm both work, DeInit one, then
 *          confirm the survivor still transmits at the correct bit
 *          rate (not corrupted, not double-serviced).
 *
 *          A prior straight-line rewrite of shared_tick() accessed
 *          g_channels[0]/g_channels[1] unconditionally instead of
 *          bounding the loop by g_channel_count. DeInit's registry
 *          compaction shifts the survivor down but leaves the
 *          vacated top slot holding a duplicate pointer to the same
 *          survivor, so the straight-line code served it twice per
 *          tick, doubling its effective bit rate and corrupting its
 *          frame. This test would have failed against that version;
 *          it passes against the g_channel_count-bounded loop.
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
    EPIC_StatusTypeDef st = EPIC_SWUART_Init(&chan_a, GPIOB, GPIO_PIN_0,
                                              GPIOB, GPIO_PIN_2, FOSC_HZ, BAUD);
    CHECK(st == EPIC_OK, "channel A init ok");
    /* Channel B: TX on RB1, RX on RB3 (unused here). */
    st = EPIC_SWUART_Init(&chan_b, GPIOB, GPIO_PIN_1, GPIOB, GPIO_PIN_3, FOSC_HZ, BAUD);
    CHECK(st == EPIC_OK, "channel B init ok");

    SIM_DRIVE('B', 2, 1); /* channel A's unused RX: idle (mark) */
    SIM_DRIVE('B', 3, 1); /* channel B's unused RX: idle (mark) */

    /* ---- Establish both channels work before touching DeInit. ---- */
    uint8_t byte_a = 0x41u; /* 'A' */
    uint8_t byte_b = 0x55u;
    size_t qa = EPIC_SWUART_Write(&chan_a, &byte_a, 1);
    size_t qb = EPIC_SWUART_Write(&chan_b, &byte_b, 1);
    CHECK(qa == 1u, "channel A queued one byte");
    CHECK(qb == 1u, "channel B queued one byte");

    /* 10 bit periods (start + 8 data + stop) plus a one-period margin. */
    run_ticks(11u * OVERSAMPLE_N);
    CHECK(chan_a.tx_count == 0u, "channel A finished transmitting before DeInit");
    CHECK(chan_b.tx_count == 0u, "channel B finished transmitting before DeInit");
    CHECK(EPIC_SWUART_GetErrorCount(&chan_a) == 0u, "channel A no errors before DeInit");
    CHECK(EPIC_SWUART_GetErrorCount(&chan_b) == 0u, "channel B no errors before DeInit");

    /* ---- DeInit channel A. The registry is [A, B]; compaction shifts
     * B down to index 0 and leaves index 1 holding a duplicate B
     * pointer (not garbage: the buggy straight-line shared_tick()
     * would still dereference it and call tx_step/rx_step on B a
     * second time every tick). ---- */
    EPIC_StatusTypeDef deinit_st = EPIC_SWUART_DeInit(&chan_a);
    CHECK(deinit_st == EPIC_OK, "DeInit(A) returns EPIC_OK");

    /* ---- Channel B alone: transmit a known byte and verify the exact
     * bit sequence, the same technique test_swuart_tx.c uses. If B
     * were serviced twice per tick, its effective bit rate would
     * double and every sample below would land off the intended grid,
     * failing this framing check. */
    uint8_t byte_c = 0x96u; /* 1001 0110, mixed 0s and 1s in every position */
    size_t qc = EPIC_SWUART_Write(&chan_b, &byte_c, 1);
    CHECK(qc == 1u, "channel B queued a byte after DeInit(A)");

    static const uint8_t expected_tx[] = {0, 0, 1, 1, 0, 1, 0, 0, 1, 1};
    uint8_t observed_tx[10];
    for (size_t bit = 0; bit < 10; bit++) {
        run_ticks(OVERSAMPLE_N / 2u);
        observed_tx[bit] = SIM_READ('B', 1);
        run_ticks(OVERSAMPLE_N - OVERSAMPLE_N / 2u);
    }
    int tx_ok = 1;
    for (size_t bit = 0; bit < 10; bit++) {
        if (observed_tx[bit] != expected_tx[bit]) tx_ok = 0;
    }
    CHECK(tx_ok, "channel B transmits correctly (single-serviced) after DeInit(A)");
    CHECK(chan_b.tx_count == 0u, "channel B finished transmitting after DeInit(A)");
    CHECK(EPIC_SWUART_GetErrorCount(&chan_b) == 0u, "channel B no errors after DeInit(A)");

    epic_harness_log("swuart_dual_deinit: fails=%d\n", g_fails);
    return epic_harness_report(g_fails == 0);
}
