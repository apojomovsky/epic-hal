/* Pure footprint probe for epic-cc: no tick/HAL. */
#include "encoder.h"

static epic_encoder_t g_enc;

/** @brief Main. @return 0. */
int main(void)
{
    // No tick - just reference the struct to pull encoder.c, no init
    // that would need timer2. XC8 keeps the full tick-driven sizecheck.
    (void)g_enc.position;
    return 0;
}
