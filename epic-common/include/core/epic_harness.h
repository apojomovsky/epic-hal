/**
 * Build-agnostic test/firmware harness: one example source builds for
 * the host simulator and a real XC8 target with no `#ifdef`; host is a
 * bounded program that pumps simulated time, target runs forever.
 */

#ifndef EPIC_HARNESS_H
#define EPIC_HARNESS_H

#include <stdint.h>

/**
 * @brief  Harness start-up. On the host this resets the simulated CPU and
 *         wires the sim IRQ callback to the family dispatcher; `cycles`
 *         bounds the upcoming run. On a real target this is a no-op
 *         (the CPU starts itself; `cycles` is ignored).
 */
void epic_harness_init(uint32_t cycles);

/**
 * @brief  Advance simulated time by one instruction cycle. On the host
 *         this pumps the simulator; on a real target time advances on its
 *         own, so this is a no-op. Call it inside any loop that must let
 *         time pass (the idle loop, or a hardware-ready wait).
 */
void epic_harness_tick(void);

/**
 * @brief  Loop-continuation test. On the host returns 1 while the
 *         bounded run is in progress, 0 when it is over. On a real
 *         target always returns 1 (firmware runs forever).
 */
int epic_harness_running(uint32_t iteration);

/**
 * @brief  printf-style log line. On the host this prints to stdout; on a
 *         real target it is a no-op (no stdout), so examples can log
 *         unconditionally without dragging in <stdio.h> or #ifdef.
 */
void epic_harness_log(const char *fmt, ...);

/**
 * @brief  Map a pass/fail flag to a process exit code (0 = pass, 1 =
 *         fail), first emitting a fixed marker line through @ref
 *         epic_harness_log so any build's captured output (including a
 *         sim-target UART capture) has one reliable line to grep.
 */
static inline int epic_harness_report(int ok)
{
    epic_harness_log(ok ? "EPIC_HARNESS_RESULT: PASS\n"
                         : "EPIC_HARNESS_RESULT: FAIL\n");
    return ok ? 0 : 1;
}

/**
 * @brief  Fan out to every peripheral IRQHandler for the linked family.
 *         Each family implements this with the same name: it walks that
 *         family's interrupt sources and calls the matching
 *         `EPIC_*_IRQHandler` weak handler whose flag is set. The host
 *         harness registers it as the sim IRQ callback; the real target
 *         calls it from its interrupt vector. Declared here so the
 *         harness can name it without a family-specific include.
 */
void epic_dispatch_all_irqs(void);

#endif /* EPIC_HARNESS_H */
