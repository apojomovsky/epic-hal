/**
 * @file    epic_swuart.c
 * @brief   Bit-banged full-duplex UART. See docs/ARCHITECTURE.md for
 *          the shared-tick design and docs/API.md for per-function
 *          semantics.
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
static uint16_t g_last_delta = 0u;
static uint8_t  g_timer_running = 0u;

/* 0xFFFFu is the "not scheduled" sentinel: larger than any real
 * countdown (max 65535 only if FOSC_HZ/baud rounds to exactly that,
 * which never happens at any real family/baud combination this module
 * supports, so this sentinel is safe). */
#define SWUART_NOT_SCHEDULED 0xFFFFu

static uint16_t tx_due_in(const EPIC_SWUART_HandleTypeDef *h)
{
    if (h->tx_state != TX_IDLE || h->tx_count > 0u) return h->tx_ticks_left;
    return SWUART_NOT_SCHEDULED;
}

static void tx_step(EPIC_SWUART_HandleTypeDef *h)
{
    h->tx_ticks_left = g_cycles_per_bit;

    switch (h->tx_state) {
    case TX_IDLE:
        if (h->tx_count == 0u) {
            EPIC_GPIO_WritePin(h->tx_port, h->tx_pin, GPIO_PIN_SET);
            return;
        }
        h->tx_shift = h->tx_ring[h->tx_tail];
        h->tx_tail = (uint8_t)((h->tx_tail + 1u) & (EPIC_SWUART_RING_SZ - 1u));
        h->tx_count--;
        h->tx_bit_index = 0u;
        EPIC_GPIO_WritePin(h->tx_port, h->tx_pin, GPIO_PIN_RESET); /* start bit */
        h->tx_state = TX_DATA;
        break;
    case TX_DATA:
        EPIC_GPIO_WritePin(h->tx_port, h->tx_pin,
                            (h->tx_shift & 0x01u) ? GPIO_PIN_SET : GPIO_PIN_RESET);
        h->tx_shift >>= 1;
        h->tx_bit_index++;
        if (h->tx_bit_index >= 8u) {
            h->tx_state = TX_STOP;
        }
        break;
    case TX_STOP:
        EPIC_GPIO_WritePin(h->tx_port, h->tx_pin, GPIO_PIN_SET);
        h->tx_state = TX_IDLE;
        break;
    default:
        h->tx_state = TX_IDLE;
        break;
    }
}

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

static void rx_step(EPIC_SWUART_HandleTypeDef *h)
{
    if (h->rx_state == RX_CONFIRM_START) {
        if (EPIC_GPIO_ReadPin(h->rx_port, h->rx_pin) != GPIO_PIN_RESET) {
            h->rx_state = RX_IDLE; /* noise, not a real start bit */
            return;
        }
        h->rx_shift = 0u;
        h->rx_bit_index = 0u;
        h->rx_state = RX_DATA0;
        h->rx_ticks_left = g_cycles_per_bit;
        return;
    }

    uint8_t sample = (EPIC_GPIO_ReadPin(h->rx_port, h->rx_pin) == GPIO_PIN_SET) ? 1u : 0u;

    if (h->rx_state == RX_STOP) {
        if (sample != 0u) {
            rx_push(h, h->rx_shift);
        } else {
            h->error_count++; /* bad stop bit, drop the byte */
        }
        h->rx_state = RX_IDLE;
        return;
    }

    h->rx_shift = (uint8_t)((h->rx_shift >> 1) | (sample ? 0x80u : 0u));
    h->rx_bit_index++;
    h->rx_state = (h->rx_bit_index < 8u) ? (uint8_t)(RX_DATA0 + h->rx_bit_index) : RX_STOP;
    h->rx_ticks_left = g_cycles_per_bit;
}

static TIMER1_HandleTypeDef s_timer1 = TIMER1_HANDLE_DEFAULT;

static EPIC_SWUART_HandleTypeDef *g_chan_a = NULL;
static EPIC_SWUART_HandleTypeDef *g_chan_b = NULL;

static uint16_t rx_due_in(const EPIC_SWUART_HandleTypeDef *h)
{
    if (h->rx_state != RX_IDLE) return h->rx_ticks_left;
    return SWUART_NOT_SCHEDULED;
}

