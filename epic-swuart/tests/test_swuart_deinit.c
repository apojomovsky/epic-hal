/* EPIC_SWUART_DeInit host test: confirms DeInit really tears down both
 * CCP instances and Timer1 (checked by reading the SFRs directly, not
 * just that it returns EPIC_OK), and that a second Init after DeInit
 * works cleanly. TX/RX driven via the same test hooks
 * test_swuart_tx.c/test_swuart_rx.c use. */
#include <stdio.h>
#include "epic_swuart.h"

/* Family dispatch for the sim's drive_input, same set of families/macro
 * shape as test_swuart_rx.c/test_swuart_errors.c use. PIC16F193X's sim
 * only stages the driven level; unlike PIC16F87XA/PIC18Fxx5x's sim it
 * does not itself refresh the PORT register that EPIC_GPIO_ReadPin
 * reads (that refresh only happens inside pic16f193x_sim_step()), so
 * SIM_DRIVE for that family pumps one cycle of it immediately after. */
#if defined(PIC18F2455) || defined(PIC18F2550) || defined(PIC18F4455) || defined(PIC18F4550)
  #include "pic18fxx5x_sim.h"
  #define SIM_DRIVE(port, pin, lvl) pic18_sim_drive_input((port), (pin), (lvl))
  #define SIM_READ(port, pin) pic18_sim_read_output((port), (pin))
#elif defined(PIC16F1933) || defined(PIC16F1934) || defined(PIC16F1936) || \
      defined(PIC16F1937) || defined(PIC16F1938) || defined(PIC16F1939)
  #include "pic16f193x_sim.h"
  #define SIM_DRIVE(port, pin, lvl) \
      do { pic16f193x_sim_drive_input((port), (pin), (lvl)); pic16f193x_sim_step(1); } while (0)
  #define SIM_READ(port, pin) pic16f193x_sim_read_output((port), (pin))
#else
  #include "pic16f87xa_sim.h"
  #define SIM_DRIVE(port, pin, lvl) pic16f87xa_sim_drive_input((port), (pin), (lvl))
  #define SIM_READ(port, pin) pic16f87xa_sim_read_output((port), (pin))
#endif

static int g_fails = 0;
#define CHECK(c, m) do { if (!(c)) { printf("FAIL: %s\n", m); g_fails++; } } while (0)

/* Test-only hooks: see test_swuart_tx.c/test_swuart_rx.c. Defined in
 * epic_swuart.c behind EPIC_SWUART_TEST_HOOKS. swuart_test_last_tx_mode
 * always reads slot A's TX CCP (CCP2CON), which is exactly the slot
 * both channels in this test occupy in turn (only one channel is ever
 * live at a time here). */
extern uint8_t swuart_test_last_tx_mode(void);
extern void swuart_test_fire_tx_event(void);
extern void swuart_test_fire_rx_event(void);
#if EPIC_SWUART_HAS_RX_FAST_PATH
extern void swuart_test_set_capture_fast(uint16_t value);
#else
extern void swuart_test_set_capture(uint16_t value);
#endif

