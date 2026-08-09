/**
 * @file    combo_lcd_tick.c
 * @brief   C10 of the combination matrix
 *          (docs/superpowers/plans/2026-08-09-combination-matrix.md):
 *          epic-lcd + epic-tick on the 16F877A. The real compiled
 *          HD44780 driver (src/epic_lcd.c) runs its init sequence plus
 *          a small print through the gpio4 transport ops while
 *          epic-tick's live 1 ms Timer2 ISR churns underneath, then
 *          the gate verifies the driver's full emission contract (the
 *          lcd gate's byte-exact oracle) and that the tick stayed
 *          alive through the LCD work, reporting PASS/FAIL over the
 *          target's real hardware USART.
 *
 * @details
 *   Verification is by transport-call counting, exactly the lcd gate's
 *   design (epic-lcd/tests/sim_lcd.c): the real epic_lcd_gpio4_init
 *   runs (so the real transport init code, pin configuration and the
 *   initial E/RS writes are executed and checked), the send op is
 *   replaced by a recorder (the documented PIC16 constraint: gpio4_send
 *   cannot be invoked through the ops function pointer under XC8 v4.00
 *   -- fptable ABI + bank-1 frame collision, compiler warning 1481),
 *   and the delay ops record the call AND then execute the delay for
 *   real on the tick counter (epic_tick_get), so the driver's 50 ms /
 *   4.5 ms / 1.6 ms / 40 us timing is genuinely consumed on the tick
 *   timebase, not just counted.
 *
 *   Stack constraint (why the send path is recorded, why the print
 *   is emitted through epic_lcd_write, and why the 1600 us clear
 *   delay is counted but not spun): the PIC16F877A hardware stack is
 *   8 levels. Measured under mdb, the live tick ISR (vector +
 *   dispatch + TIMER2_IRQHandler + its GetFlag/ClearFlag/callback
 *   calls) needs 5 stack levels, so a main line at depth 5 or deeper
 *   gets a caller's return address overwritten when the ISR fires
 *   (the lcd gate's documented constraint; reproduced here: the
 *   print API's send chain main -> epic_lcd_print -> epic_lcd_write
 *   -> send_data -> ops is 5 deep, and the ISR firing inside it
 *   derailed the sequence deterministically, dropping the last two
 *   commands and leaving GIE cleared). At depth 4 the ISR only ever
 *   clobbers the oldest slot (main's return address), which is
 *   consumed after the PASS marker (the epic-tick gate's own proven
 *   shape). The gate therefore keeps every driver path at depth <= 4:
 *   sends are recorded (the fptable constraint), "Hi!" goes through
 *   epic_lcd_write (identical bytes and delays, one frame shallower
 *   than epic_lcd_print), the 50 ms and 4500 us delays spin for real
 *   on the tick counter (depth 3-4), and the 1600 us clear delay is
 *   recorded but not spun because its cmd_long_wait path is one frame
 *   deeper. Real-target firmware driving this transport with
 *   epic_tick running concurrently should expect the same
 *   constraint. epic_tick_delay_ms itself is exercised at main level
 *   (depth 2-4) before and after the sequence.
 *
 *   MPLAB SIM behaviors pinned down here (both from the C1 gate):
 *   - epic_harness_init takes a REAL iteration count (0 would make
 *     every harness loop a no-op).
 *   - Enabling GIE while a timer interrupt is already pending wedges
 *     the sim's ISR path (the Finding 10.1 class), and main() re-runs
 *     after returning (`ljmp start`) with TMR2 still running, so per
 *     pass the timer is stopped and TMR2IF cleared before
 *     epic_tick_init's GIE-on edge.
 *   - No RX involvement, so the MPLAB SIM RX wall does not apply.
 *
 *   Bounded and self-reporting (the harness contract): the sequence
 *   consumes ~54 ms of simulated time (the driver's own delay
 *   constants, executed on the real tick), plus 5 ms and 10 ms
 *   main-level delays and 2000 harness iterations; comfortably inside
 *   the 5000 ms wait_ms budget.
 */