static void reschedule(void)
{
    uint16_t min_delta = SWUART_NOT_SCHEDULED;

    if (g_chan_a != NULL) {
        uint16_t t = tx_due_in(g_chan_a);
        if (t < min_delta) min_delta = t;
        uint16_t r = rx_due_in(g_chan_a);
        if (r < min_delta) min_delta = r;
    }
    if (g_chan_b != NULL) {
        uint16_t t = tx_due_in(g_chan_b);
        if (t < min_delta) min_delta = t;
        uint16_t r = rx_due_in(g_chan_b);
        if (r < min_delta) min_delta = r;
    }

    if (min_delta == SWUART_NOT_SCHEDULED) {
        EPIC_TIMER1_Stop();
        g_timer_running = 0u;
        return;
    }
    /* A "due immediately" request (Write() sets tx_ticks_left = 0) means
     * zero cycles of *waiting*, not a zero-cycle timer arm: hardware
     * cannot fire in 0 cycles, and 65536u - 0u truncates to 0x0000,
     * which would arm a full 65536-cycle wait instead of firing on the
     * next tick (confirmed with a throwaway probe against the sim: TMR1
     * stayed at the written value with no overflow for 600+ ticks).
     * Floor at 1 so "due now" really means "next tick", and g_last_delta
     * stays consistent with what was actually armed. */
    if (min_delta == 0u) min_delta = 1u;
    g_last_delta = min_delta;
    if (!g_timer_running) {
        EPIC_TIMER1_Start(&s_timer1);
        g_timer_running = 1u;
    }
    EPIC_TIMER1_WriteCounter((uint16_t)(65536u - min_delta));
}

static void on_timer1_overflow(void)
{
    uint16_t elapsed = g_last_delta;

    if (g_chan_a != NULL) {
        if (tx_due_in(g_chan_a) != SWUART_NOT_SCHEDULED) {
            if (g_chan_a->tx_ticks_left <= elapsed) tx_step(g_chan_a);
            else g_chan_a->tx_ticks_left = (uint16_t)(g_chan_a->tx_ticks_left - elapsed);
        }
        if (rx_due_in(g_chan_a) != SWUART_NOT_SCHEDULED) {
            if (g_chan_a->rx_ticks_left <= elapsed) rx_step(g_chan_a);
            else g_chan_a->rx_ticks_left = (uint16_t)(g_chan_a->rx_ticks_left - elapsed);
        }
    }
    if (g_chan_b != NULL) {
        if (tx_due_in(g_chan_b) != SWUART_NOT_SCHEDULED) {
            if (g_chan_b->tx_ticks_left <= elapsed) tx_step(g_chan_b);
            else g_chan_b->tx_ticks_left = (uint16_t)(g_chan_b->tx_ticks_left - elapsed);
        }
        if (rx_due_in(g_chan_b) != SWUART_NOT_SCHEDULED) {
            if (g_chan_b->rx_ticks_left <= elapsed) rx_step(g_chan_b);
            else g_chan_b->rx_ticks_left = (uint16_t)(g_chan_b->rx_ticks_left - elapsed);
        }
    }

    reschedule();
}

static void on_rx_edge_start(EPIC_SWUART_HandleTypeDef *h)
{
    if (h->rx_state != RX_IDLE) return; /* mid-frame, not a new start */
    if (EPIC_GPIO_ReadPin(h->rx_port, h->rx_pin) != GPIO_PIN_RESET) return;
    h->rx_state = RX_CONFIRM_START;
    h->rx_ticks_left = g_cycles_per_bit / 2u;
    reschedule();
}

#if defined(PIC18F2455) || defined(PIC18F2550) || defined(PIC18F4455) || defined(PIC18F4550)
static void on_port_change(uint8_t portb)
{
    (void)portb;
    if (g_chan_a != NULL) on_rx_edge_start(g_chan_a);
    if (g_chan_b != NULL) on_rx_edge_start(g_chan_b);
}
static void arm_rx_change_interrupt(void)
{
    EPIC_GPIO_RegisterChangeCallback(on_port_change);
    EPIC_IRQ_Enable(PIC18_IRQ_RB);
}
#elif defined(PIC16F1933) || defined(PIC16F1934) || defined(PIC16F1936) || \
      defined(PIC16F1937) || defined(PIC16F1938) || defined(PIC16F1939)
static void on_port_change(uint8_t iocbf, uint8_t portb)
{
    (void)iocbf; (void)portb;
    if (g_chan_a != NULL) on_rx_edge_start(g_chan_a);
    if (g_chan_b != NULL) on_rx_edge_start(g_chan_b);
}
/* Accumulates the negative-edge mask across every registered channel's
 * RX pin: unlike classic PIC16/PIC18's RBIF (fires on any RB4:7 change
 * regardless of a per-pin mask), PIC16F193X's IOC only interrupts on
 * pins actually set in IOCBN. Called once per registration (see
 * EPIC_SWUART_Init), so a second channel's pin gets OR'd in rather than
 * overwriting the first channel's. */
