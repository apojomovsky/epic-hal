/**
 * @file    combo_taskmgr_serial.c
 * @brief   C8 of the combination matrix
 *          (docs/superpowers/plans/2026-08-09-combination-matrix.md):
 *          epic-taskmgr + epic-serial together, one firmware, one CPU.
 *          Three cooperative scheduler tasks with distinct periods each
 *          push a known 3-byte payload through epic_serial_write every
 *          time they run; the ring is drained between scheduler rounds
 *          and the cross-checks verify the run counts match the period
 *          ratios, every pushed byte was popped exactly once, the ring
 *          drained fully, and the tick counter advanced. The task-vs-
 *          ring interleave is the point: the three tasks' payloads
 *          multiplex through one 32-byte ring at three different rates
 *          while the tick keeps arming them.
 *
 * @details
 *   Family choice: PIC18Fxx5x (18F4550), the same reason as
 *   epic-taskmgr's own sim gate (see its manifest comment): the PIC16
 *   Timer0-tick ISR path is unreliable under SIM (ISR storms, a GIE
 *   bit set that never persists), while PIC18's absolute-call vector
 *   model works, and epic-tick's PIC18 sim gate already proves PIC18
 *   timer interrupts work under SIM.
 *
 *   Interrupt management mirrors the epic-serial gate's GIE-managed
 *   drain pattern (sim_serial.c), deliberately GIE OFF for the whole
 *   run. Two reasons. First, determinism: the popped == pushed
 *   cross-check below requires every pop to happen in the gate's own
 *   counted drain; with GIE on, a tick vector landing between a task's
 *   write and the drain would pop in ISR context and uncouple the
 *   counters. Second, the PIC16 evidence: MPLAB SIM keeps TXIF set
 *   while TXREG holds a byte, so TXIE enabled with GIE set makes the
 *   TX ISR fire continuously, and the in-ISR Disable/Restore churn of
 *   the serial callback trips the simulator's asynchronous-interrupt-
 *   delivery quirk (a request latched while GIE was set can vector
 *   inside a disabled window and leave GIE cleared), wedging GIE --
 *   observed on PIC16 (sim_serial.c / epic_tick.c); it did NOT
 *   reproduce on PIC18 (C12's live-GIE gate, combo-modbus-full). The
 *   gate therefore drives everything through manual
 *   epic_dispatch_all_irqs() calls: the Timer0 overflow still latches
 *   TMR0IF in real simulated time and the dispatch runs
 *   TIMER0_IRQHandler -> task_manager_tick exactly like the ISR would
 *   (the tick counter is real), and each dispatch additionally pops
 *   one TX ring byte. Every pop is paced per byte (wait for the shift
 *   register to empty) like the console gate's drain, so the uart1io
 *   capture receives every payload byte intact and the transmitted
 *   stream is byte-exact and contiguous in the captured UART output
 *   (nothing else logs during the run; the only polled report bytes
 *   come after the loop).
 *
 *   Tick rate: 1:16 prescaler, reload 0 -> 256 counts x 16 = 4096
 *   instruction cycles per tick (~341 us at the 48 MHz / 12 MHz
 *   instruction rate this family's sim harness builds with). The run
 *   accumulates ~100-200 ticks, far past the 80 ticks the checks need
 *   (two full 40-tick periods of the slowest task), and the dispatch
 *   cost is a small fraction of the tick period so the scheduler
 *   loop keeps making progress between overflows.
 *
 *   Why the exact ratio bounds: with periods 10/20/40, task A fires
 *   at ticks 10, 20, 30, ..., B at 20, 40, ..., C at 40, 80, .... For
 *   any total tick count T, runs_A = T/10, runs_B = T/20,
 *   runs_C = T/40 (integer division), so runs_A is always in
 *   [4*runs_C, 4*runs_C + 3] and runs_B in [2*runs_C, 2*runs_C + 1]:
 *   between two C firings, A can fire at most three extra times (at
 *   the 10/20/30 offsets inside a 40-tick window) and B at most one
 *   extra time. The bounds hold for every T, so they are exact
 *   period-ratio checks without depending on when the run stopped.
 *   The interleave check: pushed == popped, i.e. the 3-byte payloads
 *   of all three tasks' firings entered the ring and left it exactly
 *   once each (GIE is off, so every pop happens in the gate's own
 *   drain and is counted; a ring corruption would desync the counts).
 *   The byte-exact stream in the captured UART output is the external
 *   half: the first two full 40-tick cycles (guaranteed once
 *   runs_C >= 2) produce the fixed 48-byte prefix
 *   "<A><A><B><A><A><B><C><A><A><B><A><A><B><C>" contiguous in the
 *   capture, and the per-task counts are logged for comparison.
 *
 *   Bounded and self-reporting (the harness contract); no RX
 *   involvement, so the MPLAB SIM RX wall does not apply.
 */

#include "task_manager.h"
#include "epic_serial.h"
#include "core/epic_harness.h"
#include "epic_hal.h"        /* EPIC_IRQ_*, EPIC_USART_IsTxShiftRegisterEmpty */

#include <stdint.h>

