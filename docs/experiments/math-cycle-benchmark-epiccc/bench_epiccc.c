#include <stdint.h>
#include <stdbool.h>
#include "epic_math.h"
#include <epic-cc.h>

/* Cycle benchmark via TMR1 (counts instruction cycles on the 87XA/88X),
 * the epic-cc twin of docs/experiments/math-cycle-benchmark/bench.c.
 * Same probe shape, same operands, same UART reporting, so the numbers
 * line up against the hand-asm tables in that directory's README. The
 * difference: this build links the portable C path (epic-math/src/host/
 * + src/common/) under epic-cc instead of the XC8-dialect inline-asm
 * backends. N=50, not 100: the 16-bit TMR1 wraps at 65536 and the
 * C-path mul16/div16 loops at N=100 exceed it (see this directory's
 * README).
 *
 * Op labels are numeric IDs, not strings: epic-cc does not yet lower
 * const string tables reliably in multi-table layouts, and the label
 * is not what is being measured. The ID to op mapping is in this
 * directory's README. */

EPIC_CONFIG("osc=hs, wdt=off, pwrt=on, lvp=off, xtal_hz=20000000");

/* SFR access under epic-cc: a volatile dereference of the literal
 * address, the same shape the HAL's epiccc platform headers use. */
#define REG8(a) (*(volatile uint8_t *)(uintptr_t)(a))

#define TMR1L_REG 0x0EU
#define TMR1H_REG 0x0FU
#define T1CON_REG 0x10U
#define PIR1_REG  0x0CU
#define TXREG_REG 0x19U
#define TXSTA_REG 0x98U
#define SPBRG_REG 0x99U
#define RCSTA_REG 0x18U

#define TXIF_BIT  0x10U   /* PIR1 bit 4 */
#define BRGH_BIT  0x04U   /* TXSTA bit 2 */
#define TXEN_BIT  0x20U   /* TXSTA bit 5 */
#define SPEN_BIT  0x80U   /* RCSTA bit 7 */

#define N 50u

static void uart_putc(char c)
{
    while (!(REG8(PIR1_REG) & TXIF_BIT)) { }
    REG8(TXREG_REG) = (uint8_t)c;
}
static void uart_puthex(uint16_t v)
{
    for (int s = 12; s >= 0; s -= 4) {
        uint8_t d = (uint8_t)((v >> s) & 0xFu);
        uart_putc((char)(d < 10u ? '0' + d : 'A' + (d - 10u)));
    }
}
static void uart_init(void)
{
    REG8(SPBRG_REG) = 129u;   /* 20 MHz, BRGH=1: 20e6/(16*9600)-1 */
    REG8(TXSTA_REG) = (uint8_t)(BRGH_BIT | TXEN_BIT);
    REG8(RCSTA_REG) = SPEN_BIT;
}
static uint16_t tmr1(void)
{
    return (uint16_t)((uint16_t)REG8(TMR1H_REG) << 8 | REG8(TMR1L_REG));
}
static void report(uint8_t op, uint16_t t0)
{
    uint16_t dt = (uint16_t)(tmr1() - t0);
    uart_puthex((uint16_t)op); uart_putc(' '); uart_puthex(dt); uart_putc('\r'); uart_putc('\n');
}

volatile uint16_t g_seed = 0x1357u;
volatile uint16_t g_sum  = 0u;
volatile bool g_ok = false;

/* Op IDs: 0 loop_empty, 1 add_native, 2 add_epic, 3 sub_native,
 * 4 sub_epic, 5 mul8_native, 6 mul8_epic, 7 mul16_native,
 * 8 mul16_epic, 9 div16_native, A div16_epic. */

