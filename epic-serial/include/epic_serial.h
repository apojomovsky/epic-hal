/*
 * Family-agnostic interrupt-driven ring-buffered UART + printf retarget
 * (the non-blocking serial layer Cube's HAL_UART_Transmit_DMA/_IT gives)
 * for 8-bit PICs. RX/TX ISRs feed ring buffers via the USART handle's
 * callbacks; putch retargets XC8's printf to the TX ring.
 */

#ifndef EPIC_SERIAL_H
#define EPIC_SERIAL_H

#include <stdint.h>

/* Default ring-buffer size (power of two) for both TX and RX. Override by
 * defining EPIC_SERIAL_RING_SZ before including the header. */
#ifndef EPIC_SERIAL_RING_SZ
#define EPIC_SERIAL_RING_SZ 32u
#endif

/**
 * @brief Initialize the USART for async 8N1 at baud.
 *
 * Starts interrupt-driven RX (TX IT is armed on first write). Must be
 * called with interrupts enabled afterwards: init enables RCIE; TXIE is
 * enabled on demand by epic_serial_write.
 *
 * @param fosc_hz the system oscillator frequency in Hz
 * @param baud the desired baud rate
 */
void epic_serial_init(uint32_t fosc_hz, uint32_t baud);

/**
 * @brief Enqueue len bytes for background TX.
 *
 * Non-blocking: copies into the TX ring and enables the TX ISR to drain
 * it. If the ring fills, blocks until space frees (so the whole buffer
 * is sent).
 *
 * @param data bytes to transmit
 * @param len number of bytes to enqueue
 * @return the number of bytes enqueued (len unless len is 0)
 */
int epic_serial_write(const uint8_t *data, int len);

/**
 * @brief Pull up to max received bytes from the RX ring.
 *
 * Non-blocking.
 *
 * @param buf destination buffer
 * @param max maximum number of bytes to read
 * @return the number of bytes actually read (0 if nothing received)
 */
int epic_serial_read(uint8_t *buf, int max);

/**
 * @brief Report the number of bytes available to read from the RX ring.
 *
 * @return the number of received bytes buffered
 */
int epic_serial_available(void);

/**
 * @brief Report the number of bytes still pending in the TX ring.
 *
 * 0 means the ring is empty; the last byte may still be in the shift
 * register, use epic_serial_flush to wait for that.
 *
 * @return the number of bytes not yet loaded into TXREG
 */
int epic_serial_tx_pending(void);

/**
 * @brief Block until every enqueued TX byte has been transmitted.
 *
 * Waits for the TX ring to drain and the shift register to empty, so no
 * byte is mid-transmission on return. Use before sleep/reboot or to
 * pace output.
 */
void epic_serial_flush(void);

/**
 * @brief Emit one char through the TX ring (XC8 printf retarget).
 *
 * XC8's printf family calls putch per character, so defining this makes
 * printf stream over the UART on target. Host libc printf does not call
 * putch, so this is used directly only by target firmware.
 *
 * @param c the character to emit
 */
void putch(char c);

#endif /* EPIC_SERIAL_H */