#ifndef FOSC_HZ
#define FOSC_HZ 48000000UL
#endif

/** Loop-iteration bound, not a real time unit (see core/epic_harness.h).
 *  1500 iterations is a few hundred thousand instructions, which
 *  finishes well inside a 5000 ms mdb wait budget while accumulating
 *  ~100-200 ticks (see the tick-rate note above), far past the 80
 *  ticks the checks need. */
#define SIM_ITERATIONS 1500UL

/** Task periods in ticks: distinct rates so the three periodic tasks'
 *  counts prove repeated, independent firing. With the ~341 us tick,
 *  A = ~3.4 ms, B = ~6.8 ms, C = ~13.6 ms of simulated time. */
#define PERIOD_A 10U
#define PERIOD_B 20U
#define PERIOD_C 40U

/** Timer0 tick: 1:16 prescaler, reload 0 -> 256 counts x 16 = 4096
 *  instruction cycles per tick (~341 us at 48 MHz). */
#define TICK_RELOAD    0U
#define TICK_PRESCALER TIMER0_PRESCALER_1_16

/** The checks need two full C periods (80 ticks). */
#define TICKS_MIN 80U

/** Outer guard on the paced TX drain: one pop per in-flight byte; the
 *  ring holds at most 32 and a single scheduler round pushes at most
 *  9 bytes, so 64 is a generous ceiling that fails loudly instead of
 *  hanging. */
#define TX_POP_GUARD 64UL
/** Inner guard on the per-byte TRMT wait: a 9600-baud byte takes ~5000
 *  cycles to shift, each spin iteration a few instructions, so 100000
 *  is a comfortable ceiling. Same value as the console gate. */
#define TX_TRMT_GUARD 100000UL

/** Per-task payloads: distinctive ASCII so the captured UART stream
 *  can be counted byte-exactly (none of these substrings appear in the
 *  gate's own report text). 3 bytes each. */
static const uint8_t PAYLOAD_A[] = { '<', 'A', '>' };
static const uint8_t PAYLOAD_B[] = { '<', 'B', '>' };
static const uint8_t PAYLOAD_C[] = { '<', 'C', '>' };
#define PAYLOAD_LEN 3u

/* ───────────────────────── per-task state ─────────────────────────── */

/** Counter carried through a periodic task's `arg`, since locals don't
 *  survive between runs. */
typedef struct {
    volatile uint16_t runs;
} run_count_t;

static run_count_t arg_a = { 0U };
static run_count_t arg_b = { 0U };
static run_count_t arg_c = { 0U };

/** Bytes pushed into the TX ring by every task firing (3 per run). */
static volatile uint16_t g_pushed = 0u;
/** Bytes popped by the gate's own paced drain (GIE off: every pop). */
static uint16_t g_popped = 0u;
/** Set when a write returned short or a drain hit its guard. */
static uint8_t g_drain_failed = 0u;

static uint16_t g_fail = 0u;

static void fail(uint8_t idx)
{
    static const char hx[] = "0123456789ABCDEF";
    char c[2];
    g_fail++;
    epic_harness_log("F");
    c[0] = hx[(idx >> 4) & 0xF];
    c[1] = '\0';
    epic_harness_log(c);
    c[0] = hx[idx & 0xF];
    c[1] = '\0';
    epic_harness_log(c);
    epic_harness_log(".");
}

#define CHECK(cond, idx) do {         \
    if (!(cond)) fail(idx);            \
} while (0)

/* ───────────────────────── tasks ──────────────────────────────────── */

/** Periodic writer task: bump its run count and push its payload into
 *  the epic-serial TX ring (never blocks: one round pushes at most
 *  9 bytes into a 32-byte ring that the drain empties every round). */
static void task_writer_a(void *arg)
{
    run_count_t *r = (run_count_t *)arg;
    r->runs++;
    if (epic_serial_write(PAYLOAD_A, (int)PAYLOAD_LEN) != (int)PAYLOAD_LEN) {
        g_drain_failed = 1u;
    }
    g_pushed = (uint16_t)(g_pushed + PAYLOAD_LEN);
}

static void task_writer_b(void *arg)
{
    run_count_t *r = (run_count_t *)arg;
    r->runs++;
    if (epic_serial_write(PAYLOAD_B, (int)PAYLOAD_LEN) != (int)PAYLOAD_LEN) {
        g_drain_failed = 1u;
    }
    g_pushed = (uint16_t)(g_pushed + PAYLOAD_LEN);
}

static void task_writer_c(void *arg)
{
    run_count_t *r = (run_count_t *)arg;
    r->runs++;
    if (epic_serial_write(PAYLOAD_C, (int)PAYLOAD_LEN) != (int)PAYLOAD_LEN) {
        g_drain_failed = 1u;
    }
    g_pushed = (uint16_t)(g_pushed + PAYLOAD_LEN);
}

/* ───────────────────────── TX drain ───────────────────────────────── */