void bench_loop_empty(void)
{
    uint16_t a = g_seed; uint16_t t0 = tmr1();
    for (uint16_t i = 0u; i < N; i++) { g_sum = (uint16_t)(g_sum + a); a = (uint16_t)(a + 3u); }
    report(0u, t0);
}
void bench_add_native(void)
{
    uint16_t a = g_seed, b = 0xFFFFu; uint16_t t0 = tmr1();
    for (uint16_t i = 0u; i < N; i++) { g_sum = (uint16_t)(g_sum + (a + b)); a = (uint16_t)(a + 3u); }
    report(1u, t0);
}
void bench_add_epic(void)
{
    uint16_t a = g_seed, b = 0xFFFFu; uint16_t t0 = tmr1();
    for (uint16_t i = 0u; i < N; i++) { g_sum = (uint16_t)(g_sum + epic_math_add_u16(a, b, (bool *)&g_ok)); a = (uint16_t)(a + 3u); }
    report(2u, t0);
}
void bench_sub_native(void)
{
    uint16_t a = g_seed, b = 0x1357u; uint16_t t0 = tmr1();
    for (uint16_t i = 0u; i < N; i++) { g_sum = (uint16_t)(g_sum + (a - b)); a = (uint16_t)(a + 3u); }
    report(3u, t0);
}
void bench_sub_epic(void)
{
    uint16_t a = g_seed, b = 0x1357u; uint16_t t0 = tmr1();
    for (uint16_t i = 0u; i < N; i++) { g_sum = (uint16_t)(g_sum + epic_math_sub_u16(a, b, (bool *)&g_ok)); a = (uint16_t)(a + 3u); }
    report(4u, t0);
}
void bench_mul8_native(void)
{
    uint8_t a = (uint8_t)g_seed, b = 0xCDu; uint16_t t0 = tmr1();
    for (uint16_t i = 0u; i < N; i++) { g_sum = (uint16_t)(g_sum + (uint16_t)(a * b)); a = (uint8_t)(a + 3u); }
    report(5u, t0);
}
void bench_mul8_epic(void)
{
    uint8_t a = (uint8_t)g_seed, b = 0xCDu; uint16_t t0 = tmr1();
    for (uint16_t i = 0u; i < N; i++) { g_sum = (uint16_t)(g_sum + epic_math_mul_u8(a, b)); a = (uint8_t)(a + 3u); }
    report(6u, t0);
}
void bench_mul16_native(void)
{
    uint16_t a = g_seed, b = 0xCDEFu; uint16_t t0 = tmr1();
    for (uint16_t i = 0u; i < N; i++) { g_sum = (uint16_t)(g_sum + (uint16_t)(a * b)); a = (uint16_t)(a + 3u); }
    report(7u, t0);
}
void bench_mul16_epic(void)
{
    uint16_t a = g_seed, b = 0xCDEFu; uint16_t t0 = tmr1();
    for (uint16_t i = 0u; i < N; i++) { g_sum = (uint16_t)(g_sum + (uint16_t)epic_math_mul_u16(a, b)); a = (uint16_t)(a + 3u); }
    report(8u, t0);
}
void bench_div16_native(void)
{
    uint16_t a = g_seed, b = 0x0013u; uint16_t t0 = tmr1();
    for (uint16_t i = 0u; i < N; i++) { g_sum = (uint16_t)(g_sum + (a / b)); a = (uint16_t)(a + 3u); }
    report(9u, t0);
}
void bench_div16_epic(void)
{
    uint16_t a = g_seed, b = 0x0013u; uint16_t t0 = tmr1();
    for (uint16_t i = 0u; i < N; i++) { epic_math_udiv16_t r = epic_math_divmod_u16(a, b, (bool *)&g_ok); g_sum = (uint16_t)(g_sum + r.quotient); a = (uint16_t)(a + 3u); }
    report(0xAu, t0);
}

void main(void)
{
    uart_init();
    REG8(T1CON_REG) = 0x01u;   /* TMR1ON */
    g_seed = 0x1357u;   /* epic-cc has no RAM-initializer copy; write it */
    uart_putc('\r'); uart_putc('\n');
    bench_loop_empty();
    bench_add_native();
    bench_add_epic();
    bench_sub_native();
    bench_sub_epic();
    bench_mul8_native();
    bench_mul8_epic();
    bench_mul16_native();
    bench_mul16_epic();
    bench_div16_native();
    bench_div16_epic();
    for (;;) { }
}
