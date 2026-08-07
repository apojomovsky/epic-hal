/**
 * @file    test_swuart_rx.c
 * @brief   RX-only host test, v2: drives the RX pin's simulated input,
 *          triggers the change interrupt (real sim modeling on
 *          PIC16F193X; the documented test-only RBIF-assert fallback on
 *          PIC16F87XA/PIC18Fxx5x, see pic16f87xa-hal/tests/
 *          example_rb_change.c for the established workaround pattern),
 *          and lets the event-driven scheduler take it from there.
 */
#include <stdio.h>
#include "epic_swuart.h"
#include "core/epic_harness.h"

#if defined(PIC18F2455) || defined(PIC18F2550) || defined(PIC18F4455) || defined(PIC18F4550)
  #include "pic18fxx5x_sim.h"
  #include "core/pic18_irq.h"
  #define SIM_DRIVE(port, pin, lvl) pic18_sim_drive_input((port), (pin), (lvl))
  #define ASSERT_CHANGE_FLAG() (EPIC_REG8(PIC_REG_INTCON) |= PIC_INTCON_RBIF)
#elif defined(PIC16F1933) || defined(PIC16F1934) || defined(PIC16F1936) || \
      defined(PIC16F1937) || defined(PIC16F1938) || defined(PIC16F1939)
  #include "pic16f193x_sim.h"
  #define SIM_DRIVE(port, pin, lvl) pic16f193x_sim_drive_input((port), (pin), (lvl))
  #define ASSERT_CHANGE_FLAG() ((void)0) /* real IOC sim modeling fires it */
#else
  #include "pic16f87xa_sim.h"
  #include "core/pic16_irq.h"
  #define SIM_DRIVE(port, pin, lvl) pic16f87xa_sim_drive_input((port), (pin), (lvl))
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
    epic_harness_init(2000000UL);

    EPIC_SWUART_HandleTypeDef h;
    EPIC_SWUART_Init(&h, GPIOB, GPIO_PIN_0, GPIOB, GPIO_PIN_4, FOSC_HZ, 9600u);

    /* 'A' = 0x41 = 0b01000001, LSB first: start=0,1,0,0,0,0,0,1,0,stop=1. */
    static const uint8_t bits[] = {0, 1, 0, 0, 0, 0, 0, 1, 0, 1};

    SIM_DRIVE('B', 4, bits[0]); /* start bit: pin goes low */
    ASSERT_CHANGE_FLAG();
    epic_dispatch_all_irqs(); /* deliver the edge synchronously, matching
                                * example_rb_change.c's own established
                                * test pattern for this same limitation */

    for (size_t i = 1; i < 10; i++) {
        run_bit_periods(1);
        SIM_DRIVE('B', 4, bits[i]);
    }
    run_bit_periods(2); /* let the stop-bit sample land */

    uint8_t buf[4] = {0};
    int n = EPIC_SWUART_Read(&h, buf, sizeof(buf));
    CHECK(n == 1, "one byte read");
    CHECK(buf[0] == 0x41u, "byte == 'A'");
    CHECK(EPIC_SWUART_GetErrorCount(&h) == 0u, "no framing errors");

    epic_harness_log("swuart_rx: fails=%d\n", g_fails);
    return epic_harness_report(g_fails == 0);
}
