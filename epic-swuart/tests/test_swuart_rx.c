/**
 * @file    test_swuart_rx.c
 * @brief   RX-only host test: synthesise an inbound 'A' (0x41) on the RX
 *          pin via the family sim's `*_sim_drive_input`, at the correct
 *          tick offsets, and check it decodes through EPIC_SWUART_Read.
 */
#include <stdio.h>
#include "epic_swuart.h"
#include "core/epic_harness.h"

#if defined(PIC18F2455) || defined(PIC18F2550) || defined(PIC18F4455) || defined(PIC18F4550)
  #include "pic18fxx5x_sim.h"
  #define SIM_DRIVE(port, pin, lvl) pic18_sim_drive_input((port), (pin), (lvl))
#elif defined(PIC16F1933) || defined(PIC16F1934) || defined(PIC16F1936) || \
      defined(PIC16F1937) || defined(PIC16F1938) || defined(PIC16F1939)
  #include "pic16f193x_sim.h"
  #define SIM_DRIVE(port, pin, lvl) pic16f193x_sim_drive_input((port), (pin), (lvl))
#else
  #include "pic16f87xa_sim.h"
  #define SIM_DRIVE(port, pin, lvl) pic16f87xa_sim_drive_input((port), (pin), (lvl))
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

    EPIC_SWUART_HandleTypeDef h;
    EPIC_SWUART_Init(&h, GPIOB, GPIO_PIN_0, GPIOB, GPIO_PIN_2, FOSC_HZ, 9600u);

    /* 'A' = 0x41 = 0b01000001, LSB first: start=0,1,0,0,0,0,0,1,0,stop=1. */
    static const uint8_t bits[] = {0, 1, 0, 0, 0, 0, 0, 1, 0, 1};

    for (size_t i = 0; i < 10; i++) {
        SIM_DRIVE('B', 2, bits[i]);
        run_ticks(OVERSAMPLE_N);
    }
    run_ticks(OVERSAMPLE_N * 2u); /* let the stop-bit sample land */

    uint8_t buf[4] = {0};
    int n = EPIC_SWUART_Read(&h, buf, sizeof(buf));
    CHECK(n == 1, "one byte read");
    CHECK(buf[0] == 0x41u, "byte == 'A'");
    CHECK(EPIC_SWUART_GetErrorCount(&h) == 0u, "no framing errors");

    epic_harness_log("swuart_rx: fails=%d\n", g_fails);
    return epic_harness_report(g_fails == 0);
}
