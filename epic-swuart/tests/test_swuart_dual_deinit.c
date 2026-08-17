/* Regression test for the Timer1-teardown rule: EPIC_SWUART_DeInit
 * must only tear down the shared Timer1 once BOTH channel slots are
 * empty (g_chan_a == NULL && g_chan_b == NULL); the old unconditional
 * teardown silently broke a still-active sibling on PIC16F193X. Inits
 * two channels, DeInits one, then exercises the survivor (TX+RX and
 * T1CON/CCP CON SFR readback). PIC16F193X only; empty TU elsewhere. */
#include "epic_swuart.h"

#if EPIC_SWUART_MAX_CHANNELS >= 2

#include <stdio.h>
#if defined(PIC16F1933) || defined(PIC16F1934) || defined(PIC16F1936) || \
      defined(PIC16F1937) || defined(PIC16F1938) || defined(PIC16F1939)
  #include "pic16f193x_sim.h"
#else
  #include "pic16f87xa_sim.h"
#endif

static int g_fails = 0;
#define CHECK(c, m) do { if (!(c)) { printf("FAIL: %s\n", m); g_fails++; } } while (0)

/* Test-only hooks: see test_swuart_tx.c/test_swuart_rx.c/test_swuart_dual.c
 * for the originals. There is no TX hook for channel B (nothing in the
 * existing suite transmits on B), so this test transmits on A and
 * receives on both A and B, exactly like test_swuart_dual.c's split.
 * Defined in epic_swuart.c behind EPIC_SWUART_TEST_HOOKS. */
#ifndef EPIC_SWUART_TEST_HOOKS
#define EPIC_SWUART_TEST_HOOKS 1
#endif
/** @brief Test hook: fire one channel A TX compare event. */
extern void epic_swuart_test_fire_tx_event(void);
/** @brief Test hook: fire one channel A RX capture/compare event. */
extern void epic_swuart_test_fire_rx_event(void);
/** @brief Test hook: fire one channel B RX capture/compare event. */
extern void epic_swuart_test_fire_rx_event_b(void);
/** @brief Test hook: inject the generic RX capture value. */
extern void epic_swuart_test_set_capture(uint16_t value);

/**
 * @brief  Drives one full byte (start + 8 data + stop, LSB first) onto
 *         channel A's RX pin (RC2) and fires the matching
 *         capture-then-compare event sequence, same technique
 *         test_swuart_rx.c/test_swuart_errors.c use. PIC16F193X channel
 *         A uses the generic two-fire path (the RX hot-path fix is
 *         PIC16F87XA-only; see EPIC_SWUART_HAS_RX_FAST_PATH), so this
 *         is the capture-then-confirm sequence rx_capture_event expects.
 * @param bits the 10 bit levels (start, d0..d7, stop), LSB first.
 */
static void receive_byte_a(const uint8_t *bits)
{
    pic16f193x_sim_drive_input('C', 2, bits[0]);
    pic16f193x_sim_step(1);
    epic_swuart_test_set_capture(2000u);
    epic_swuart_test_fire_rx_event(); /* capture event: IDLE -> CONFIRM_START */
    epic_swuart_test_fire_rx_event(); /* confirm event, half a bit later */
    for (size_t i = 1; i < 10; i++) {
        pic16f193x_sim_drive_input('C', 2, bits[i]);
        pic16f193x_sim_step(1);
        epic_swuart_test_fire_rx_event(); /* compare event: sample + arm next */
    }
}

