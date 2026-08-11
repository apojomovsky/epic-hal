/* Bit-banged full-duplex UART on CCP hardware capture/compare timing.
 * Channel capacity is a hardware ceiling (1 on PIC16F87XA/PIC18Fxx5x,
 * 2 on PIC16F193X), not configurable; RX/TX pins are fixed by the CCP
 * wiring and validated in Init. While a channel is active the module
 * owns Timer1 and the CCP instances listed in docs/ARCHITECTURE.md. */
#ifndef EPIC_SWUART_H
#define EPIC_SWUART_H

#include <stdint.h>
#include <stddef.h>
#include "epic_hal.h"

/* Channel B needs GPIOD (RD1, its TX pin), which only the 40/44-pin
 * PIC16F193X variants (1934/1937/1939) have (PIC16F193X_FAMILY_HAS_PORTD,
 * per-device in pic16f193x.h); on the 28-pin variants GPIOD is an
 * undeclared identifier, so channel B must not compile there. PORTD
 * alone is not enough: PIC16F1934 has PORTD but only 4KW of flash
 * (PIC16F193X_FAMILY_FLASH_KW), and the compiled real-target example
 * with channel B needs 4127 words, 31 over 1934's 4096-word budget
 * (measured XC8 build). Requiring >=8KW alongside PORTD excludes
 * exactly 1934 and nothing else. */
#if (defined(PIC16F1933) || defined(PIC16F1934) || defined(PIC16F1936) || \
     defined(PIC16F1937) || defined(PIC16F1938) || defined(PIC16F1939)) && \
    PIC16F193X_FAMILY_HAS_PORTD && (PIC16F193X_FAMILY_FLASH_KW >= 8)
#define EPIC_SWUART_MAX_CHANNELS 2u
#else
#define EPIC_SWUART_MAX_CHANNELS 1u
#endif

/* PIC16F87XA detection for the RX hot-path fix (rx_capture_event_fast
 * hardcodes CCP1's literal SFR addresses, 0x15/0x16/0x17, which are
 * only valid on this family). PIC18Fxx5x's CCPR1L is 0xFBE and
 * PIC16F193X's is 0x291, so the fast path must not compile there;
 * channel A on those families keeps the generic rx_capture_event until
 * a follow-up ports the pattern with their own literal addresses. */
#if defined(PIC16F1933) || defined(PIC16F1934) || defined(PIC16F1936) || \
    defined(PIC16F1937) || defined(PIC16F1938) || defined(PIC16F1939) || \
    defined(PIC18F2455) || defined(PIC18F2550) || defined(PIC18F4455) || \
    defined(PIC18F4550)
#define EPIC_SWUART_HAS_RX_FAST_PATH 0u
#else
#define EPIC_SWUART_HAS_RX_FAST_PATH 1u
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

    volatile uint8_t  tx_state;
    volatile uint8_t  tx_shift;
    volatile uint8_t  tx_bit_index;
    volatile uint16_t tx_deadline;   /**< Absolute Timer1 count, valid
                                       *   only while tx_state != TX_IDLE
                                       *   or the TX CCP module is armed. */
    uint8_t          tx_ring[EPIC_SWUART_RING_SZ];
    volatile uint8_t tx_head;
    volatile uint8_t tx_tail;
    volatile uint8_t tx_count;

    volatile uint8_t  rx_state;
    volatile uint8_t  rx_shift;
    volatile uint8_t  rx_bit_index;
    volatile uint16_t rx_deadline;   /**< Absolute Timer1 count, valid
                                       *   only while rx_state != RX_IDLE. */
    uint8_t          rx_ring[EPIC_SWUART_RING_SZ];
    volatile uint8_t rx_head;
    volatile uint8_t rx_tail;
    volatile uint8_t rx_count;

    volatile uint16_t error_count;
} EPIC_SWUART_HandleTypeDef;

/**
 * @brief  Register a channel. `tx_port`/`tx_pin`/`rx_port`/`rx_pin`
 *         must match the fixed pins wired to this channel slot's CCP
 *         instances (see docs/ARCHITECTURE.md for the per-family
 *         table); a mismatch returns EPIC_INVALID, the same as a NULL
 *         handle or an already-full channel registry.
 *         `baud` is validated only at 9600; anything else is accepted
 *         but unsupported (see docs/API.md).
 *
 *         Precondition: the RX pin must idle high (pulled up, or connected
 *         to a live transmitter) whenever the channel is not actively
 *         receiving; a floating or held-low RX pin will be misread as a
 *         start bit.
 * @param h        handle to register; its ring/state fields are reset.
 * @param tx_port  GPIO port of the channel's fixed TX pin (CCP compare output).
 * @param tx_pin   GPIO pin of the channel's fixed TX pin.
 * @param rx_port  GPIO port of the channel's fixed RX pin (CCP capture input).
 * @param rx_pin   GPIO pin of the channel's fixed RX pin.
 * @param fosc_hz  system oscillator frequency in Hz, drives bit timing.
 * @param baud     desired baud rate; validated only at 9600, other values
 *                 are accepted but unsupported (see docs/API.md).
 * @return EPIC_OK, or EPIC_INVALID if `h` is NULL, the pins don't match
 *         this slot's fixed CCP pins, or all EPIC_SWUART_MAX_CHANNELS
 *         slots are already in use.
 */
EPIC_StatusTypeDef EPIC_SWUART_Init(EPIC_SWUART_HandleTypeDef *h,
                                     GPIO_TypeDef tx_port, uint16_t tx_pin,
                                     GPIO_TypeDef rx_port, uint16_t rx_pin,
                                     uint32_t fosc_hz, uint32_t baud);

/**
 * @brief  Remove `h` from the shared registry. Stops Timer1 if no channel
 *         is left active.
 * @param h the handle previously registered with EPIC_SWUART_Init.
 * @return EPIC_OK, or EPIC_INVALID if `h` is NULL or not a registered
 *         channel.
 */
EPIC_StatusTypeDef EPIC_SWUART_DeInit(EPIC_SWUART_HandleTypeDef *h);

/**
 * @brief  Enqueue up to `len` bytes. Returns the number actually queued (a
 *         short write when the TX ring is nearly full), never blocks.
 * @param h    registered channel handle.
 * @param data bytes to transmit.
 * @param len  number of bytes in `data`.
 * @return the number of bytes actually queued, less than `len` only when
 *         the TX ring is nearly full.
 */
size_t EPIC_SWUART_Write(EPIC_SWUART_HandleTypeDef *h, const uint8_t *data, size_t len);

/**
 * @brief  Drain up to `maxlen` received bytes into `buf`. Returns the number
 *         read, never blocks.
 * @param h      registered channel handle.
 * @param buf    destination buffer for the received bytes.
 * @param maxlen capacity of `buf`.
 * @return the number of bytes read, 0 when the RX ring is empty.
 */
int EPIC_SWUART_Read(EPIC_SWUART_HandleTypeDef *h, uint8_t *buf, size_t maxlen);

/**
 * @brief  Running total of dropped bytes (bad stop bit or RX ring full)
 *         since Init.
 * @param h registered channel handle.
 * @return the channel's accumulated dropped-byte count.
 */
uint16_t EPIC_SWUART_GetErrorCount(const EPIC_SWUART_HandleTypeDef *h);

#endif /* EPIC_SWUART_H */
