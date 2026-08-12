#include <xc.h>
#include <stdint.h>
#include <stdbool.h>
#include "epic_math.h"

/* Cycle benchmark via TMR1 (counts instruction cycles on the 87XA/18Fxx5x).
 * Each bench_* runs N iterations with TMR1 snapshotted before/after and
 * prints "<name> <delta>" over UART; per-op cycles = delta / N. */
#if defined(PIC18F4550)
#pragma config FOSC = HS, WDT = OFF, PWRT = ON, BOR = OFF, LVP = OFF, MCLRE = OFF, XINST = OFF, DEBUG = OFF
#define SPBRG_VAL 34u   /* 20 MHz, BRGH=1, 16-bit BRG: 20e6/(16*9600)-1 = 129; hi byte 0 */
#else
#pragma config FOSC = HS, WDTE = OFF, PWRTE = ON, BOREN = ON, LVP = OFF, WRT = OFF
#define SPBRG_VAL 129u
#endif
#define N 100u

static void uart_putc(char c) { while (!PIR1bits.TXIF) { } TXREG = c; }
static void uart_puts(const char *s) { while (*s) uart_putc(*s++); }
static void uart_puthex(uint16_t v) {
    static const char hex[] = "0123456789ABCDEF";
    uart_putc(hex[(v >> 12) & 0xF]); uart_putc(hex[(v >> 8) & 0xF]);
    uart_putc(hex[(v >> 4) & 0xF]);  uart_putc(hex[v & 0xF]);
}
#if defined(PIC18F4550)
static void uart_init(void) { SPBRGH = 0u; SPBRG = SPBRG_VAL; TXSTA = 0x24; RCSTA = 0x80; }
static uint16_t tmr1(void) { return (uint16_t)((uint16_t)TMR1H << 8 | TMR1L); }
#define TMR1_CONF 0x81u  /* RD16, TMR1ON */
#else
static void uart_init(void) { SPBRG = SPBRG_VAL; TXSTA = 0x24; RCSTA = 0x80; }
static uint16_t tmr1(void) { return (uint16_t)((uint16_t)TMR1H << 8 | TMR1L); }
#define TMR1_CONF 0x01u
#endif
static void report(const char *name, uint16_t t0) {
    uint16_t dt = (uint16_t)(tmr1() - t0);
    uart_puts(name); uart_putc(' '); uart_puthex(dt); uart_puts("\r\n");
}

volatile uint16_t g_seed = 0x1357u;
volatile uint16_t g_sum  = 0u;

void bench_loop_empty(void) {
    uint16_t a = g_seed; uint16_t t0 = tmr1();
    for (uint16_t i = 0u; i < N; i++) { g_sum = (uint16_t)(g_sum + a); a = (uint16_t)(a + 3u); }
    report("loop_empty", t0);
}
void bench_add_native(void) {
    uint16_t a = g_seed, b = 0xFFFFu; uint16_t t0 = tmr1();
    for (uint16_t i = 0u; i < N; i++) { g_sum = (uint16_t)(g_sum + (a + b)); a = (uint16_t)(a + 3u); }
    report("add_native", t0);
}
void bench_sub_native(void) {
    uint16_t a = g_seed, b = 0x1357u; uint16_t t0 = tmr1();
    for (uint16_t i = 0u; i < N; i++) { g_sum = (uint16_t)(g_sum + (a - b)); a = (uint16_t)(a + 3u); }
    report("sub_native", t0);
}
void bench_mul8_native(void) {
    uint8_t a = (uint8_t)g_seed, b = 0xCDu; uint16_t t0 = tmr1();
    for (uint16_t i = 0u; i < N; i++) { g_sum = (uint16_t)(g_sum + (uint16_t)(a * b)); a = (uint8_t)(a + 3u); }
    report("mul8_native", t0);
}
void bench_mul16_native(void) {
    uint16_t a = g_seed, b = 0xCDEFu; uint16_t t0 = tmr1();
    for (uint16_t i = 0u; i < N; i++) { g_sum = (uint16_t)(g_sum + (uint16_t)(a * b)); a = (uint16_t)(a + 3u); }
    report("mul16_native", t0);
}
void bench_div16_native(void) {
    uint16_t a = g_seed, b = 0x0013u; uint16_t t0 = tmr1();
    for (uint16_t i = 0u; i < N; i++) { g_sum = (uint16_t)(g_sum + (a / b)); a = (uint16_t)(a + 3u); }
    report("div16_native", t0);
}
#ifndef BENCH_NATIVE_ONLY
void bench_add_epic(void) {
    uint16_t a = g_seed, b = 0xFFFFu; bool c; uint16_t t0 = tmr1();
    for (uint16_t i = 0u; i < N; i++) { g_sum = (uint16_t)(g_sum + epic_math_add_u16(a, b, &c)); a = (uint16_t)(a + 3u); }
    report("add_epic", t0);
}
void bench_sub_epic(void) {
    uint16_t a = g_seed, b = 0x1357u; bool c; uint16_t t0 = tmr1();
    for (uint16_t i = 0u; i < N; i++) { g_sum = (uint16_t)(g_sum + epic_math_sub_u16(a, b, &c)); a = (uint16_t)(a + 3u); }
    report("sub_epic", t0);
}
void bench_mul8_epic(void) {
    uint8_t a = (uint8_t)g_seed, b = 0xCDu; uint16_t t0 = tmr1();
    for (uint16_t i = 0u; i < N; i++) { g_sum = (uint16_t)(g_sum + epic_math_mul_u8(a, b)); a = (uint8_t)(a + 3u); }
    report("mul8_epic", t0);
}
void bench_mul16_epic(void) {
    uint16_t a = g_seed, b = 0xCDEFu; uint16_t t0 = tmr1();
    for (uint16_t i = 0u; i < N; i++) { g_sum = (uint16_t)(g_sum + (uint16_t)epic_math_mul_u16(a, b)); a = (uint16_t)(a + 3u); }
    report("mul16_epic", t0);
}
void bench_div16_epic(void) {
    uint16_t a = g_seed, b = 0x0013u; bool ok; uint16_t t0 = tmr1();
    for (uint16_t i = 0u; i < N; i++) { epic_math_udiv16_t r = epic_math_divmod_u16(a, b, &ok); g_sum = (uint16_t)(g_sum + r.quotient); a = (uint16_t)(a + 3u); }
    report("div16_epic", t0);
}
#endif

void main(void) {
    uart_init();
    T1CON = TMR1_CONF;
    uart_puts("\r\nbench-start\r\n");
    bench_loop_empty();
    bench_add_native();
#ifndef BENCH_NATIVE_ONLY
    bench_add_epic();
#endif
    bench_sub_native();
#ifndef BENCH_NATIVE_ONLY
    bench_sub_epic();
#endif
    bench_mul8_native();
#ifndef BENCH_NATIVE_ONLY
    bench_mul8_epic();
#endif
    bench_mul16_native();
#ifndef BENCH_NATIVE_ONLY
    bench_mul16_epic();
#endif
    bench_div16_native();
#ifndef BENCH_NATIVE_ONLY
    bench_div16_epic();
#endif
    uart_puts("bench-done\r\n");
    for (;;) { }
}