int main(void)
{
    /* Channel 1: register, start a TX frame, DeInit mid-frame. */
    EPIC_SWUART_HandleTypeDef chan1;
    EPIC_StatusTypeDef st = EPIC_SWUART_Init(&chan1, GPIOC, GPIO_PIN_1, GPIOC, GPIO_PIN_2,
                                              FOSC_HZ, 9600u);
    CHECK(st == EPIC_OK, "channel 1 init ok");

    /* Sanity: Init actually programmed both CCP instances and started
     * Timer1, so DeInit below has something real to tear down instead
     * of coincidentally finding zeroed registers already. RX (CCP1) is
     * armed for CAPTURE_FALLING at Init time, a non-zero encoding;
     * TX (CCP2) starts at CCP_MODE_OFF (0) until the first Write, so
     * check it only after queuing a byte below. */
    CHECK(EPIC_REG8(PIC_REG_CCP1CON) != 0x00u, "channel 1 RX CCP armed after init");
    CHECK((EPIC_REG8(PIC_REG_T1CON) & PIC_T1CON_TMR1ON) != 0u,
          "Timer1 running after channel 1 init");

    uint8_t zero_byte = 0x00u;
    size_t queued = EPIC_SWUART_Write(&chan1, &zero_byte, 1);
    CHECK(queued == 1u, "channel 1 queued one byte");
    CHECK(EPIC_REG8(PIC_REG_CCP2CON) != 0x00u, "channel 1 TX CCP armed after Write");

    /* Fire two of the nine events (start bit already armed by Write();
     * this lands mid-frame, not at a coincidentally-idle boundary).
     * tx_state's numeric encoding (0 = TX_IDLE) is epic_swuart.c-local,
     * not exposed via the header, but the handle struct itself is not
     * opaque; checking != 0 here is enough to prove "still mid-frame,
     * not idle" without needing the name. */
    swuart_test_fire_tx_event();
    swuart_test_fire_tx_event();
    CHECK(chan1.tx_state != 0u, "channel 1 genuinely mid-frame before DeInit");

    EPIC_StatusTypeDef deinit_st = EPIC_SWUART_DeInit(&chan1);
    CHECK(deinit_st == EPIC_OK, "DeInit returns EPIC_OK");

    /* The actual regression this test exists for: DeInit must really
     * call EPIC_CCP_DeInit on both of the channel's CCP instances
     * (both CON registers zeroed) and EPIC_TIMER1_DeInit (T1CON back
     * to its POR value), not merely return EPIC_OK while leaving
     * hardware armed. Only one channel is active here, so Timer1
     * teardown is unconditional; the conditional (survivor-preserving)
     * case is test_swuart_dual_deinit.c's job. */
    CHECK(EPIC_REG8(PIC_REG_CCP1CON) == 0x00u, "DeInit zeroed the RX CCP (CCP1)");
    CHECK(EPIC_REG8(PIC_REG_CCP2CON) == 0x00u, "DeInit zeroed the TX CCP (CCP2)");
    CHECK(EPIC_REG8(PIC_REG_T1CON) == PIC_T1CON_POR_VALUE,
          "DeInit reset Timer1 (T1CON) to its POR value");
    CHECK(SIM_READ('C', 1) == 1, "DeInit leaves TX at idle/mark, not stuck low");

    /* Channel 2: a *new* registration in the same slot, after the
     * registry emptied out. Only works if DeInit's Timer1 release
     * didn't leave the peripheral in a state that blocks a fresh Init,
     * and if Init's lazy restart re-arms Timer1 and the CCP instances
     * correctly. */
    EPIC_SWUART_HandleTypeDef chan2;
    st = EPIC_SWUART_Init(&chan2, GPIOC, GPIO_PIN_1, GPIOC, GPIO_PIN_2, FOSC_HZ, 9600u);
    CHECK(st == EPIC_OK, "channel 2 init ok after channel 1's DeInit");
    CHECK((EPIC_REG8(PIC_REG_T1CON) & PIC_T1CON_TMR1ON) != 0u,
          "Timer1 running again after channel 2 init");

    /* Register, write, read a byte again: TX 'A' (0x41), same mode
     * sequence test_swuart_tx.c checks. */
    uint8_t a_byte = 0x41u;
    queued = EPIC_SWUART_Write(&chan2, &a_byte, 1);
    CHECK(queued == 1u, "channel 2 queued one byte");

    static const uint8_t expected_modes[] = {8, 9, 9, 9, 9, 9, 8, 9, 8};
    int tx_ok = 1;
    for (size_t i = 0; i < 9; i++) {
        swuart_test_fire_tx_event();
        if (swuart_test_last_tx_mode() != expected_modes[i]) tx_ok = 0;
    }
    CHECK(tx_ok, "channel 2 transmits the correct mode sequence after re-init");
    CHECK(chan2.tx_count == 0u, "channel 2 finished transmitting");

    /* RX: an inbound 'A' (0x41), same technique test_swuart_rx.c uses. */
    static const uint8_t rx_bits[] = {0, 1, 0, 0, 0, 0, 0, 1, 0, 1};
    SIM_DRIVE('C', 2, rx_bits[0]);
#if EPIC_SWUART_HAS_RX_FAST_PATH
    swuart_test_set_capture_fast(1000u);
    swuart_test_fire_rx_event(); /* one fire: deglitch check + arm d0,
                                   * IDLE -> DATA0 (see test_swuart_rx.c
                                   * for why this collapsed from two). */
#else
    /* PIC18Fxx5x/PIC16F193X channel A keeps the generic two-fire
     * capture-then-confirm sequence; see rx_capture_event. */
    swuart_test_set_capture(1000u);
    swuart_test_fire_rx_event(); /* capture event: IDLE -> CONFIRM_START */
    swuart_test_fire_rx_event(); /* confirm event, half a bit later */
#endif
    for (size_t i = 1; i < 10; i++) {
        SIM_DRIVE('C', 2, rx_bits[i]);
        swuart_test_fire_rx_event(); /* compare event: sample + arm next */
    }

    uint8_t rx_buf[4] = {0};
    int n = EPIC_SWUART_Read(&chan2, rx_buf, sizeof(rx_buf));
    CHECK(n == 1, "channel 2 received one byte");
    CHECK(rx_buf[0] == 0x41u, "channel 2 byte == 'A'");
    CHECK(EPIC_SWUART_GetErrorCount(&chan2) == 0u, "channel 2 no errors");

    printf("swuart_deinit: fails=%d\n", g_fails);
    return g_fails == 0 ? 0 : 1;
}
