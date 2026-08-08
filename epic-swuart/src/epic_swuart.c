/**
 * @file    epic_swuart.c
 * @brief   Bit-banged full-duplex UART, CCP hardware capture/compare
 *          timing. See docs/ARCHITECTURE.md for the shared-tick design
 *          and docs/API.md for per-function semantics.
 */
#include "epic_swuart.h"

/* TX state machine states. TX_DATA covers all 8 data bits, distinguished
 * by tx_bit_index, not by a separate enum value per bit: a per-bit enum
 * (tried first, caught in review) needs one shared case label per bit
 * plus a final duplicate for bit 7, which drives one extra spurious
 * transition using stale shifted-out data. Counting with tx_bit_index
 * instead makes "8 bits done" an explicit condition, not an off-by-one
 * in the case list. */
enum {
    TX_IDLE = 0,
    TX_DATA,
    TX_STOP,
};

/* RX state machine states. */
enum {
    RX_IDLE = 0,
    RX_CONFIRM_START,
    RX_DATA0, RX_DATA1, RX_DATA2, RX_DATA3,
    RX_DATA4, RX_DATA5, RX_DATA6, RX_DATA7,
    RX_STOP,
};

/* Forward declaration: defined in Task 5 (RX state machine). Stubbed
 * empty for this task only so the TX-only rewrite compiles and links
 * standalone; Task 5 replaces the stub with the real definition. */
static void on_rx_event_a(void);

/* TODO(Task 5): replace this empty stub with the real RX compare/capture
 * event handler. Kept here only so this task's TX rewrite compiles and
 * links standalone without RX logic existing yet. */
static void on_rx_event_a(void) { }

/* Cycles of lead time between EPIC_SWUART_Write() arming the start bit's
 * compare deadline and that deadline actually landing: must be large
 * enough that EPIC_CCP_SetCompare/SetMode land before Timer1 reaches the
 * armed value, confirmed on real PIC16F877A hardware (see
 * docs/superpowers/plans/probe-swuart-v3-ccp-cost.md) to need 120
 * cycles, not the original 40-cycle guess. */
#define SWUART_LEAD_CYCLES 120u

/* One bit period in instruction cycles: round(FOSC_HZ / 4 / baud).
 * Timer1 prescaler stays 1:1 (unchanged from v1) so this is directly
 * the Timer1 counter delta for one bit. */
static uint16_t compute_cycles_per_bit(uint32_t fosc_hz, uint32_t baud)
{
    uint32_t cycles = (fosc_hz / 4u + baud / 2u) / baud;
    if (cycles > 65535u) cycles = 65535u;
    if (cycles < 1u) cycles = 1u;
    return (uint16_t)cycles;
}

static uint16_t g_cycles_per_bit = 0u;
static EPIC_SWUART_HandleTypeDef *g_chan_a = NULL;
static TIMER1_HandleTypeDef s_timer1 = TIMER1_HANDLE_DEFAULT;

#define SWUART_CCP_RX CCP_INSTANCE_1
#define SWUART_CCP_TX CCP_INSTANCE_2

/* Arms the mode for the *next* compare match, not the one that just
 * fired (hardware already toggled the pin per whatever was armed
 * ahead of time; this function's job is only to decide what happens
 * at the deadline it is about to write). Traced against 'A' = 0x41,
 * LSB first (start=0, d0=1, d1..d5=0, d6=1, d7=0, stop=1): Write()
 * arms CLEAR for the start bit; this handler then arms SET (d0=1),
 * CLEAR x5 (d1..d5=0), SET (d6=1), CLEAR (d7=0), SET (stop=1), in that
 * order, landing back in TX_IDLE exactly at the stop bit's deadline. */