/** @brief Dual-channel DeInit host test main: survivor keeps Timer1. */
int main(void)
{
    pic16f193x_sim_reset();

    EPIC_SWUART_HandleTypeDef chan_a, chan_b;
    /* Channel A: CCP1 = RX (RC2), CCP2 = TX (RC1). */
    EPIC_StatusTypeDef st_a = EPIC_SWUART_Init(&chan_a, GPIOC, GPIO_PIN_1, GPIOC, GPIO_PIN_2,
                                                FOSC_HZ, 9600u);
    CHECK(st_a == EPIC_OK, "channel A init ok");
    /* Channel B: CCP3 = RX (RB5), CCP4 = TX (RD1). */
    EPIC_StatusTypeDef st_b = EPIC_SWUART_Init(&chan_b, GPIOD, GPIO_PIN_1, GPIOB, GPIO_PIN_5,
                                                FOSC_HZ, 9600u);
    CHECK(st_b == EPIC_OK, "channel B init ok");

    CHECK((EPIC_REG8(PIC_REG_T1CON) & PIC_T1CON_TMR1ON) != 0u,
          "Timer1 running with both channels active");

    /* Establish both channels actually work before touching DeInit:
     * channel A transmits 'Z', channel B receives 'A', same technique
     * and bytes test_swuart_dual.c uses. */
    size_t queued = EPIC_SWUART_Write(&chan_a, (const uint8_t *)"Z", 1);
    CHECK(queued == 1u, "channel A queued one byte");
    for (size_t i = 0; i < 9; i++) epic_swuart_test_fire_tx_event();
    CHECK(chan_a.tx_count == 0u, "channel A finished transmitting before DeInit");
    CHECK(EPIC_SWUART_GetErrorCount(&chan_a) == 0u, "channel A no errors before DeInit");

    static const uint8_t bits[] = {0, 1, 0, 0, 0, 0, 0, 1, 0, 1}; /* 'A', LSB first */
    pic16f193x_sim_drive_input('B', 5, bits[0]);
    pic16f193x_sim_step(1);
    epic_swuart_test_set_capture(1000u);
    epic_swuart_test_fire_rx_event_b(); /* capture event: IDLE -> CONFIRM_START */
    epic_swuart_test_fire_rx_event_b(); /* confirm event, half a bit later */
    for (size_t i = 1; i < 10; i++) {
        pic16f193x_sim_drive_input('B', 5, bits[i]);
        pic16f193x_sim_step(1);
        epic_swuart_test_fire_rx_event_b();
    }
    uint8_t rx_buf_b[4] = {0};
    int n_b = EPIC_SWUART_Read(&chan_b, rx_buf_b, sizeof(rx_buf_b));
    CHECK(n_b == 1, "channel B received one byte before DeInit");
    CHECK(rx_buf_b[0] == 0x41u, "channel B byte == 'A' before DeInit");
    CHECK(EPIC_SWUART_GetErrorCount(&chan_b) == 0u, "channel B no errors before DeInit");

    /* Snapshot channel A's own CCP registers before touching B's
     * DeInit, so the "untouched by the sibling's DeInit" check below
     * compares against what they actually were, not an assumption. */
    uint8_t ccp1_before = EPIC_REG8(PIC_REG_CCP1CON);
    uint8_t ccp2_before = EPIC_REG8(PIC_REG_CCP2CON);

    /* DeInit channel B. Channel A is the survivor. */
    EPIC_StatusTypeDef deinit_b = EPIC_SWUART_DeInit(&chan_b);
    CHECK(deinit_b == EPIC_OK, "DeInit(B) returns EPIC_OK");

    /* Channel B's own CCP instances really were torn down. */
    CHECK(EPIC_REG8(PIC_REG_CCP3CON) == 0x00u, "channel B RX CCP (CCP3) zeroed by DeInit");
    CHECK(EPIC_REG8(PIC_REG_CCP4CON) == 0x00u, "channel B TX CCP (CCP4) zeroed by DeInit");

    /* The actual regression this test exists for: with channel A still
     * active, Timer1 must NOT have been torn down. The pre-fix
     * unconditional-teardown DeInit would clear TMR1ON and reset T1CON
     * to its POR value here even though channel A was never touched. */
    CHECK((EPIC_REG8(PIC_REG_T1CON) & PIC_T1CON_TMR1ON) != 0u,
          "Timer1 still running after DeInit(B): shared with surviving channel A");
    CHECK(EPIC_REG8(PIC_REG_T1CON) != PIC_T1CON_POR_VALUE,
          "T1CON not reset to its POR value while channel A survives");

    /* Channel A's own CCP instances are bit-for-bit unchanged: DeInit(B)
     * only ever touches CCP3/CCP4 plus Timer1 (conditionally). */
    CHECK(EPIC_REG8(PIC_REG_CCP1CON) == ccp1_before,
          "channel A RX CCP (CCP1) untouched by DeInit(B)");
    CHECK(EPIC_REG8(PIC_REG_CCP2CON) == ccp2_before,
          "channel A TX CCP (CCP2) untouched by DeInit(B)");

    /* Channel A's own ring/deadline state is untouched too. */
    CHECK(chan_a.tx_count == 0u, "channel A tx ring state untouched by DeInit(B)");
    CHECK(EPIC_SWUART_GetErrorCount(&chan_a) == 0u, "channel A error count untouched by DeInit(B)");

    /* The real proof: the survivor still works, both directions. A
     * regression that yanked Timer1 out from under channel A would
     * show here first (a fresh Write() reads EPIC_TIMER1_ReadCounter()
     * to arm the next deadline). */
    uint8_t byte_x = 0x96u; /* 1001 0110: mixed bits in every position */
    size_t qx = EPIC_SWUART_Write(&chan_a, &byte_x, 1);
    CHECK(qx == 1u, "channel A queues a byte after sibling DeInit");
    for (size_t i = 0; i < 9; i++) epic_swuart_test_fire_tx_event();
    CHECK(chan_a.tx_count == 0u, "channel A TX completes after sibling DeInit");
    CHECK(EPIC_SWUART_GetErrorCount(&chan_a) == 0u, "channel A TX no errors after sibling DeInit");

    receive_byte_a(bits); /* channel A now receives 'A' too. */
    uint8_t rx_buf_a[4] = {0};
    int n_a = EPIC_SWUART_Read(&chan_a, rx_buf_a, sizeof(rx_buf_a));
    CHECK(n_a == 1, "channel A received one byte after sibling DeInit");
    CHECK(rx_buf_a[0] == 0x41u, "channel A byte == 'A' after sibling DeInit");
    CHECK(EPIC_SWUART_GetErrorCount(&chan_a) == 0u, "channel A RX no errors after sibling DeInit");

    /* Close the loop: DeInit the survivor too, and confirm Timer1 *is*
     * torn down once both slots are genuinely empty, proving the
     * conditional cuts both ways rather than just never firing. */
    EPIC_StatusTypeDef deinit_a = EPIC_SWUART_DeInit(&chan_a);
    CHECK(deinit_a == EPIC_OK, "DeInit(A) returns EPIC_OK");
    CHECK(EPIC_REG8(PIC_REG_T1CON) == PIC_T1CON_POR_VALUE,
          "Timer1 torn down once both channels are gone");
    CHECK(EPIC_REG8(PIC_REG_CCP1CON) == 0x00u, "channel A RX CCP (CCP1) zeroed once it too is DeInit'd");
    CHECK(EPIC_REG8(PIC_REG_CCP2CON) == 0x00u, "channel A TX CCP (CCP2) zeroed once it too is DeInit'd");

    printf("epic_swuart_dual_deinit: fails=%d\n", g_fails);
    return g_fails == 0 ? 0 : 1;
}

#else /* EPIC_SWUART_MAX_CHANNELS < 2: not a PIC16F193X build. */

/** @brief Empty TU main for single-channel families. */
int main(void) { return 0; }

#endif