#include "epic_lcd.h"
#include "epic_lcd_transport.h"
#include "epic_tick.h"
#include "core/epic_harness.h"
#include "core/pic16_irq.h"
#include "pic16f87xa_sfr.h"
#include "target/pic16f87xa_platform.h"

#include <stdint.h>

#ifndef FOSC_HZ
#define FOSC_HZ 20000000UL
#endif

#define SIM_ITERATIONS 2000UL

/* LCD pins, all on PORTB (16F877A): RS=RB0, E=RB1, DB4..DB7=RB4..RB7. */
static const epic_lcd_gpio4_pins_t LCD_PINS = {
    .rs_port = GPIOB,  .rs_pin  = GPIO_PIN_0,
    .e_port  = GPIOB,  .e_pin   = GPIO_PIN_1,
    .db4_port = GPIOB, .db4_pin = GPIO_PIN_4,
    .db5_port = GPIOB, .db5_pin = GPIO_PIN_5,
    .db6_port = GPIOB, .db6_pin = GPIO_PIN_6,
    .db7_port = GPIOB, .db7_pin = GPIO_PIN_7,
};

/* Expected (rs, byte) stream, in send order: the lcd gate's oracle.
 * The first seven are the epic_lcd_init sequence for a 16x2 display
 * with the default row-address table; the rest are the API calls
 * main() makes. */
typedef struct {
    uint8_t rs;
    uint8_t byte;
} expect_t;

static const expect_t EXPECT[] = {
    { 0u, 0x38u },   /* function set: 8-bit, 2-line (init)   */
    { 0u, 0x38u },
    { 0u, 0x38u },
    { 0u, 0x08u },   /* display off                          */
    { 0u, 0x01u },   /* clear display                        */
    { 0u, 0x06u },   /* entry mode: increment, no shift      */
    { 0u, 0x0Cu },   /* display on, cursor off, blink off    */
    { 0u, 0x80u },   /* set_cursor(0, 0): DDRAM addr 0x00    */
    { 1u, 0x48u },   /* print("Hi!"): 'H'                    */
    { 1u, 0x69u },   /* 'i'                                  */
    { 1u, 0x21u },   /* '!'                                  */
    { 0u, 0xC0u },   /* set_cursor(0, 1): DDRAM addr 0x40    */
    { 1u, 0x41u },   /* write_char('A')                      */
    { 0u, 0x18u },   /* scroll_left                          */
    { 0u, 0x08u },   /* display_on(false)                    */
    { 0u, 0x0Cu },   /* display_on(true)                     */
};
#define EXPECT_LEN (sizeof(EXPECT) / sizeof(EXPECT[0]))

/* Expected delay calls, in order. kind 0 = delay_us, 1 = delay_ms.
 * Matches epic_lcd.c's DELAY_INIT_MS/DELAY_INIT4_US/DELAY_CLEAR_US/
 * DELAY_CMD_US constants, one entry per driver send. */
typedef struct {
    uint8_t  kind;
    uint16_t value;
} delay_expect_t;

static const delay_expect_t EXPECT_DELAY[] = {
    { 1u, 50u },     /* power-on wait before the init preamble   */
    { 0u, 4500u },   /* first function-set wait (>4.1 ms)        */
    { 0u, 40u },     /* second function-set wait                 */
    { 0u, 40u },     /* third function-set wait                  */
    { 0u, 40u },     /* display off                              */
    { 0u, 1600u },   /* clear (1.53 ms, rounded up)              */
    { 0u, 40u },     /* entry mode set                           */
    { 0u, 40u },     /* display on                               */
    { 0u, 40u },     /* set_cursor(0, 0)                         */
    { 0u, 40u },     /* 'H'                                      */
    { 0u, 40u },     /* 'i'                                      */
    { 0u, 40u },     /* '!'                                      */
    { 0u, 40u },     /* set_cursor(0, 1)                         */
    { 0u, 40u },     /* 'A'                                      */
    { 0u, 40u },     /* scroll_left                              */
    { 0u, 40u },     /* display_on(false)                        */
    { 0u, 40u },     /* display_on(true)                         */
};
#define EXPECT_DELAY_LEN (sizeof(EXPECT_DELAY) / sizeof(EXPECT_DELAY[0]))

