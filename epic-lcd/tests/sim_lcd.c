/**
 * @file    sim_lcd.c
 * @brief   Bounded, self-reporting HARNESS=sim build for epic-lcd:
 *          the module's first real `mdb` gate. Runs the real compiled
 *          driver (src/epic_lcd.c) under MPLAB SIM on a 16F877A,
 *          driving the HD44780 init sequence plus a small print
 *          through the real ops interface, and verifies the exact
 *          command/data byte stream and delay pattern the driver
 *          emitted, reporting PASS/FAIL over the target's real
 *          hardware USART (see
 *          pic16f87xa-hal/src/core/pic16_harness_sim_target.c).
 *
 * @details
 *   Verification is by transport-call counting, the gate brief's
 *   fallback clause ("or by counting the transport's calls if a
 *   probeable transport exists"): the transport's ops are installed
 *   by the real epic_lcd_gpio4_init (so the real transport init code,
 *   pin configuration and initial E/RS writes run and are checked),
 *   then the send/delay ops are replaced by recorders that log the
 *   (rs, byte) / (kind, value) pairs the driver emits. The recorded
 *   streams are checked byte-for-byte against the driver's documented
 *   contract (init preamble, function/display/entry commands, DDRAM
 *   addressing, data bytes, and the 50 ms / 4.5 ms / 1.6 ms / 40 us
 *   timing constants), and TRISB is checked through EPIC_BANK1_READ8
 *   (all six LCD pins outputs, RB2/RB3 untouched) plus the E/RS
 *   low-after-init state via the literal-token path.
 *
 *   Finding (why the gate records the send calls instead of driving
 *   the real gpio4_send, MPLAB SIM + XC8 v4.00, 16F877A): the
 *   transport's send functions cannot be invoked through the ops
 *   function-pointer mechanism in this build. XC8's PIC16 indirect
 *   calls go through its PC-relative fptable and pass arguments in
 *   shared non-reentrant parameter storage, and gpio4_send's
 *   bank-1-local frame collides with that ABI: the compiler itself
 *   warns "call from non-reentrant function ... might corrupt
 *   parameters" (warning 1481) at the wrapper call site. Measured
 *   under mdb, calling gpio4_send through ops.send left PORTB
 *   untouched, left STATUS at 0xF8 (RP1:RP0 = 11, IRP set, i.e. the
 *   machine parked in Bank 3, which then hangs the harness's polled
 *   USART report), and a driver + real-send probe reset the part
 *   before reaching report. The same calls made DIRECTLY work
 *   perfectly: a probe calling EPIC_GPIO_WritePin directly (the
 *   transport's own building block) produced the exact expected
 *   PORTB state (0x30), clean STATUS, and a PASS report. The
 *   transport's nibble-splitting send code is therefore not
 *   verifiable end to end under this toolchain; the gate verifies
 *   the driver's full emission contract instead and leaves the
 *   transport's pin data path to real hardware.
 *
 *   Second finding (why the delays are recorded rather than spun,
 *   same toolchain): the transport's real delay functions busy-wait
 *   on the epic-tick Timer2 ISR, and that ISR (vector + dispatch +
 *   TIMER2_IRQHandler + callback, 4 hardware stack levels) firing
 *   while the driver's send path is 6-7 levels deep overflows the
 *   PIC16F877A's 8-level hardware call stack (observed: the run
 *   completed and printed its marker, then main's return address,
 *   the oldest clobbered stack slot, took the part to a reset and
 *   re-ran the whole build repeatedly in the 5000 ms mdb wait).
 *   Real-target firmware driving this transport with epic_tick
 *   running concurrently should expect the same constraint.
 *
 *   LCD pins are all on PORTB (RB0=RS, RB1=E, RB4..RB7=DB4..DB7),
 *   deliberately clear of the harness's USART pins (RC6/RC7). MPLAB
 *   SIM cannot inject RX input (same constraint documented in
 *   epic-swuart's sim build), so the gate is TX-side only.
 */

#include "epic_lcd.h"
#include "epic_lcd_transport.h"
#include "epic_hal.h"
#include "core/epic_harness.h"

#include <stdint.h>

#ifndef FOSC_HZ
#define FOSC_HZ 20000000UL
#endif

/* LCD pins, all on PORTB (16F877A): RS=RB0, E=RB1, DB4..DB7=RB4..RB7. */
static const epic_lcd_gpio4_pins_t LCD_PINS = {
    .rs_port = GPIOB,  .rs_pin  = GPIO_PIN_0,
    .e_port  = GPIOB,  .e_pin   = GPIO_PIN_1,
    .db4_port = GPIOB, .db4_pin = GPIO_PIN_4,
    .db5_port = GPIOB, .db5_pin = GPIO_PIN_5,
    .db6_port = GPIOB, .db6_pin = GPIO_PIN_6,
    .db7_port = GPIOB, .db7_pin = GPIO_PIN_7,
};

/* Expected (rs, byte) stream, in send order. The first seven are the
 * epic_lcd_init sequence for a 16x2 display with the default row-address
 * table; the rest are the API calls main() makes. */
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

static void recorder_delay_us(void *ctx, uint32_t us)
{
    (void)ctx;
    if (g_dlog_len < DELAY_CAP) {
        g_dlog[g_dlog_len].kind  = 0u;
        g_dlog[g_dlog_len].value = (uint16_t)us;
        g_dlog_len++;
    }
}

static void recorder_delay_ms(void *ctx, uint32_t ms)
{
    (void)ctx;
    if (g_dlog_len < DELAY_CAP) {
        g_dlog[g_dlog_len].kind  = 1u;
        g_dlog[g_dlog_len].value = (uint16_t)ms;
        g_dlog_len++;
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
    uint8_t i;

    epic_harness_init(0UL);

    epic_lcd_ops_t ops;
    void *ops_ctx;
    epic_lcd_gpio4_init(&ops, &ops_ctx, &LCD_PINS);

    /* 0x06: the real transport init left E and RS low on the pins
     * (its own EPIC_GPIO_WritePin calls; DB4..DB7 untouched). */
    CHECK((portb_latch_read() & 0x03u) == 0x00u, 0x06);

    /* Replace send/delay ops with recorders (see file comment). */
    ops.send     = recorder_send;
    ops.delay_us = recorder_delay_us;
    ops.delay_ms = recorder_delay_ms;

    epic_lcd_t lcd;
    epic_lcd_config_t cfg = { .cols = 16u, .rows = 2u, .row_addr = {0u} };
    epic_lcd_init(&lcd, &ops, ops_ctx, &cfg);

    epic_lcd_set_cursor(&lcd, 0u, 0u);
    epic_lcd_print(&lcd, "Hi!");
    epic_lcd_set_cursor(&lcd, 0u, 1u);
    epic_lcd_write_char(&lcd, 'A');
    epic_lcd_scroll_left(&lcd);
    epic_lcd_display_on(&lcd, false);
    epic_lcd_display_on(&lcd, true);

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
        epic_harness_log("lcd sim: TRIS/sequence/delay checks all ok\n");
    }

    for (uint32_t iter = 0; epic_harness_running(iter); iter++) {
        epic_harness_tick();
    }
    return epic_harness_report(g_fail == 0u);
}
