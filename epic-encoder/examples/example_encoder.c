/*
 * epic-encoder target example: x4 quadrature decode on the RB<7:4>
 * interrupt-on-change, position logged over serial. Wire channel A to
 * RB4 and channel B to RB5; see docs/API.md for the port-byte bit
 * convention and the at-most-two-encoders-per-port ceiling.
 */

#include "encoder.h"
#include "epic_tick.h"
#include "epic_serial.h"
#include "epic_hal.h"

#ifndef __EPIC_CC__
 #include <stdio.h>
#endif

/* The IRQ source enum is family-specific; the build defines PIC<mcu>,
 * same pattern epic-serial uses for its TX/RX IRQ ids. */
#if defined(PIC18F2455) || defined(PIC18F2550) || defined(PIC18F4455) || defined(PIC18F4550)
#define EXAMPLE_IRQ_RB PIC18_IRQ_RB
#else
#define EXAMPLE_IRQ_RB PIC16_IRQ_RB
#endif

#define ENC_PIN_A      4u      /* RB4 */
#define ENC_PIN_B      5u      /* RB5 */
#define GLITCH_GATE_MS 5u      /* min edge interval; 0 disables the gate */
#define LOG_PERIOD_MS  250u

static epic_encoder_t g_encoder;

/**
 * @brief RB-change callback: forward the freshly-read PORTB byte to the decoder.
 */
static void on_rb_change(uint8_t portb_value)
{
    epic_encoder_update(&g_encoder, portb_value);
}

/**
 * @brief Decode x4 quadrature on RB<7:4> and log position over serial.
 */
int main(void)
{
    epic_tick_init(FOSC_HZ);
    epic_serial_init(FOSC_HZ, 115200u);
    EPIC_IRQ_Restore(1);

    EPIC_GPIO_Init(GPIOB, GPIO_PIN_4 | GPIO_PIN_5, GPIO_MODE_INPUT);
    EPIC_GPIO_RegisterChangeCallback(on_rb_change);
    epic_encoder_init(&g_encoder, ENC_PIN_A, ENC_PIN_B, GLITCH_GATE_MS,
                      EPIC_GPIO_ReadPort(GPIOB));
    EPIC_IRQ_Enable(EXAMPLE_IRQ_RB);

    // epic-cc: stdio not available under clang; keep footprint without printf
#ifndef __EPIC_CC__
    printf("epic-encoder: x4 quadrature on RB4/RB5, position logged\r\n");
#endif

    uint32_t last_log = epic_tick_get();
    for (;;) {
        if (epic_tick_elapsed_since(last_log) >= LOG_PERIOD_MS) {
            last_log = epic_tick_get();
#ifndef __EPIC_CC__
            printf("pos=%ld err=%u glitch=%u\r\n",
                   (long)epic_encoder_get_position(&g_encoder),
                   (unsigned)epic_encoder_get_error_count(&g_encoder),
                   (unsigned)epic_encoder_get_glitch_count(&g_encoder));
#endif
        }
    }
}
