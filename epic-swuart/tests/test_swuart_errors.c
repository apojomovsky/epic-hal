/**
 * @file    test_swuart_errors.c
 * @brief   Two error paths: a bad stop bit (framing error) and an RX
 *          ring flooded past EPIC_SWUART_RING_SZ without being drained.
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

static void send_bits(char port, uint8_t pin, const uint8_t *bits, size_t n)
{
    for (size_t i = 0; i < n; i++) {
        SIM_DRIVE(port, pin, bits[i]);
        run_ticks(OVERSAMPLE_N);
    }
    run_ticks(OVERSAMPLE_N * 2u);
}

int main(void)
{
    epic_harness_init(4000000UL);

    /* ---- Bad stop bit: hold the line low instead of returning to mark. ---- */
    EPIC_SWUART_HandleTypeDef bad_stop;
    EPIC_SWUART_Init(&bad_stop, GPIOB, GPIO_PIN_0, GPIOB, GPIO_PIN_2, FOSC_HZ, 9600u);
    static const uint8_t framing_err_bits[] = {0, 1, 0, 0, 0, 0, 0, 1, 0, 0}; /* stop=0 */
    send_bits('B', 2, framing_err_bits, 10);

    uint8_t buf[4];
    CHECK(EPIC_SWUART_Read(&bad_stop, buf, sizeof(buf)) == 0, "bad-stop byte dropped");
    CHECK(EPIC_SWUART_GetErrorCount(&bad_stop) == 1u, "one framing error counted");

    /* ---- RX ring overflow: send more bytes than EPIC_SWUART_RING_SZ
     * without draining. ---- */
    EPIC_SWUART_HandleTypeDef flood;
    EPIC_SWUART_Init(&flood, GPIOB, GPIO_PIN_1, GPIOB, GPIO_PIN_3, FOSC_HZ, 9600u);
    static const uint8_t ok_bits[] = {0, 1, 0, 0, 0, 0, 0, 1, 0, 1}; /* 'A', valid framing */
    for (unsigned i = 0; i < EPIC_SWUART_RING_SZ + 2u; i++) {
        send_bits('B', 3, ok_bits, 10);
    }
    CHECK(EPIC_SWUART_GetErrorCount(&flood) == 2u, "two overflow drops counted");

    int total = 0, n;
    while ((n = EPIC_SWUART_Read(&flood, buf, sizeof(buf))) > 0) total += n;
    CHECK(total == (int)EPIC_SWUART_RING_SZ, "ring held exactly RING_SZ bytes");

    epic_harness_log("swuart_errors: fails=%d\n", g_fails);
    return epic_harness_report(g_fails == 0);
}