static uint8_t g_ioc_neg_mask = 0u;
static void arm_rx_change_interrupt(EPIC_SWUART_HandleTypeDef *h)
{
    g_ioc_neg_mask |= (uint8_t)h->rx_pin;
    EPIC_GPIO_EnableChangeDetect(0u, g_ioc_neg_mask);
    EPIC_GPIO_RegisterChangeCallback(on_port_change);
    EPIC_IRQ_Enable(PIC16F193X_IRQ_IOC);
}
#else
static void on_port_change(uint8_t portb)
{
    (void)portb;
    if (g_chan_a != NULL) on_rx_edge_start(g_chan_a);
    if (g_chan_b != NULL) on_rx_edge_start(g_chan_b);
}
static void arm_rx_change_interrupt(void)
{
    EPIC_GPIO_RegisterChangeCallback(on_port_change);
    EPIC_IRQ_Enable(PIC16_IRQ_RB);
}
#endif

EPIC_StatusTypeDef EPIC_SWUART_Init(EPIC_SWUART_HandleTypeDef *h,
                                     GPIO_TypeDef tx_port, uint16_t tx_pin,
                                     GPIO_TypeDef rx_port, uint16_t rx_pin,
                                     uint32_t fosc_hz, uint32_t baud)
{
    if (!h || (g_chan_a != NULL && g_chan_b != NULL)) return EPIC_INVALID;

    h->tx_port = tx_port; h->tx_pin = tx_pin;
    h->rx_port = rx_port; h->rx_pin = rx_pin;
    h->tx_state = TX_IDLE; h->tx_ticks_left = 0u;
    h->tx_head = h->tx_tail = h->tx_count = 0u;
    h->rx_state = RX_IDLE; h->rx_ticks_left = 0u;
    h->rx_head = h->rx_tail = h->rx_count = 0u;
    h->error_count = 0u;
    h->active = 1u;

    EPIC_GPIO_Init(tx_port, tx_pin, GPIO_MODE_OUTPUT);
    EPIC_GPIO_WritePin(tx_port, tx_pin, GPIO_PIN_SET); /* idle = mark */
    EPIC_GPIO_Init(rx_port, rx_pin, GPIO_MODE_INPUT);

    if (g_chan_a == NULL && g_chan_b == NULL) {
        g_cycles_per_bit = compute_cycles_per_bit(fosc_hz, baud);
        s_timer1 = (TIMER1_HandleTypeDef)TIMER1_HANDLE_DEFAULT;
        s_timer1.OverflowCallback = on_timer1_overflow;
        EPIC_TIMER1_Init(&s_timer1);
        EPIC_IRQ_Restore(1);
    }
    /* Runs on every successful registration, not just the first: on
     * PIC16F193X the IOC negative-edge mask must accumulate each
     * channel's RX pin (see arm_rx_change_interrupt's comment), so the
     * second channel needs this call too. Re-registering the same
     * callback and re-enabling an already-enabled IRQ is harmless on
     * all three families (RegisterChangeCallback is a plain pointer
     * store, IRQ_Enable only ORs bits), so PIC16F87XA/PIC18Fxx5x's
     * no-arg variant runs here unconditionally too rather than being
     * split out to first-registration-only. */
#if defined(PIC16F1933) || defined(PIC16F1934) || defined(PIC16F1936) || \
    defined(PIC16F1937) || defined(PIC16F1938) || defined(PIC16F1939)
    arm_rx_change_interrupt(h);
#else
    arm_rx_change_interrupt();
#endif

    uint8_t prev_reg = EPIC_IRQ_Disable();
    if (g_chan_a == NULL) {
        g_chan_a = h;
    } else {
        g_chan_b = h;
    }
    EPIC_IRQ_Restore(prev_reg);
    return EPIC_OK;
}

EPIC_StatusTypeDef EPIC_SWUART_DeInit(EPIC_SWUART_HandleTypeDef *h)
{
    if (!h) return EPIC_INVALID;
    uint8_t prev = EPIC_IRQ_Disable();
    if (g_chan_a == h) {
        g_chan_a = NULL;
    } else if (g_chan_b == h) {
        g_chan_b = NULL;
    }
    EPIC_IRQ_Restore(prev);
    /* Idle/mark, not whatever level the state machine was at mid-frame:
     * leaving TX low here would be a break condition on the wire. */
    EPIC_GPIO_WritePin(h->tx_port, h->tx_pin, GPIO_PIN_SET);
    if (g_chan_a == NULL && g_chan_b == NULL) {
        /* EPIC_TIMER1_Stop() only clears TMR1ON; the module claims to
         * fully release Timer1 when the registry goes empty, so the
         * interrupt source must go with it, not just the counter. */
        EPIC_TIMER1_DeInit();
        g_timer_running = 0u;
    }
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
        h->tx_ticks_left = 0u; /* due immediately: start the frame now */
        reschedule();
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
