/* Real-driver epic-cc build and PORTB execution gate: links the real
 * encoder.c (tick symbols provided here; epic-tick is HAL-3c, #86) and
 * runs init plus three port samples, one glitch-rejected through the
 * real tick gate; the glitch count lands on PORTB for mdb-hex. */
#include "encoder.h"
#include "epic_hal.h"

/* No manifest config words on this path; keep the WDT off so the
 * MPLAB SIM gate is not reset mid-run. */
EPIC_CONFIG("osc=hs, wdt=off, xtal_hz=20000000");

/* PORTB as output, value written below (the mdb-hex execution gate). */
#define TRISB_REG PIC_REG_TRISB
#define PORTB_REG PIC_REG_PORTB

static uint32_t g_now;

uint32_t epic_tick_get(void) { return g_now; }
uint32_t epic_tick_elapsed_since(uint32_t t0) { return g_now - t0; }

static epic_encoder_t g_enc;

/** @brief Main. @return 0. */
int main(void)
{
    epic_encoder_init(&g_enc, 4, 5, 5u, 0x00u);
    g_now += 10u; epic_encoder_update(&g_enc, 0x20u);  /* 00->01, accepted */
    g_now += 10u; epic_encoder_update(&g_enc, 0x30u);  /* 01->11, accepted */
    g_now += 1u;  epic_encoder_update(&g_enc, 0x10u);  /* 11->10, 1 ms later: glitch */
    (void)epic_encoder_get_position(&g_enc);
    EPIC_REG8(TRISB_REG) = (uint8_t)0x00u;
    EPIC_REG8(PORTB_REG) = (uint8_t)epic_encoder_get_glitch_count(&g_enc);
    for (;;) {
    }
}
