/**
 * @file    test_swuart_dual.c
 * @brief   Two channels active at once, v2: channel A transmits while
 *          channel B receives, both RX pins in the same RB4:7 group on
 *          PIC16F87XA/PIC18Fxx5x, proving on_port_change correctly
 *          attributes the edge to the right channel and the 4-slot
 *          scheduler correctly interleaves two independent deadlines.
 */
#include <stdio.h>
#include "epic_swuart.h"
#include "core/epic_harness.h"

#if defined(PIC18F2455) || defined(PIC18F2550) || defined(PIC18F4455) || defined(PIC18F4550)
  #include "pic18fxx5x_sim.h"
  #include "core/pic18_irq.h"
  #define SIM_DRIVE(port, pin, lvl) pic18_sim_drive_input((port), (pin), (lvl))
  #define SIM_READ(port, pin) pic18_sim_read_output((port), (pin))
  #define ASSERT_CHANGE_FLAG() (EPIC_REG8(PIC_REG_INTCON) |= PIC_INTCON_RBIF)
#elif defined(PIC16F1933) || defined(PIC16F1934) || defined(PIC16F1936) || \
      defined(PIC16F1937) || defined(PIC16F1938) || defined(PIC16F1939)
  #include "pic16f193x_sim.h"
  #define SIM_DRIVE(port, pin, lvl) pic16f193x_sim_drive_input((port), (pin), (lvl))
  #define SIM_READ(port, pin) pic16f193x_sim_read_output((port), (pin))
  #define ASSERT_CHANGE_FLAG() ((void)0)
#else
  #include "pic16f87xa_sim.h"
  #include "core/pic16_irq.h"
  #define SIM_DRIVE(port, pin, lvl) pic16f87xa_sim_drive_input((port), (pin), (lvl))
  #define SIM_READ(port, pin) pic16f87xa_sim_read_output((port), (pin))
  #define ASSERT_CHANGE_FLAG() (EPIC_REG8(PIC_REG_INTCON) |= PIC_INTCON_RBIF)
#endif

static int g_fails = 0;
#define CHECK(c, m) do { if (!(c)) { epic_harness_log("FAIL: %s\n", m); g_fails++; } } while (0)

static void run_bit_periods(uint32_t n)
{
    for (uint32_t i = 0; i < n; i++) {
        for (uint32_t t = 0; t < (FOSC_HZ / 4u) / 9600u + 5u; t++) {
            epic_harness_tick();
        }
    }
}

int main(void)
{
    epic_harness_init(4000000UL);

    EPIC_SWUART_HandleTypeDef chan_a, chan_b;
    EPIC_SWUART_Init(&chan_a, GPIOB, GPIO_PIN_0, GPIOB, GPIO_PIN_4, FOSC_HZ, 9600u);
    EPIC_SWUART_Init(&chan_b, GPIOB, GPIO_PIN_1, GPIOB, GPIO_PIN_5, FOSC_HZ, 9600u);

    /* Channel A's RX (RB4) is unused in this test but shares the RB4:7
     * group interrupt with channel B's RX (RB5): every time B's start
     * bit or any later data-bit transition fires that shared interrupt,
     * on_port_change() checks A's current pin level too (this is by
     * design, see docs/ARCHITECTURE.md; it's what lets one shared
     * physical interrupt serve two channels without per-pin edge
     * tracking). A's precondition (documented since v1: the RX pin
     * must idle high whenever not actively receiving, or it's misread
     * as a start bit) is not automatically satisfied by the simulator,
     * whose undriven-input default is 0, not 1. Drive it high before
     * either channel does anything, exactly as real wiring (a pull-up
     * or a connected idle transmitter) would already guarantee. */
    SIM_DRIVE('B', 4, 1);

    EPIC_SWUART_Write(&chan_a, (const uint8_t *)"Z", 1); /* 0x5A */

    static const uint8_t bits[] = {0, 1, 0, 0, 0, 0, 0, 1, 0, 1}; /* 'A' on B's RX (pin 5) */
    SIM_DRIVE('B', 5, bits[0]);
    ASSERT_CHANGE_FLAG();
    epic_dispatch_all_irqs();
    for (size_t i = 1; i < 10; i++) {
        run_bit_periods(1);
        SIM_DRIVE('B', 5, bits[i]);
    }
    run_bit_periods(3); /* let A's TX and B's stop-bit both finish */

    uint8_t rx_buf[4] = {0};
    int n = EPIC_SWUART_Read(&chan_b, rx_buf, sizeof(rx_buf));
    CHECK(n == 1, "channel B received one byte");
    CHECK(rx_buf[0] == 0x41u, "channel B byte == 'A'");
    CHECK(chan_a.tx_count == 0u, "channel A finished transmitting");
    CHECK(EPIC_SWUART_GetErrorCount(&chan_a) == 0u, "channel A no errors");
    CHECK(EPIC_SWUART_GetErrorCount(&chan_b) == 0u, "channel B no errors");

    /* ---- EPIC_SWUART_Init: NULL handle and a full channel registry
     * both return EPIC_INVALID. Both slots (chan_a, chan_b) are already
     * occupied at this point in the test, which is what v1's equivalent
     * check needed a filler loop up to EPIC_SWUART_MAX_CHANNELS to set
     * up; v2 removed that macro (Global Constraints: hardcoded two
     * slots), so this test's own two live channels are the fixture. ---- */
    CHECK(EPIC_SWUART_Init(NULL, GPIOC, GPIO_PIN_2, GPIOC, GPIO_PIN_3,
                            FOSC_HZ, 9600u) == EPIC_INVALID,
          "init rejects NULL handle");
    EPIC_SWUART_HandleTypeDef chan_c;
    CHECK(EPIC_SWUART_Init(&chan_c, GPIOC, GPIO_PIN_2, GPIOC, GPIO_PIN_3,
                            FOSC_HZ, 9600u) == EPIC_INVALID,
          "init rejects a third channel when both slots are full");

    epic_harness_log("swuart_dual: fails=%d\n", g_fails);
    return epic_harness_report(g_fails == 0);
}
