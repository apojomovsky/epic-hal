/* Epic-cc gate firmware for the PIC16F5x family: a software-loop
 * PORTB:0 toggle driving the registers directly. The canonical blink
 * is unobservable here (crates/sim's PicBaseline core never advances
 * TMR0 and the die has no interrupt to inject), and the HAL GPIO
 * driver is flash-bound on this part: driver plus a minimal toggle
 * program is 557 of 512 words, measured under the corrected
 * epic-cc#437 RAM model. The shape is still the blink's: direction
 * out, set, delay, clear, delay. */

#include <stdint.h>

/** Delay loop length: a few thousand cycles per phase, far inside the
 * runner's 200000-step budget across 12 samples. */
#define BLINK_DELAY 500U

static volatile uint8_t * const portb = (volatile uint8_t *)0x06;

/**
 * @brief Burn a few thousand cycles between pin phases.
 */
static void delay(void)
{
    volatile uint16_t n = BLINK_DELAY;

    while (n-- != 0U)
    {
    }
}

/**
 * @brief Toggle RB0 forever from a software delay loop.
 */
int main(void)
{
    asm volatile ("movlw 0x00");
    asm volatile ("tris 6");    /* RB0 drives: no TRIS file address exists. */
    *portb = 0x00U;
    for (;;)
    {
        *portb = 0x01U;
        delay();
        *portb = 0x00U;
        delay();
    }
}