static void tx_compare_event(EPIC_SWUART_HandleTypeDef *h, CCP_InstanceTypeDef tx_inst)
{
    CCP_ModeTypeDef next_mode;

    switch (h->tx_state) {
    case TX_IDLE:
        if (h->tx_count == 0u) {
            EPIC_CCP_SetMode(tx_inst, CCP_MODE_OFF);
            return;
        }
        h->tx_shift = h->tx_ring[h->tx_tail];
        h->tx_tail = (uint8_t)((h->tx_tail + 1u) & (EPIC_SWUART_RING_SZ - 1u));
        h->tx_count--;
        h->tx_bit_index = 0u;
        next_mode = CCP_MODE_COMPARE_CLEAR;
        h->tx_state = TX_DATA;
        break;
    case TX_DATA:
        next_mode = (h->tx_shift & 0x01u) ? CCP_MODE_COMPARE_SET : CCP_MODE_COMPARE_CLEAR;
        h->tx_shift >>= 1;
        h->tx_bit_index++;
        if (h->tx_bit_index >= 8u) h->tx_state = TX_STOP;
        break;
    case TX_STOP:
    default:
        next_mode = CCP_MODE_COMPARE_SET;
        h->tx_state = TX_IDLE;
        break;
    }

    h->tx_deadline = (uint16_t)(h->tx_deadline + g_cycles_per_bit);
    EPIC_CCP_SetCompare(tx_inst, h->tx_deadline);
    EPIC_CCP_SetMode(tx_inst, next_mode);
}

static void on_tx_event_a(void) { tx_compare_event(g_chan_a, SWUART_CCP_TX); }

/* Test-only hooks (see test_swuart_tx.c): default-enabled, same guard
 * pattern the test file uses, so a CMake host-sim build gets them
 * without a separate compile-definition wire-up; a real-target build
 * that wants them compiled out can predefine EPIC_SWUART_TEST_HOOKS=0. */
#ifndef EPIC_SWUART_TEST_HOOKS
#define EPIC_SWUART_TEST_HOOKS 1
#endif
#if EPIC_SWUART_TEST_HOOKS
uint8_t swuart_test_last_tx_mode(void) { return (uint8_t)EPIC_REG8(0x1DU); }
uint16_t swuart_test_last_tx_compare(void) { return g_chan_a->tx_deadline; }
void swuart_test_fire_tx_event(void) { on_tx_event_a(); }
#endif

static void rx_push(EPIC_SWUART_HandleTypeDef *h, uint8_t byte)
{
    if (h->rx_count >= EPIC_SWUART_RING_SZ) {
        h->error_count++;
        return;
    }
    h->rx_ring[h->rx_head] = byte;
    h->rx_head = (uint8_t)((h->rx_head + 1u) & (EPIC_SWUART_RING_SZ - 1u));
    h->rx_count++;
}

EPIC_StatusTypeDef EPIC_SWUART_Init(EPIC_SWUART_HandleTypeDef *h,
                                     GPIO_TypeDef tx_port, uint16_t tx_pin,
                                     GPIO_TypeDef rx_port, uint16_t rx_pin,
                                     uint32_t fosc_hz, uint32_t baud)
{
    if (!h || g_chan_a != NULL) return EPIC_INVALID;
    /* PIC16F87XA: CCP1 = RC2 (RX), CCP2 = RC1 (TX). */
    if (tx_port != GPIOC || tx_pin != GPIO_PIN_1) return EPIC_INVALID;
    if (rx_port != GPIOC || rx_pin != GPIO_PIN_2) return EPIC_INVALID;

    h->tx_port = tx_port; h->tx_pin = tx_pin;
    h->rx_port = rx_port; h->rx_pin = rx_pin;
    h->tx_state = TX_IDLE; h->tx_deadline = 0u;
    h->tx_head = h->tx_tail = h->tx_count = 0u;
    h->rx_state = RX_IDLE; h->rx_deadline = 0u;
    h->rx_head = h->rx_tail = h->rx_count = 0u;
    h->error_count = 0u;

    g_cycles_per_bit = compute_cycles_per_bit(fosc_hz, baud);

    EPIC_GPIO_Init(tx_port, tx_pin, GPIO_MODE_OUTPUT);
    EPIC_GPIO_Init(rx_port, rx_pin, GPIO_MODE_INPUT);

    s_timer1 = (TIMER1_HandleTypeDef)TIMER1_HANDLE_DEFAULT;
    EPIC_TIMER1_Init(&s_timer1);
    EPIC_TIMER1_Start(&s_timer1);

    CCP_HandleTypeDef ccp_rx = { .Instance = SWUART_CCP_RX, .Mode = CCP_MODE_CAPTURE_FALLING,
                                 .CompareValue = 0u, .EventCallback = on_rx_event_a };
    EPIC_CCP_Init(&ccp_rx);
    CCP_HandleTypeDef ccp_tx = { .Instance = SWUART_CCP_TX, .Mode = CCP_MODE_OFF,
                                 .CompareValue = 0u, .EventCallback = on_tx_event_a };
    EPIC_CCP_Init(&ccp_tx);

    EPIC_IRQ_Restore(1);
    g_chan_a = h;
    return EPIC_OK;
}

