/**
 * @file    test_swuart_deinit.c
 * @brief   EPIC_SWUART_DeInit host test: never exercised before this
 *          test existed (XC8 warned "_EPIC_SWUART_DeInit is never
 *          called" on every real-target build). Covers the scenario
 *          the module's design always claimed to support but nothing
 *          verified: init one channel, use it briefly, DeInit it
 *          mid-frame, then init a *new* channel and confirm it works
 *          correctly, proving the lazy Timer1 restart when the
 *          registry goes from 0 back to active.
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

    /* ---- Channel 1: use it briefly, then DeInit it mid-frame. ---- */
    EPIC_SWUART_HandleTypeDef chan1;
    EPIC_StatusTypeDef st = EPIC_SWUART_Init(&chan1, GPIOB, GPIO_PIN_0,
                                              GPIOB, GPIO_PIN_2,
                                              FOSC_HZ, 9600u);
    CHECK(st == EPIC_OK, "channel 1 init ok");
    SIM_DRIVE('B', 2, 1); /* channel 1's unused RX: idle (mark) */

    /* 0x00: every data bit is 0, so the line stays low through the
     * whole data field, guaranteeing DeInit below catches it mid-low
     * rather than by coincidence already back at mark. */
    uint8_t zero_byte = 0x00u;
    size_t queued = EPIC_SWUART_Write(&chan1, &zero_byte, 1);
    CHECK(queued == 1u, "channel 1 queued one byte");

    /* Run into the middle of the start bit: the line should be low
     * here, confirming the DeInit below really does interrupt a
     * frame in progress, not one that already finished. */
    run_ticks(OVERSAMPLE_N / 2u);
    CHECK(SIM_READ('B', 0) == 0, "channel 1 mid-frame: TX genuinely low before DeInit");

    EPIC_StatusTypeDef deinit_st = EPIC_SWUART_DeInit(&chan1);
    CHECK(deinit_st == EPIC_OK, "DeInit returns EPIC_OK");
    CHECK(SIM_READ('B', 0) == 1, "DeInit leaves TX at idle/mark, not stuck low");

    /* ---- Channel 2: a *new* channel, registered after the registry
     * emptied out. This only works if DeInit's Timer1 release didn't
     * leave the peripheral in a state that blocks a fresh Init, and
     * if Init's lazy restart (g_channel_count 0 -> 1) actually re-arms
     * Timer1 and its interrupt correctly. ---- */
    EPIC_SWUART_HandleTypeDef chan2;
    st = EPIC_SWUART_Init(&chan2, GPIOC, GPIO_PIN_0, GPIOC, GPIO_PIN_1,
                           FOSC_HZ, 9600u);
    CHECK(st == EPIC_OK, "channel 2 init ok after channel 1's DeInit");
    SIM_DRIVE('C', 1, 1); /* channel 2's unused RX: idle (mark) */

    /* TX: 'A' (0x41), same framing check as test_swuart_tx.c. */
    uint8_t a_byte = 0x41u;
    queued = EPIC_SWUART_Write(&chan2, &a_byte, 1);
    CHECK(queued == 1u, "channel 2 queued one byte");

    static const uint8_t expected_tx[] = {0, 1, 0, 0, 0, 0, 0, 1, 0, 1};
    uint8_t observed_tx[10];
    for (size_t bit = 0; bit < 10; bit++) {
        run_ticks(OVERSAMPLE_N / 2u);
        observed_tx[bit] = SIM_READ('C', 0);
        run_ticks(OVERSAMPLE_N - OVERSAMPLE_N / 2u);
    }
    int tx_ok = 1;
    for (size_t bit = 0; bit < 10; bit++) {
        if (observed_tx[bit] != expected_tx[bit]) tx_ok = 0;
    }
    CHECK(tx_ok, "channel 2 transmits correctly after the lazy Timer1 restart");
    CHECK(chan2.tx_count == 0u, "channel 2 finished transmitting");

    /* RX: drive an inbound 'A' onto channel 2's RX pin. */
    static const uint8_t rx_bits[] = {0, 1, 0, 0, 0, 0, 0, 1, 0, 1};
    for (size_t i = 0; i < 10; i++) {
        SIM_DRIVE('C', 1, rx_bits[i]);
        run_ticks(OVERSAMPLE_N);
    }
    run_ticks(OVERSAMPLE_N * 2u); /* let the stop-bit sample land */

    uint8_t rx_buf[4] = {0};
    int n = EPIC_SWUART_Read(&chan2, rx_buf, sizeof(rx_buf));
    CHECK(n == 1, "channel 2 received one byte");
    CHECK(rx_buf[0] == 0x41u, "channel 2 byte == 'A'");
    CHECK(EPIC_SWUART_GetErrorCount(&chan2) == 0u, "channel 2 no errors");

    epic_harness_log("swuart_deinit: fails=%d\n", g_fails);
    return epic_harness_report(g_fails == 0);
}
