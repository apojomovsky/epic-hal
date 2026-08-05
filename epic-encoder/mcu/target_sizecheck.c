/**
 * @file    target_sizecheck.c
 * @brief   Minimal on-target build proving encoder.c (and the epic-tick
 *          timebase it links) cross-compiles for real XC8/PIC16/PIC18
 *          silicon and reporting flash/RAM footprint; not a correctness
 *          test (see ../tests/test_encoder.c for that).
 *
 * Exercises the glitch-gate path, the QUAD_TABLE decode path, and all
 * three atomic getters, so the linker pulls in every code path an
 * on-target application would use. Measured footprint is recorded in
 * docs/ARCHITECTURE.md.
 */

#include "encoder.h"
#include "epic_tick.h"

static encoder_t g_enc;

int main(void)
{
    epic_tick_init(FOSC_HZ);

    /* Gate armed so the epic_tick timebase path in encoder_update links. */
    encoder_init(&g_enc, 4, 5, 5, 0x00U);

    for (;;) {
        encoder_update(&g_enc, 0x20U);   /* RB5 high -> state 01 */
        encoder_update(&g_enc, 0x30U);   /* RB4+RB5 high -> state 11 */

        (void)encoder_get_position(&g_enc);
        (void)encoder_get_error_count(&g_enc);
        (void)encoder_get_glitch_count(&g_enc);
    }
}