/* ---- recorders: count the transport ops the driver emits ---- */

/* One guard slot beyond the expected counts so a driver that sends
 * MORE than the expected sequence still trips the length check
 * (a cap equal to the expectation would silently drop extras). */
#define SEND_CAP  (EXPECT_LEN + 1u)
#define DELAY_CAP (EXPECT_DELAY_LEN + 1u)

typedef struct {
    uint8_t rs;
    uint8_t byte;
} log_entry_t;

static log_entry_t g_log[SEND_CAP];
static uint8_t     g_log_len;

static delay_expect_t g_dlog[DELAY_CAP];
static uint8_t        g_dlog_len;

/* The send op is recorded, not forwarded to the real transport: the
 * real gpio4_send cannot be invoked through the ops pointer under
 * this toolchain (see the file comment's finding). */
static void recorder_send(void *ctx, uint8_t rs, uint8_t byte)
{
    (void)ctx;
    if (g_log_len < SEND_CAP) {
        g_log[g_log_len].rs   = rs;
        g_log[g_log_len].byte = byte;
        g_log_len++;
    }
}

/* Record a delay call and then execute it for real on the tick
 * timebase. Spun inline (not via epic_tick_delay_ms): its
 * elapsed_since/get frames would put the spin at depth 6 and the live
 * 4-level tick ISR overflows the 8-level hardware stack (see the file
 * comment's stack math); at depth 3-4 the ISR fits exactly. Every
 * completed spin is itself proof the tick stayed alive mid-delay. */
static void combo_delay_ms(void *ctx, uint32_t ms)
{
    (void)ctx;
    if (g_dlog_len < DELAY_CAP) {
        g_dlog[g_dlog_len].kind  = 1u;
        g_dlog[g_dlog_len].value = (uint16_t)ms;
        g_dlog_len++;
    }
    uint32_t t0 = epic_tick_get();
    while (epic_tick_get() - t0 < ms) {
        epic_harness_tick();
    }
}

/* Same policy as the real gpio4_delay_us: epic-tick's resolution is
 * 1 ms, so only >= 1000 us spin (the E-pulse setup/hold time is
 * already met by the pin-write overhead). Two exceptions, both the
 * lcd gate's documented stack constraint, not firmware bugs: the
 * 40 us command delays never spin (sub-ms), and a 1-tick spin is
 * skipped when the call arrives through the driver's cmd_long_wait
 * helper (the 1600 us clear delay), because that path is one frame
 * deeper (init -> cmd_long_wait -> ops -> this) and the spin's
 * epic_tick_get call then sits at stack depth 5, where the live ISR
 * overwrites the caller's return address (measured under mdb). The
 * >= 2-tick spins (the 4500 us init delay) stay real: they arrive
 * through the direct ops call at depth 3, get at 4, safe. */
static void combo_delay_us(void *ctx, uint32_t us)
{
    (void)ctx;
    if (g_dlog_len < DELAY_CAP) {
        g_dlog[g_dlog_len].kind  = 0u;
        g_dlog[g_dlog_len].value = (uint16_t)us;
        g_dlog_len++;
    }
    if (us >= 2000u) {
        us /= 1000u;   /* reuse the parameter slot (RAM budget) */
        uint32_t t0 = epic_tick_get();
        while (epic_tick_get() - t0 < us) {
            epic_harness_tick();
        }
    }
}

