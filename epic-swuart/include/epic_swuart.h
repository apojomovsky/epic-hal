/**
 * @file    epic_swuart.h
 * @brief   Bit-banged full-duplex UART, any GPIO pin, up to
 *          EPIC_SWUART_MAX_CHANNELS channels active at once.
 *
 * @details
 *   One shared Timer1 tick (see docs/ARCHITECTURE.md for the oversample
 *   factor and why Timer1) drives every active channel's TX bit-clock
 *   and RX sampler. 9600 baud only is validated; 8 data bits, no
 *   parity, 1 stop bit, not configurable. Non-blocking, ring-buffered,
 *   same shape as epic-serial's read/write.
 *
 *   This module owns Timer1 for as long as any channel is active, the
 *   same way epic_tick owns Timer2: do not also drive Timer1 directly
 *   from application code while a channel is initialised.
 */
#ifndef EPIC_SWUART_H
#define EPIC_SWUART_H

#include <stdint.h>
#include <stddef.h>
#include "epic_hal.h"

#ifndef EPIC_SWUART_MAX_CHANNELS
#define EPIC_SWUART_MAX_CHANNELS 2u
#endif

#ifndef EPIC_SWUART_RING_SZ
#define EPIC_SWUART_RING_SZ 8u
#endif

/* Ring index math masks with (EPIC_SWUART_RING_SZ - 1u) instead of
 * using '%', which only works if the size is a power of two. Catch a
 * future override that breaks that at compile time, not at runtime
 * with a silently wrong wraparound. */
_Static_assert((EPIC_SWUART_RING_SZ & (EPIC_SWUART_RING_SZ - 1u)) == 0u,
               "EPIC_SWUART_RING_SZ must be a power of two");

typedef struct {
    GPIO_TypeDef tx_port;
    uint16_t     tx_pin;
    GPIO_TypeDef rx_port;
    uint16_t     rx_pin;

    volatile uint8_t tx_state;
    volatile uint8_t tx_shift;
    volatile uint8_t tx_bit_index;
    volatile uint8_t tx_ticks_left;
    uint8_t          tx_ring[EPIC_SWUART_RING_SZ];
    volatile uint8_t tx_head;
    volatile uint8_t tx_tail;
    volatile uint8_t tx_count;

    volatile uint8_t rx_state;
    volatile uint8_t rx_shift;
    volatile uint8_t rx_bit_index;
    volatile uint8_t rx_ticks_left;
    uint8_t          rx_ring[EPIC_SWUART_RING_SZ];
    volatile uint8_t rx_head;
    volatile uint8_t rx_tail;
    volatile uint8_t rx_count;

    volatile uint16_t error_count;
    uint8_t           active;
} EPIC_SWUART_HandleTypeDef;

/**
 * @brief  Register a channel and, on the first call, start the shared
 *         Timer1 tick. `baud` is validated only at 9600; anything else
 *         is accepted but unsupported (see docs/API.md).
 *
 *         Precondition: the RX pin must idle high (pulled up, or connected
 *         to a live transmitter) whenever the channel is not actively
 *         receiving; a floating or held-low RX pin will be misread as a
 *         continuous stream of start bits.
 *
 * @return EPIC_OK, or EPIC_INVALID if `h` is NULL or the channel
 *         registry (EPIC_SWUART_MAX_CHANNELS) is full.
 */
EPIC_StatusTypeDef EPIC_SWUART_Init(EPIC_SWUART_HandleTypeDef *h,
                                     GPIO_TypeDef tx_port, uint16_t tx_pin,
                                     GPIO_TypeDef rx_port, uint16_t rx_pin,
                                     uint32_t fosc_hz, uint32_t baud);

/** Remove `h` from the shared registry. Stops Timer1 if no channel is
 *  left active. */
EPIC_StatusTypeDef EPIC_SWUART_DeInit(EPIC_SWUART_HandleTypeDef *h);

/** Enqueue up to `len` bytes. Returns the number actually queued (a
 *  short write when the TX ring is nearly full), never blocks. */
size_t EPIC_SWUART_Write(EPIC_SWUART_HandleTypeDef *h, const uint8_t *data, size_t len);

/** Drain up to `maxlen` received bytes into `buf`. Returns the number
 *  read, never blocks. */
int EPIC_SWUART_Read(EPIC_SWUART_HandleTypeDef *h, uint8_t *buf, size_t maxlen);

/** Running total of dropped bytes (bad stop bit or RX ring full) since
 *  Init. */
uint16_t EPIC_SWUART_GetErrorCount(const EPIC_SWUART_HandleTypeDef *h);

#endif /* EPIC_SWUART_H */