EPIC_StatusTypeDef EPIC_SWUART_DeInit(EPIC_SWUART_HandleTypeDef *h)
{
    if (!h || g_chan_a != h) return EPIC_INVALID;
    EPIC_CCP_DeInit(SWUART_CCP_RX);
    EPIC_CCP_DeInit(SWUART_CCP_TX);
    EPIC_TIMER1_DeInit();
    g_chan_a = NULL;
    return EPIC_OK;
}

size_t EPIC_SWUART_Write(EPIC_SWUART_HandleTypeDef *h, const uint8_t *data, size_t len)
{
    if (!h) return 0u;
    size_t written = 0u;
    while (written < len && h->tx_count < EPIC_SWUART_RING_SZ) {
        uint8_t prev = EPIC_IRQ_Disable();
        h->tx_ring[h->tx_head] = data[written];
        h->tx_head = (uint8_t)((h->tx_head + 1u) & (EPIC_SWUART_RING_SZ - 1u));
        h->tx_count++;
        EPIC_IRQ_Restore(prev);
        written++;
    }
    if (written > 0u && h->tx_state == TX_IDLE) {
        h->tx_shift = h->tx_ring[h->tx_tail];
        h->tx_tail = (uint8_t)((h->tx_tail + 1u) & (EPIC_SWUART_RING_SZ - 1u));
        h->tx_count--;
        h->tx_bit_index = 0u;
        h->tx_state = TX_DATA;
        h->tx_deadline = (uint16_t)(EPIC_TIMER1_ReadCounter() + SWUART_LEAD_CYCLES);
        EPIC_CCP_SetCompare(SWUART_CCP_TX, h->tx_deadline);
        EPIC_CCP_SetMode(SWUART_CCP_TX, CCP_MODE_COMPARE_CLEAR);
    }
    return written;
}

int EPIC_SWUART_Read(EPIC_SWUART_HandleTypeDef *h, uint8_t *buf, size_t maxlen)
{
    if (!h) return 0;
    size_t n = 0u;
    while (n < maxlen && h->rx_count > 0u) {
        uint8_t prev = EPIC_IRQ_Disable();
        buf[n] = h->rx_ring[h->rx_tail];
        h->rx_tail = (uint8_t)((h->rx_tail + 1u) & (EPIC_SWUART_RING_SZ - 1u));
        h->rx_count--;
        EPIC_IRQ_Restore(prev);
        n++;
    }
    return (int)n;
}

uint16_t EPIC_SWUART_GetErrorCount(const EPIC_SWUART_HandleTypeDef *h)
{
    if (!h) return 0u;
    uint8_t prev = EPIC_IRQ_Disable();          /* atomic 16-bit read */
    uint16_t count = h->error_count;
    EPIC_IRQ_Restore(prev);
    return count;
}
