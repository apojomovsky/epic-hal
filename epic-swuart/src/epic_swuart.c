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

static EPIC_SWUART_HandleTypeDef *g_channels[EPIC_SWUART_MAX_CHANNELS];
static uint8_t g_channel_count = 0;
static uint16_t g_reload = 0;
static uint8_t g_oversample_n = 3u; /* Task 1's probe verdict + safety margin, see OVERSAMPLE_N above. */

/* reload = 65536 - round(fosc_hz / 4 / (baud * N)), Timer1 prescaler
 * 1:1. Timer1 does not auto-reload on overflow (it is a free-running
 * 16-bit counter, unlike Timer2's period-register peripherals), so the
 * ISR below rewrites TMR1H:TMR1L on every tick, not just at Start. */
static uint16_t compute_reload(uint32_t fosc_hz, uint32_t baud, uint8_t n)
{
    uint32_t ticks_per_period = (fosc_hz / 4u + (baud * n) / 2u) / (baud * n);
    if (ticks_per_period > 65535u) ticks_per_period = 65535u;
    if (ticks_per_period < 1u) ticks_per_period = 1u;
    return (uint16_t)(65536u - ticks_per_period);
}

static void tx_step(EPIC_SWUART_HandleTypeDef *h)
{
    if (h->tx_ticks_left != 0u) {
        h->tx_ticks_left--;
        return;
    }
    h->tx_ticks_left = g_oversample_n - 1u;

    switch (h->tx_state) {
    case TX_IDLE:
        if (h->tx_count == 0u) {
            EPIC_GPIO_WritePin(h->tx_port, h->tx_pin, GPIO_PIN_SET);
            return;
        }
        h->tx_shift = h->tx_ring[h->tx_tail];
        h->tx_tail = (uint8_t)((h->tx_tail + 1u) % EPIC_SWUART_RING_SZ);
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
    h->rx_head = (uint8_t)((h->rx_head + 1u) % EPIC_SWUART_RING_SZ);
    h->rx_count++;
}

static void rx_step(EPIC_SWUART_HandleTypeDef *h)
{
    if (h->rx_state == RX_IDLE) {
        if (EPIC_GPIO_ReadPin(h->rx_port, h->rx_pin) == GPIO_PIN_RESET) {
            h->rx_state = RX_CONFIRM_START;
            h->rx_ticks_left = g_oversample_n / 2u - 1u;
        }
        return;
    }

    if (h->rx_ticks_left != 0u) {
        h->rx_ticks_left--;
        return;
    }

    if (h->rx_state == RX_CONFIRM_START) {
        if (EPIC_GPIO_ReadPin(h->rx_port, h->rx_pin) != GPIO_PIN_RESET) {
            h->rx_state = RX_IDLE; /* noise, not a real start bit */
            return;
        }
        h->rx_shift = 0u;
        h->rx_bit_index = 0u;
        h->rx_state = RX_DATA0;
        h->rx_ticks_left = g_oversample_n - 1u;
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
    h->rx_ticks_left = g_oversample_n - 1u;
}

static void shared_tick(void)
{
    EPIC_TIMER1_WriteCounter(g_reload);
    for (uint8_t i = 0; i < g_channel_count; i++) {
        EPIC_SWUART_HandleTypeDef *h = g_channels[i];
        if (!h->active) continue;
        tx_step(h);
        rx_step(h);
    }
}

static TIMER1_HandleTypeDef s_timer1 = TIMER1_HANDLE_DEFAULT;

EPIC_StatusTypeDef EPIC_SWUART_Init(EPIC_SWUART_HandleTypeDef *h,
                                     GPIO_TypeDef tx_port, uint16_t tx_pin,
                                     GPIO_TypeDef rx_port, uint16_t rx_pin,
                                     uint32_t fosc_hz, uint32_t baud)
{
    if (!h || g_channel_count >= EPIC_SWUART_MAX_CHANNELS) return EPIC_INVALID;

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

    if (g_channel_count == 0u) {
        g_reload = compute_reload(fosc_hz, baud, g_oversample_n);
        s_timer1 = (TIMER1_HandleTypeDef)TIMER1_HANDLE_DEFAULT;
        s_timer1.ReloadValue = g_reload;
        s_timer1.OverflowCallback = shared_tick;
        EPIC_TIMER1_Init(&s_timer1);
        EPIC_TIMER1_Start(&s_timer1);
        EPIC_IRQ_Restore(1);
    }

    g_channels[g_channel_count] = h;
    g_channel_count++;
    return EPIC_OK;
}

EPIC_StatusTypeDef EPIC_SWUART_DeInit(EPIC_SWUART_HandleTypeDef *h)
{
    if (!h) return EPIC_INVALID;
    for (uint8_t i = 0; i < g_channel_count; i++) {
        if (g_channels[i] == h) {
            h->active = 0u;
            for (uint8_t j = i; j + 1u < g_channel_count; j++) {
                g_channels[j] = g_channels[j + 1u];
            }
            g_channel_count--;
            break;
        }
    }
    if (g_channel_count == 0u) {
        EPIC_TIMER1_Stop();
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
        h->tx_head = (uint8_t)((h->tx_head + 1u) % EPIC_SWUART_RING_SZ);
        h->tx_count++;
        EPIC_IRQ_Restore(prev);
        written++;
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
        h->rx_tail = (uint8_t)((h->rx_tail + 1u) % EPIC_SWUART_RING_SZ);
        h->rx_count--;
        EPIC_IRQ_Restore(prev);
        n++;
    }
    return (int)n;
}

uint16_t EPIC_SWUART_GetErrorCount(const EPIC_SWUART_HandleTypeDef *h)
{
    return h ? h->error_count : 0u;
}
