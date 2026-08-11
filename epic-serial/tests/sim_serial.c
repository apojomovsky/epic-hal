/*
 * Bounded, self-reporting HARNESS=sim build for epic-serial: the module's
 * first real mdb gate, running the actual epic_serial.c under MPLAB SIM
 * on a 16F877A and reporting PASS/FAIL over the real hardware USART.
 *
 * MPLAB SIM's USART model keeps TXIF SET while TXREG holds a byte, so
 * TXIE with GIE set fires the TX ISR continuously and the in-ISR
 * Disable/Restore churn trips the simulator's interrupt-delivery quirk
 * (documented in epic_tick.c), wedging GIE. The TX phases therefore run
 * with GIE OFF (manual dispatch), and the live-ISR phase only after TXIE
 * is off. MPLAB SIM cannot inject RX input, so the RX side is the
 * empty-ring contract only.
 */

#include "epic_serial.h"
#include "core/epic_harness.h"
#include "epic_hal.h"

#ifndef FOSC_HZ
#define FOSC_HZ 20000000UL
#endif

/* Per-phase loop-iteration bounds (see core/epic_harness.h). */
#define SIM_ITERATIONS  30000UL
#define PHASE2_ITERS    2000UL
#define PHASE3_ITERS    20000UL
#define PHASE4_ITERS    5000UL

/* Timer2 tick, ~819 us period at 20 MHz ((PR2+1) x prescaler, the same
 * ~1 ms class epic-tick's compute_period picks). Deliberately long: a
 * shorter period lets the tick ISR consume all of the simulator's time,
 * starving main and freezing the run. */
#define TICK_PR2 ((uint8_t)255)
/* Tick units (819 us each): the live-ISR pump must see at least this. */
#define TICK_MIN_ALIVE 4UL

#define TX_SEQ_LEN 5u

static volatile uint32_t g_tick_us = 0u;

static void s_tick_inc(void)
{
    g_tick_us++;
}

static uint32_t tick_now(void)
{
    /* 32-bit read the ISR can tear mid-update on the 8-bit core (same
     * pattern as epic_tick_get): retry until two reads agree. */
    uint32_t a, b;
    do {
        a = g_tick_us;
        b = g_tick_us;
    } while (a != b);
    return a;
}

static int g_fails = 0;

static void check(int cond, const char *what)
{
    epic_harness_log(what);
    epic_harness_log(cond ? " ok\n" : " FAIL\n");
    if (!cond) g_fails++;
}

/* Raw-string hex logging (the sim harness prints fmt verbatim, no
 * varargs): logs " T=xx R=xx" for the TXIF flag and TXREG readback. */
static void log_hex_pair(uint8_t tag, uint8_t v)
{
    static const char hx[] = "0123456789ABCDEF";
    char s[5];
    epic_harness_log(" ");
    s[0] = (char)tag;
    s[1] = '=';
    s[2] = hx[(v >> 4) & 0xF];
    s[3] = hx[v & 0xF];
    s[4] = '\0';
    epic_harness_log(s);
}

int main(void)
{
    epic_harness_init(SIM_ITERATIONS);
    epic_serial_init(FOSC_HZ, 9600u);
    /* Deterministic start: nothing may vector during phases 1-3 (the
     * harness leaves GIE clear; make it explicit). */
    (void)EPIC_IRQ_Disable();

    /* Phase 1: RX-empty contract (read's Disable/Restore). */
    uint8_t rbuf[8] = { 0 };
    check(epic_serial_available() == 0, "rx avail empty");
    check(epic_serial_read(rbuf, (int)sizeof(rbuf)) == 0, "rx read empty");

    /* Phase 2: manual-dispatch TX drain, GIE off. */
    static const uint8_t seq[TX_SEQ_LEN] = { 'A', 'B', 'C', 'D', 'E' };
    int w2 = epic_serial_write(seq, (int)TX_SEQ_LEN);
    int pops = 0;
    for (uint32_t i = 0; epic_harness_running(i) && i < PHASE2_ITERS &&
                        epic_serial_tx_pending() > 0; i++) {
        epic_harness_tick();
        int p = epic_serial_tx_pending();
        epic_dispatch_all_irqs();
        if (epic_serial_tx_pending() < p) {
            pops++;
            if (pops == 1) {
                /* Probe the sim's USART model: TXIF/TXREG readback after
                 * a TXREG write. */
                epic_harness_log("pop");
                log_hex_pair('T',
                    (uint8_t)EPIC_IRQ_GetFlag(PIC16_IRQ_USART_TX));
                log_hex_pair('R', (uint8_t)EPIC_REG8(PIC_REG_TXREG));
                epic_harness_log("\n");
            }
        }
    }
    check(w2 == (int)TX_SEQ_LEN, "tx write all");
    check(epic_serial_tx_pending() == 0, "tx drain all");

    /* Phase 3: blocking write + drain + flush, GIE off. */
    uint8_t fill[EPIC_SERIAL_RING_SZ + 2u];
    for (int j = 0; j < (int)sizeof(fill); j++) {
        fill[j] = (uint8_t)j;
    }
    int w3 = epic_serial_write(fill, (int)sizeof(fill));   /* blocks at ring-full */
    check(w3 == (int)sizeof(fill), "tx block all");
    for (uint32_t i = 0; epic_harness_running(i) && i < PHASE3_ITERS &&
                        epic_serial_tx_pending() > 0; i++) {
        epic_harness_tick();
        epic_dispatch_all_irqs();
    }
    int drained = (epic_serial_tx_pending() == 0);
    epic_serial_flush();                 /* waits for the last shift-register byte */
    check(drained, "tx drain full");
    /* Belt and braces: the flush dispatch already cleared TXIE via the
     * callback's empty-ring branch; make it explicit so phase 4 cannot
     * start a TX interrupt storm under the live tick. */
    EPIC_IRQ_DisableSrc(PIC16_IRQ_USART_TX);

    /* Phase 4: live tick ISR with the TXIE gate armed. */
    static TIMER2_HandleTypeDef t2 = TIMER2_HANDLE_DEFAULT;
    t2.Prescaler = TIMER2_PRESCALER_1_16;
    t2.Postscaler = TIMER2_POSTSCALER_1_1;
    t2.Period = TICK_PR2;
    t2.OverflowCallback = s_tick_inc;
    (void)EPIC_TIMER2_Init(&t2);
    (void)EPIC_TIMER2_Start(&t2);
    EPIC_IRQ_Restore(1);                 /* GIE on: the epic-tick role */

    uint32_t t0 = tick_now();
    for (uint32_t i = 0; epic_harness_running(i) && i < PHASE4_ITERS &&
                        (tick_now() - t0) < TICK_MIN_ALIVE; i++) {
        epic_harness_tick();
        epic_dispatch_all_irqs();        /* TXIF pending, TXIE off: gate skips */
    }
    check((tick_now() - t0) >= TICK_MIN_ALIVE, "tick alive after churn");
    /* Read-side Disable/Restore under the live preempting ISR. */
    check(epic_serial_read(rbuf, (int)sizeof(rbuf)) == 0, "rx read live");
    check(epic_serial_available() == 0, "rx still empty");

    epic_harness_log("serial sim done\n");
    return epic_harness_report(g_fails == 0);
}