/** Paced drain of the epic-serial TX ring through the real TX ISR
 *  entry: wait for the shift register to empty (so the byte in TXREG
 *  has fully left), then pop one ring byte via the manual dispatch
 *  (which loads TXREG), and repeat. Same per-byte pacing the console
 *  gate's drain uses, so the uart1io capture receives every byte
 *  intact. Each dispatch also services any pending TMR0IF, so the
 *  scheduler tick advances in real simulated time here too. */
static void drain_tx(void)
{
    uint32_t outer = 0UL;
    while (epic_serial_tx_pending() > 0 && outer < TX_POP_GUARD) {
        uint32_t guard = 0UL;
        while (!EPIC_USART_IsTxShiftRegisterEmpty() &&
               guard < TX_TRMT_GUARD) {
            guard++;
        }
        if (guard >= TX_TRMT_GUARD) {
            g_drain_failed = 1u;
            return;
        }
        epic_dispatch_all_irqs();
        g_popped++;
        outer++;
    }
    if (epic_serial_tx_pending() > 0) {
        g_drain_failed = 1u;
    }
}

/** Log a 16-bit value as four hex digits (the sim harness prints
 *  fmt verbatim, no varargs). */
static void log_u16(uint16_t v)
{
    static const char hx[] = "0123456789ABCDEF";
    char c[5];
    c[0] = hx[(v >> 12) & 0xFu];
    c[1] = hx[(v >> 8) & 0xFu];
    c[2] = hx[(v >> 4) & 0xFu];
    c[3] = hx[v & 0xFu];
    c[4] = '\0';
    epic_harness_log(c);
}

/* ───────────────────────── main ───────────────────────────────────── */

int main(void)
{
    epic_harness_init(SIM_ITERATIONS);
    epic_serial_init(FOSC_HZ, 9600u);
    /* Deterministic start: GIE off for the whole run (see the file
     * header); the harness leaves it clear, make it explicit. */
    (void)EPIC_IRQ_Disable();

    task_manager_init();

    /* Three periodic writer tasks, priorities distinct so the run
     * order within one round is fixed: A, then B, then C. */
    task_spawn(task_writer_a, &arg_a, PERIOD_A, 0U);
    task_spawn(task_writer_b, &arg_b, PERIOD_B, 1U);
    task_spawn(task_writer_c, &arg_c, PERIOD_C, 2U);

    /* Wire the diagnostic Timer0 tick to the scheduler (starts the
     * timer; TMR0IF latches in real simulated time and is serviced by
     * the manual dispatch below, standing in for the ISR the same way
     * the serial gate's drain stands in for the TX ISR). */
    task_manager_attach_timer0(TICK_RELOAD, TICK_PRESCALER);

    /* The bounded scheduler loop. Each round: service any pending
     * TMR0IF (the tick), run every task due this round (their writes
     * arm TXIE), then drain the TX ring byte by byte. */
    for (uint32_t i = 0; epic_harness_running(i); i++) {
        epic_harness_tick();
        epic_dispatch_all_irqs();   /* tick: TMR0IF -> task_manager_tick */
        (void)task_manager_run_once();
        drain_tx();
    }

    /* Stop the timer and finish any last drain before the checks. */
    (void)EPIC_TIMER0_Stop();
    epic_dispatch_all_irqs();       /* clear any latched TMR0IF */
    drain_tx();

    /* Cross-checks. */
    uint16_t ticks = task_manager_ticks();
    uint8_t  used  = task_manager_count();
    uint16_t pushed = g_pushed;

    CHECK(arg_c.runs >= 2u, 0x00);              /* C fired repeatedly */
    CHECK((arg_a.runs >= 4u * arg_c.runs) &&
          (arg_a.runs <= 4u * arg_c.runs + 3u), 0x01); /* A:C period ratio */
    CHECK((arg_b.runs >= 2u * arg_c.runs) &&
          (arg_b.runs <= 2u * arg_c.runs + 1u), 0x02); /* B:C period ratio */
    CHECK(ticks >= TICKS_MIN, 0x03);            /* tick counter advanced */
    CHECK(used == 3u, 0x04);                    /* all tasks still registered */
    CHECK(g_popped == pushed, 0x05);            /* ring: every pushed byte popped */
    CHECK(epic_serial_tx_pending() == 0, 0x06); /* ring fully drained */
    CHECK(g_drain_failed == 0u, 0x07);          /* no short write / drain timeout */

    /* Log the counts so the capture can be cross-checked byte-exactly:
     * the payload stream in the captured UART output must contain
     * <A> runs_A times, <B> runs_B times, <C> runs_C times, and the
     * fixed 48-byte prefix "<A><A><B><A><A><B><C><A><A><B><A><A><B><C>"
     * from the first two full 40-tick cycles (see the file header). */
    epic_harness_log("C8 A=");
    log_u16(arg_a.runs);
    epic_harness_log(" B=");
    log_u16(arg_b.runs);
    epic_harness_log(" C=");
    log_u16(arg_c.runs);
    epic_harness_log(" T=");
    log_u16(ticks);
    epic_harness_log(" P=");
    log_u16(pushed);
    epic_harness_log(" O=");
    log_u16(g_popped);
    epic_harness_log("\n");

    return epic_harness_report(g_fail == 0u);
}
