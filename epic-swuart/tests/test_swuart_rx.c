/* RX-only host test for the RX hot-path fix: the deglitch check and
 * the first-sample arm happen in one synchronous pass
 * (rx_capture_event_fast), so one fire does what used to take two.
 * Drives the state machine via test-only hooks and injects the
 * captured edge value into the same CCP1 registers the fast path
 * reads. */
#include <stdio.h>
#include "epic_swuart.h"

/* Family dispatch for the sim's drive_input, same set of families/macro
 * shape as test_swuart_errors.c/test_swuart_dual.c use. PIC16F193X's sim
 * only stages the driven level; unlike PIC16F87XA/PIC18Fxx5x's sim it
 * does not itself refresh the PORT register that EPIC_GPIO_ReadPin
 * reads (that refresh only happens inside pic16f193x_sim_step()), so
 * SIM_DRIVE for that family pumps one cycle of it immediately after. */
#if defined(PIC18F2455) || defined(PIC18F2550) || defined(PIC18F4455) || defined(PIC18F4550)
  #include "pic18fxx5x_sim.h"
  #define SIM_DRIVE(port, pin, lvl) pic18_sim_drive_input((port), (pin), (lvl))
#elif defined(PIC16F1933) || defined(PIC16F1934) || defined(PIC16F1936) || \
      defined(PIC16F1937) || defined(PIC16F1938) || defined(PIC16F1939)
  #include "pic16f193x_sim.h"
  #define SIM_DRIVE(port, pin, lvl) \
      do { pic16f193x_sim_drive_input((port), (pin), (lvl)); pic16f193x_sim_step(1); } while (0)
#else
  #include "pic16f87xa_sim.h"
  #define SIM_DRIVE(port, pin, lvl) pic16f87xa_sim_drive_input((port), (pin), (lvl))
#endif

static int g_fails = 0;
#define CHECK(c, m) do { if (!(c)) { printf("FAIL: %s\n", m); g_fails++; } } while (0)

/** @brief Test hook: fire one channel A RX capture/compare event. */
extern void epic_swuart_test_fire_rx_event(void);
#if EPIC_SWUART_HAS_RX_FAST_PATH
/** @brief Test hook: inject the fast-path RX capture value. */
extern void epic_swuart_test_set_capture_fast(uint16_t value);
#else
/** @brief Test hook: inject the generic RX capture value. */
extern void epic_swuart_test_set_capture(uint16_t value);
#endif

/** @brief RX-only host test main for the fast-path hot fix. */
int main(void)
{
    EPIC_SWUART_HandleTypeDef h;
    EPIC_SWUART_Init(&h, GPIOC, GPIO_PIN_1, GPIOC, GPIO_PIN_2, 20000000UL, 9600u);

    /* 'A' = 0x41 = 0b01000001, LSB first: start=0,1,0,0,0,0,0,1,0,stop=1. */
    static const uint8_t bits[] = {0, 1, 0, 0, 0, 0, 0, 1, 0, 1};

    SIM_DRIVE('C', 2, bits[0]);
#if EPIC_SWUART_HAS_RX_FAST_PATH
    epic_swuart_test_set_capture_fast(1000u);
    epic_swuart_test_fire_rx_event(); /* one fire: deglitch check (bits[0]=0,
                                   * still on the line, passes) AND arms
                                   * d0's deadline, IDLE -> DATA0. The
                                   * old two-fire sequence (capture,
                                   * then a separately-scheduled
                                   * confirm) collapses into this one
                                   * synchronous pass; see
                                   * rx_capture_event_fast. */
#else
    /* PIC18Fxx5x/PIC16F193X channel A keeps the generic two-fire
     * capture-then-confirm sequence; see rx_capture_event. */
    epic_swuart_test_set_capture(1000u);
    epic_swuart_test_fire_rx_event(); /* capture event: IDLE -> CONFIRM_START */
    epic_swuart_test_fire_rx_event(); /* confirm event, half a bit later */
#endif

    for (size_t i = 1; i < 10; i++) {
        SIM_DRIVE('C', 2, bits[i]);
        epic_swuart_test_fire_rx_event(); /* compare event: sample + arm next */
    }

    uint8_t buf[4] = {0};
    int n = EPIC_SWUART_Read(&h, buf, sizeof(buf));
    CHECK(n == 1, "one byte read");
    CHECK(buf[0] == 0x41u, "byte == 'A'");
    CHECK(EPIC_SWUART_GetErrorCount(&h) == 0u, "no framing errors");

    printf("epic_swuart_rx: fails=%d\n", g_fails);
    return g_fails == 0 ? 0 : 1;
}
