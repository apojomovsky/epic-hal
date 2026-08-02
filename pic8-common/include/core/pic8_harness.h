/**
 * @file    core/pic8_harness.h
 * @brief   Build-agnostic test/firmware harness, the one seam that lets the
 *          same example source build for the host simulator and a real XC8
 *          target with no `#ifdef` in the example code. Shared by every
 *          8-bit PIC family.
 *
 * @details
 *   Two execution models meet here:
 *
 *     - Host simulator: a normal program. Simulated time advances only
 *       when the family's sim step() is pumped, the test must terminate,
 *       and it reports pass/fail to stdout.
 *
 *     - Real target: firmware. Hardware time advances on its own, main()
 *       never returns, and there is no stdout.
 *
 *   The harness hides that difference behind four functions, each with a
 *   host implementation (the family's `pic8_harness_sim.c`, linked by the
 *   CMake build) and a target implementation (`pic8_harness_target.c` in
 *   this shared layer, linked by the XC8 Makefile). The build selects
 *   which file links; there is no `#ifdef` anywhere in the API or in
 *   example code that uses it. The target implementation is genuinely
 *   family-blind (four no-ops), so it lives here, not in any family tree;
 *   the host implementation is family-specific (it pumps that family's
 *   simulator) so each family provides its own.
 *
 *   Typical example shape (no `#ifdef` around code):
 *
 *       pic8_harness_init(SIM_CYCLES);
 *       // ... peripheral setup, identical on both builds ...
 *       for (uint32_t i = 0; pic8_harness_running(i); i++) {
 *           pic8_harness_tick();
 *           // ... work, same on both builds ...
 *       }
 *       pic8_harness_log("toggled %u\n", (unsigned)count);
 *       return pic8_harness_report(count >= 2);
 *
 *   On the target `harness_running` always returns 1, so the loop never
 *   exits and the log/report lines are unreachable. On the host the loop
 *   runs for `cycles` simulated cycles, then logs and returns the exit
 *   code.
 *
 *   Interrupt dispatch is also handled here: the host harness registers
 *   the family's @ref pic8_dispatch_all_irqs as the single sim IRQ
 *   callback, so examples never call the sim's set_irq_callback
 *   themselves. The real target gets the equivalent dispatch from its
 *   interrupt vector (the family's `pic*_isr_vector.c`). Each family
 *   implements `pic8_dispatch_all_irqs` with the same name (it fans out
 *   to that family's `HAL_*_IRQHandler` weak handlers), so the shared
 *   harness can name it without knowing which family it linked against.
 */

#ifndef PIC8_HARNESS_H
#define PIC8_HARNESS_H

#include <stdint.h>

/**
 * @brief  Harness start-up. On the host this resets the simulated CPU and
 *         wires the sim IRQ callback to the family dispatcher; `cycles`
 *         bounds the upcoming run. On a real target this is a no-op
 *         (the CPU starts itself; `cycles` is ignored).
 */
void pic8_harness_init(uint32_t cycles);

/**
 * @brief  Advance simulated time by one instruction cycle. On the host
 *         this pumps the simulator; on a real target time advances on its
 *         own, so this is a no-op. Call it inside any loop that must let
 *         time pass (the idle loop, or a hardware-ready wait).
 */
void pic8_harness_tick(void);

/**
 * @brief  Loop-continuation test. On the host returns 1 while the
 *         bounded run is in progress, 0 when it is over. On a real
 *         target always returns 1 (firmware runs forever).
 */
int pic8_harness_running(uint32_t iteration);

/**
 * @brief  printf-style log line. On the host this prints to stdout; on a
 *         real target it is a no-op (no stdout), so examples can log
 *         unconditionally without dragging in <stdio.h> or #ifdef.
 */
void pic8_harness_log(const char *fmt, ...);

/**
 * @brief  Map a pass/fail flag to a process exit code (0 = pass, 1 =
 *         fail), and, first, emit a fixed marker line through @ref
 *         pic8_harness_log so anything capturing that build's log output
 *         has a single, reliable line to check. Identical on every build
 *         and every family, so it is inlined here; what differs is
 *         pic8_harness_log's own per-build implementation (no-op on the
 *         host printed already-plenty diagnostics; no-op on a real
 *         target; and, for the sim-target build variant added in
 *         docs/ci-plan.md's Phase 3, an actual USART write, which is
 *         what turns this line into something MPLAB SIM's UART capture
 *         can give CI a PASS/FAIL grep on).
 */
static inline int pic8_harness_report(int ok)
{
    pic8_harness_log(ok ? "PIC8_HARNESS_RESULT: PASS\n"
                         : "PIC8_HARNESS_RESULT: FAIL\n");
    return ok ? 0 : 1;
}

/**
 * @brief  Fan out to every peripheral IRQHandler for the linked family.
 *         Each family implements this with the same name: it walks that
 *         family's interrupt sources and calls the matching
 *         `HAL_*_IRQHandler` weak handler whose flag is set. The host
 *         harness registers it as the sim IRQ callback; the real target
 *         calls it from its interrupt vector. Declared here so the
 *         harness can name it without a family-specific include.
 */
void pic8_dispatch_all_irqs(void);

#endif /* PIC8_HARNESS_H */
