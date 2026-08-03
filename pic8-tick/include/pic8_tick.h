/**
 * @file    pic8_tick.h
 * @brief   Family-agnostic 1 ms timebase (`pic8_tick_get`/`pic8_tick_delay_ms`,
 *          the STM32Cube `HAL_GetTick`/`HAL_Delay` equivalent) built on the
 *          HAL's auto-reload Timer2: a period-match ISR increments a
 *          volatile 32-bit counter this header's functions read. See
 *          docs/ARCHITECTURE.md for the timebase math.
 */

#ifndef PIC8_TICK_H
#define PIC8_TICK_H

#include <stdint.h>

/**
 * @brief  Start the 1 ms timebase: configures Timer2 for the closest
 *         achievable 1 ms period from @p fosc_hz and enables its ISR.
 *         Call once at startup.
 * @param  fosc_hz  the system oscillator frequency in Hz (e.g. 20000000UL).
 */
void pic8_tick_init(uint32_t fosc_hz);

/**
 * @brief  Read the elapsed milliseconds since `pic8_tick_init`. Monotonic;
 *         wraps every ~49.7 days (2^32 ms). The 32-bit read is made atomic
 *         against the ISR (interrupts disabled around the read) so a
 *         mid-update tear cannot occur.
 * @return the millisecond tick count.
 */
uint32_t pic8_tick_get(void);

/**
 * @brief  Block for @p ms milliseconds. On the host sim this pumps
 *         `pic8_harness_tick()` so simulated time advances; on a real target
 *         it spins while the Timer2 ISR advances the counter. Guarantees at
 *         least @p ms (may overshoot by up to ~1 tick).
 */
void pic8_tick_delay_ms(uint32_t ms);

/**
 * @brief  Non-blocking elapsed-time helper: `pic8_tick_get() - t0`, the
 *         idiom for `if (pic8_tick_elapsed_since(t0) >= timeout)` without
 *         blocking. Wraparound-safe (unsigned subtraction).
 */
uint32_t pic8_tick_elapsed_since(uint32_t t0);

#endif /* PIC8_TICK_H */
