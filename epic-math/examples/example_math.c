/*
 * epic-math on-target demo: a free-running 16-bit counter feeds the
 * fixed-point core. Each step multiplies the counter by a constant
 * (16x16 -> 32), divides it with remainder (16/16, divide-by-zero
 * flagged via the ok out-param), and takes its integer square root;
 * the results stream over serial at 9600 baud. The counter wraps at
 * 65535, so the full input range is exercised.
 */

#include "epic_hal.h"
#include "epic_math.h"
#include "epic_serial.h"

#define BAUD_RATE    9600u
#define MUL_FACTOR   321u
#define DIV_DIVISOR  13u

/**
 * @brief Transmit a NUL-terminated string over the serial ring.
 */
static void putstr(const char *s)
{
    int len = 0;
    while (s[len] != '\0') {
        len++;
    }
    epic_serial_write((const uint8_t *)s, len);
}

/**
 * @brief Transmit an unsigned value as decimal digits.
 */
static void putu32(uint32_t v)
{
    char buf[10];
    int n = 0, i;
    do {
        buf[n++] = (char)('0' + (int)(v % 10u));
        v /= 10u;
    } while (v > 0u);
    for (i = 0; i < n / 2; i++) {
        char t = buf[i];
        buf[i] = buf[n - 1 - i];
        buf[n - 1 - i] = t;
    }
    epic_serial_write((const uint8_t *)buf, n);
}

/**
 * @brief Stream multiply/divide/sqrt results for a running counter.
 */
int main(void)
{
    epic_serial_init(FOSC_HZ, BAUD_RATE);
    EPIC_IRQ_Restore(1);

    uint16_t counter = 0u;
    for (;;) {
        uint32_t product = epic_math_mul_u16(counter, MUL_FACTOR);

        bool ok;
        epic_math_udiv16_t div = epic_math_divmod_u16(counter,
                                                      DIV_DIVISOR, &ok);

        uint16_t root = epic_math_sqrt_u16(counter);

        putstr("cnt=");
        putu32(counter);
        putstr(" mul=");
        putu32(product);
        putstr(" div=");
        putu32(div.quotient);
        putstr(" rem=");
        putu32(div.remainder);
        putstr(" sqrt=");
        putu32(root);
        putstr("\r\n");

        counter++;
        EPIC_WDT_Refresh();
    }
}