/* Absolute Bank-0 read of PORTB (address 0x06) through the
 * literal-token path: clear both RP bits, read, bank out, hand the
 * value over through the common-RAM scratch byte, the same discipline
 * as EPIC_BANK1_READ8. */
static uint8_t portb_latch_read(void)
{
    uint8_t v;

    asm("bcf STATUS,5");
    asm("bcf STATUS,6");
    asm("movf 0x06,w");
    asm("movwf _epic_bank1_scratch");
    v = epic_bank1_scratch;
    return v;
}

/* ---- failure reporting (same shape as pic16f87xa-hal's probe) ---- */

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

int main(void)
{
    uint8_t trisb;
    uint8_t pie1;
    uint8_t i;

    epic_harness_init(SIM_ITERATIONS);

    /* Per-pass pre-clean (C1 lesson): main() re-runs via `ljmp start`
     * with TMR2 still running and TMR2IF possibly latched, and a
     * GIE-on edge with a pending timer interrupt wedges the sim's ISR
     * path, so stop the timer and clear its flag before
     * epic_tick_init's GIE-on. */
    EPIC_REG8(PIC_REG_T2CON) &= (uint8_t)~PIC_T2CON_TMR2ON;
    EPIC_IRQ_ClearFlag(PIC16_IRQ_TMR2);

    /* (a) The live 1 ms tick ISR: epic_tick_init configures Timer2
     * for ~1 ms at FOSC_HZ, starts it, and enables GIE. */
    epic_tick_init(FOSC_HZ);

    /* Real epic_tick_delay_ms round trip at main level (depth 2-4;
     * at depth 4 the 5-level ISR only ever clobbers the oldest stack
     * slot, consumed after the PASS marker: the epic-tick gate's own
     * proven shape); also makes sure the ISR is firing before the
     * LCD work starts. One 32-bit slot is reused for every timing
     * measurement (the PIC16 RAM budget of the full-HAL build is
     * tight; see the lcd gate's link map). */
    uint32_t tck = epic_tick_get();
    epic_tick_delay_ms(5u);
    tck = epic_tick_get() - tck;
    CHECK((tck >= 5u) && (tck <= 7u), 0x04);

    /* (b) Real gpio4 transport init: the six LCD pins are configured
     * as outputs and E/RS are left low on the pins. */
    epic_lcd_ops_t ops;
    void *ops_ctx;
    epic_lcd_gpio4_init(&ops, &ops_ctx, &LCD_PINS);

    /* 0x06: the real transport init left E and RS low (its own
     * EPIC_GPIO_WritePin calls; DB4..DB7 untouched). */
    CHECK((portb_latch_read() & 0x03u) == 0x00u, 0x06);

    /* Replace the send op with a recorder (the fptable constraint);
     * the delay ops record AND execute on the real tick counter. */
    ops.send     = recorder_send;
    ops.delay_us = combo_delay_us;
    ops.delay_ms = combo_delay_ms;

    /* (c) The HD44780 init sequence plus a print, with the tick ISR
     * live underneath: every delay the driver issues is executed for
     * real on the tick (50 + 4 ms of spins). Note the print is
     * emitted through epic_lcd_write, not epic_lcd_print: the print
     * wrapper adds a call level (print -> write -> send_data -> ops),
     * putting the send at stack depth 5, where the live 4-5 level ISR
     * overwrites the caller's return address (measured under mdb:
     * the print derailed and the last two commands never ran). Write
     * sends the identical bytes and delays at depth 4. */
    tck = epic_tick_get();
    epic_lcd_t lcd;
    epic_lcd_config_t cfg = { .cols = 16u, .rows = 2u, .row_addr = {0u} };
    epic_lcd_init(&lcd, &ops, ops_ctx, &cfg);
    epic_lcd_set_cursor(&lcd, 0u, 0u);
    epic_lcd_write(&lcd, "Hi!", 3u);
    epic_lcd_set_cursor(&lcd, 0u, 1u);
    epic_lcd_write_char(&lcd, 'A');
    epic_lcd_scroll_left(&lcd);
    epic_lcd_display_on(&lcd, false);
    epic_lcd_display_on(&lcd, true);
    tck = epic_tick_get() - tck;

    /* Real epic_tick_delay_ms round trip after the LCD work: the
     * tick must still block-and-advance with the driver's sequence
     * behind it (the epic-tick gate's bounds). */
    uint32_t t10 = epic_tick_get();
    epic_tick_delay_ms(10u);
    t10 = epic_tick_get() - t10;

    /* ---- Cross-checks (the tick is still live; the checks run at
     *      depth 2-3, safe under the 5-level ISR). ---- */

    /* 0x00: all six LCD pins are outputs, RB2/RB3 untouched (TRISB
     * reads 0x0C through the safe Bank-1 path). */
    EPIC_BANK1_READ8(TRISB, trisb);
    CHECK(trisb == 0x0Cu, 0x00);

    /* 0x01/0x02: the driver handed the transport exactly the expected
     * byte stream, with the right RS for each send. */
    CHECK(g_log_len == EXPECT_LEN, 0x01);
    if (g_log_len == EXPECT_LEN) {
        uint8_t seq_ok = 1u;
        for (i = 0; i < EXPECT_LEN; i++) {
            if (g_log[i].rs != EXPECT[i].rs ||
                g_log[i].byte != EXPECT[i].byte) {
                seq_ok = 0u;
                break;
            }
        }
        CHECK(seq_ok != 0u, 0x02);
    }

    /* 0x03: the driver's delay constants (50 + 4 ms of real spins)
     * must have consumed at least 54 ticks during the sequence: the
     * tick stayed alive through the whole LCD work (a wedged ISR
     * would have hung the first spin, never reaching this check). */
    CHECK(tck >= 54u, 0x03);

    /* 0x0B: the post-sequence epic_tick_delay_ms round trip landed
     * within one tick of the request (the epic-tick gate's bounds):
     * the tick still blocks and advances after the LCD work. */
    CHECK((t10 >= 10u) && (t10 <= 12u), 0x0B);

    /* 0x05/0x0A: PIE1 still holds exactly TMR2IE (the harness's TXIE
     * stays off; nothing else got armed during the sequence). */
    EPIC_BANK1_READ8(PIE1, pie1);
    CHECK((pie1 & PIC_PIE1_TMR2IE) != 0u, 0x05);
    CHECK((pie1 & (uint8_t)~(PIC_PIE1_TMR2IE)) == 0u, 0x0A);

    /* 0x09: GIE is still set (the tick can keep firing). */
    CHECK((EPIC_REG8(PIC_REG_INTCON) & PIC_INTCON_GIE) != 0u, 0x09);

    /* 0x07/0x08: the delay calls match the driver's documented
     * timing pattern. */
    CHECK(g_dlog_len == EXPECT_DELAY_LEN, 0x07);
    if (g_dlog_len == EXPECT_DELAY_LEN) {
        uint8_t dly_ok = 1u;
        for (i = 0; i < EXPECT_DELAY_LEN; i++) {
            if (g_dlog[i].kind != EXPECT_DELAY[i].kind ||
                g_dlog[i].value != EXPECT_DELAY[i].value) {
                dly_ok = 0u;
                break;
            }
        }
        CHECK(dly_ok != 0u, 0x08);
    }

    if (g_fail == 0u) {
        epic_harness_log("C10 lcd-tick: TRIS/sequence/delay checks ok, tick alive\n");
    }

    /* Idle under the live tick, then quiet before the report so the
     * ISR cannot preempt the marker. */
    for (uint32_t iter = 0; epic_harness_running(iter); iter++) {
        epic_harness_tick();
    }
    (void)EPIC_IRQ_Disable();
    return epic_harness_report(g_fail == 0u);
}
