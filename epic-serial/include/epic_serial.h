/**
 * @file    epic_serial.h
 * @brief   Family-agnostic interrupt-driven ring-buffered UART + printf
 *          retarget, the non-blocking serial layer Cube's
 *          `HAL_UART_Transmit_DMA`/`Receive_DMA`/`_IT` gives, for 8-bit
 *          PICs. RX/TX ISRs feed ring buffers via the USART handle's
 *          callbacks; `putch` retargets XC8's `printf` to the TX ring.
 */

#ifndef EPIC_SERIAL_H
#define EPIC_SERIAL_H

#include <stdint.h>

/** Default ring-buffer size (power of two) for both TX and RX. Override by
 *  defining EPIC_SERIAL_RING_SZ before including the header. */
#ifndef EPIC_SERIAL_RING_SZ
#define EPIC_SERIAL_RING_SZ 32u
#endif

/**
 * @brief  Initialize the USART for async 8N1 at @p baud and start
 *         interrupt-driven RX (and arm IT TX on first `epic_serial_write`).
 * @param  fosc_hz  system oscillator frequency in Hz.
 * @param  baud     baud rate, e.g. 9600.
 * @note   Must be called with interrupts enabled afterwards (it enables RCIE;
 *         TXIE is enabled on demand by `epic_serial_write`).
 */
void epic_serial_init(uint32_t fosc_hz, uint32_t baud);

/**
 * @brief  Enqueue @p len bytes for background TX. Non-blocking: copies into
 *         the TX ring and enables the TX ISR to drain it. If the ring fills,
 *         this blocks until space frees (so the whole buffer is sent).
 * @return the number of bytes enqueued (always @p len unless @p len is 0).
 */
int epic_serial_write(const uint8_t *data, int len);

/**
 * @brief  Pull up to @p max received bytes from the RX ring. Non-blocking.
 * @return the number of bytes actually read (0 if nothing received).
 */
int epic_serial_read(uint8_t *buf, int max);

/**
 * @brief  Number of bytes available to read from the RX ring.
 */
int epic_serial_available(void);

/**
 * @brief  Number of bytes still pending in the TX ring (not yet loaded into
 *         TXREG). 0 means the ring is empty (the last byte may still be in
 *         the shift register -- use `epic_serial_flush` to wait for that).
 */
int epic_serial_tx_pending(void);

/**
 * @brief  Block until every enqueued TX byte has left the TX ring AND the
 *         shift register has drained (so no byte is mid-transmission on
 *         return). Use before sleep/reboot or to pace output.
 */
void epic_serial_flush(void);

/**
 * @brief  XC8 `printf` retarget: emit one char through the TX ring. XC8's
 *         `<stdio.h>` `printf` family calls `putch` per character, so defining
 *         this makes `printf("x=%u\n", x)` stream over the UART on target.
 *         On the host build libc `printf` does not call `putch`, so this is
 *         used directly only by target firmware.
 */
void putch(char c);

#endif /* EPIC_SERIAL_H */
