/**
 * @file    example_encoder_hal.c
 * @brief   Two quadrature encoders sharing one PORTB RB<7:4> interrupt-on-change,
 *          wired end to end through EPIC_GPIO_RegisterChangeCallback.
 *
 * @details
 *   Encoder A on RB4/RB5, B on RB6/RB7 (the at-most-two-per-port ceiling,
 *   see docs/API.md); one callback fans the PORTB byte out to both via
 *   `encoder_update`. The host sim doesn't auto-assert RBIF on a mismatch,
 *   so this example drives PORTB, sets RBIF, and calls
 *   `epic_dispatch_all_irqs()` directly to simulate an edge.
 */

#include "encoder.h"
#include "epic_tick.h"
#include "epic_hal.h"
#include "core/epic_harness.h"

#ifndef FOSC_HZ
#define FOSC_HZ 20000000UL
#endif

#define SIM_CYCLES 4000000UL

/* Two encoders on RB<7:4>: A on RB4/RB5, B on RB6/RB7. */
static encoder_t g_enc_a, g_enc_b;

/* The one RB-change callback the HAL calls. Fans the already-read PORTB byte
 * out to both instances; the HAL does not know how many consumers there are. */
static void on_rb_change(uint8_t portb_value)
{
    encoder_update(&g_enc_a, portb_value);
    encoder_update(&g_enc_b, portb_value);
}

/* Build a PORTB byte putting `state_a` (a<<1|b) on RB4/RB5 and `state_b` on
 * RB6/RB7 (the same bit-position convention encoder_init is given). */
static uint8_t make_portb(uint8_t state_a, uint8_t state_b)
{
    uint8_t v = 0U;
    if (state_a & 0x2U) v |= (uint8_t)(1U << 4);   /* A channel A bit -> RB4 */
    if (state_a & 0x1U) v |= (uint8_t)(1U << 5);   /* A channel B bit -> RB5 */
    if (state_b & 0x2U) v |= (uint8_t)(1U << 6);   /* B channel A bit -> RB6 */
    if (state_b & 0x1U) v |= (uint8_t)(1U << 7);   /* B channel B bit -> RB7 */
    return v;
}

/* Simulate one RB-change interrupt: drive the byte, assert RBIF, dispatch. */
static void sim_rb_edge(uint8_t portb)
{
    PIC8_REG8(PIC_REG_PORTB)   = portb;
    PIC8_REG8(PIC_REG_INTCON) |= PIC_INTCON_RBIF;   /* test-only RBIF assertion */
    epic_dispatch_all_irqs();                        /* -> RB_IRQHandler -> on_rb_change */
}

int main(void)
{
    epic_harness_init(SIM_CYCLES);
    epic_tick_init(FOSC_HZ);

    /* Seed PORTB to a known state before reading it for encoder_init. */
    EPIC_GPIO_Init(GPIOB, GPIO_PIN_4 | GPIO_PIN_5 | GPIO_PIN_6 | GPIO_PIN_7,
                  GPIO_MODE_INPUT);
    uint8_t start = make_portb(0, 0);
    PIC8_REG8(PIC_REG_PORTB) = start;

    EPIC_GPIO_RegisterChangeCallback(on_rb_change);
    encoder_init(&g_enc_a, 4, 5, 0, start);   /* A on RB4/RB5, gate off */
    encoder_init(&g_enc_b, 6, 7, 0, start);   /* B on RB6/RB7, gate off */

    /* Two full rotations, interleaved on the shared byte:
     *   A: 00->10->11->01->00 x2  (table's positive direction, +8)
     *   B: 00->01->11->10->00 x2  (negative direction, -8) */
    static const uint8_t a_seq[8] = { 2, 3, 1, 0, 2, 3, 1, 0 };
    static const uint8_t b_seq[8] = { 1, 3, 2, 0, 1, 3, 2, 0 };
    for (int i = 0; i < 8; i++)
        sim_rb_edge(make_portb(a_seq[i], b_seq[i]));

    /* One step per instance, proving the shared byte doesn't cross-react. */
    sim_rb_edge(make_portb(2, 0));   /* A: 00->10 (+1);  B: 00==00 no-op   */
    sim_rb_edge(make_portb(2, 1));   /* A: 10==10 no-op; B: 00->01 (-1)    */

    int32_t pa = encoder_get_position(&g_enc_a);
    int32_t pb = encoder_get_position(&g_enc_b);
    uint16_t ea = encoder_get_error_count(&g_enc_a);
    uint16_t eb = encoder_get_error_count(&g_enc_b);

    epic_harness_log("encoder A (RB4/RB5): position=%ld errors=%u\n",
                     (long)pa, (unsigned)ea);
    epic_harness_log("encoder B (RB6/RB7): position=%ld errors=%u\n",
                     (long)pb, (unsigned)eb);

    int ok = (pa == 9)  && (pb == -9) && (ea == 0) && (eb == 0);
    return epic_harness_report(ok);
}
