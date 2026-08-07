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
  #define SIM_DRIVE(port, pin, lvl) pic18_sim_drive_input((port), (pin), (lvl))
#elif defined(PIC16F1933) || defined(PIC16F1934) || defined(PIC16F1936) || \
      defined(PIC16F1937) || defined(PIC16F1938) || defined(PIC16F1939)
  #include "pic16f193x_sim.h"
  #define SIM_READ(port, pin) pic16f193x_sim_read_output((port), (pin))
  #define SIM_DRIVE(port, pin, lvl) pic16f193x_sim_drive_input((port), (pin), (lvl))
#else
  #include "pic16f87xa_sim.h"
  #define SIM_READ(port, pin) pic16f87xa_sim_read_output((port), (pin))
  #define SIM_DRIVE(port, pin, lvl) pic16f87xa_sim_drive_input((port), (pin), (lvl))
#endif

/* N=3, per docs/superpowers/plans/probe-swuart-isr-budget.md: the
 * straight-line probe measured N=4 as technically reachable (122/130
 * cycles, 6.2% margin) but that margin was measured on a minimal
 * snapshot without ring-buffer or parameter-passing overhead. N=3 has
 * a comfortable 29.9% margin (122/174) and is the production default. */
#define OVERSAMPLE_N 3u
#define BAUD 9600u

#define CYCLES_PER_TICK \
    ((uint32_t)((FOSC_HZ / 4u + ((uint32_t)BAUD * OVERSAMPLE_N) / 2u) \
                / ((uint32_t)BAUD * OVERSAMPLE_N)))

static void run_ticks(uint32_t software_ticks)
{
    for (uint32_t i = 0; i < software_ticks * CYCLES_PER_TICK; i++) epic_harness_tick();
}

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
        run_ticks(OVERSAMPLE_N / 2u);
        observed[bit] = SIM_READ('B', 0);
        run_ticks(OVERSAMPLE_N - OVERSAMPLE_N / 2u);
    }

    for (size_t bit = 0; bit < 10; bit++) {
        char msg[32];
        snprintf(msg, sizeof(msg), "bit %u", (unsigned)bit);
        CHECK(observed[bit] == expected[bit], msg);
    }

    /* ---- Two bytes queued back-to-back: confirm the stop bit of the
     * first and the start bit of the second are both correctly timed,
     * no dropped or extra bits in between. 'A' (0x41) then 0x02
     * (0b00000010, LSB first: d0=0, d1=1, d2..d7=0). ---- */
    EPIC_SWUART_HandleTypeDef h2;
    st = EPIC_SWUART_Init(&h2, GPIOC, GPIO_PIN_0, GPIOC, GPIO_PIN_1, FOSC_HZ, 9600u);
    CHECK(st == EPIC_OK, "second-channel init ok");
    SIM_DRIVE('C', 1, 1); /* h2's unused RX: idle (mark), avoid spurious start-bit detection */

    static const uint8_t two_bytes[] = {0x41u, 0x02u};
    size_t queued2 = EPIC_SWUART_Write(&h2, two_bytes, 2);
    CHECK(queued2 == 2u, "queued two bytes");

    static const uint8_t expected2[20] = {
        0, 1, 0, 0, 0, 0, 0, 1, 0, 1, /* 'A': start, d0..d7, stop */
        0, 0, 1, 0, 0, 0, 0, 0, 0, 1, /* 0x02: start, d0..d7, stop */
    };
    uint8_t observed2[20];

    for (size_t bit = 0; bit < 20; bit++) {
        run_ticks(OVERSAMPLE_N / 2u);
        observed2[bit] = SIM_READ('C', 0);
        run_ticks(OVERSAMPLE_N - OVERSAMPLE_N / 2u);
    }

    for (size_t bit = 0; bit < 20; bit++) {
        char msg[32];
        snprintf(msg, sizeof(msg), "two-byte bit %u", (unsigned)bit);
        CHECK(observed2[bit] == expected2[bit], msg);
    }
    CHECK(h2.tx_count == 0u, "both queued bytes drained");

    /* ---- EPIC_SWUART_Write returns a short count when the TX ring is
     * nearly full: h2's ring is confirmed empty above, so queueing
     * more than EPIC_SWUART_RING_SZ bytes in one call must return
     * exactly EPIC_SWUART_RING_SZ, not the full request. ---- */
    static const uint8_t overflow_bytes[EPIC_SWUART_RING_SZ + 3u] = {0};
    size_t short_write = EPIC_SWUART_Write(&h2, overflow_bytes, EPIC_SWUART_RING_SZ + 3u);
    CHECK(short_write == EPIC_SWUART_RING_SZ, "write returns a short count near-full");
    CHECK(h2.tx_count == EPIC_SWUART_RING_SZ, "ring holds exactly RING_SZ queued bytes");

    /* Drain h2's ring back to empty before reusing it below, so the
     * pending overflow bytes don't leak into a later assertion. */
    run_ticks((uint32_t)EPIC_SWUART_RING_SZ * 10u * OVERSAMPLE_N);

    /* ---- EPIC_SWUART_Init: NULL handle and a full channel registry
     * both return EPIC_INVALID. Two channels are already registered
     * above (h, h2); fill the registry up to EPIC_SWUART_MAX_CHANNELS,
     * then confirm the next Init is rejected. ---- */
    CHECK(EPIC_SWUART_Init(NULL, GPIOC, GPIO_PIN_2, GPIOC, GPIO_PIN_3,
                            FOSC_HZ, 9600u) == EPIC_INVALID,
          "init rejects NULL handle");

    EPIC_SWUART_HandleTypeDef fillers[EPIC_SWUART_MAX_CHANNELS];
    EPIC_StatusTypeDef fill_st = EPIC_OK;
    unsigned filled = 0;
    for (unsigned i = 0; i < EPIC_SWUART_MAX_CHANNELS; i++) {
        /* GPIO_PIN_n is a bitmask (EPIC_BIT(n)), not a pin index, so
         * each channel's pair is shifted by 2 bit positions, not
         * added arithmetically to the GPIO_PIN_2 macro's value. */
        uint16_t tx_pin = (uint16_t)(1u << (2u + i * 2u));
        uint16_t rx_pin = (uint16_t)(1u << (3u + i * 2u));
        fill_st = EPIC_SWUART_Init(&fillers[i], GPIOC, tx_pin, GPIOC, rx_pin, FOSC_HZ, 9600u);
        if (fill_st != EPIC_OK) break;
        filled++;
    }
    /* Two channels (h, h2) were already registered above, so the
     * registry is already full before this loop starts for the
     * default EPIC_SWUART_MAX_CHANNELS == 2; the loop's first Init is
     * expected to be the one that finds the registry full. */
    CHECK(fill_st == EPIC_INVALID, "init rejects a full channel registry");
    (void)filled;

    epic_harness_log("swuart_tx: fails=%d\n", g_fails);
    return epic_harness_report(g_fails == 0);
}
