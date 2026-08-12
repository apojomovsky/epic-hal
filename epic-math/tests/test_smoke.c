/*
 * Minimal link/compile smoke test: proves the CMake host build links
 * the epic_math static library and the public API types are visible from
 * the header alone.
 */

#include "epic_math.h"

/** @brief Link/compile smoke: proves the host build links and the header types are visible. */
int main(void)
{
    /* The API types must be visible from the header alone. */
    epic_math_udiv16_t u16;
    epic_math_sdiv16_t s16;
    (void)u16;
    (void)s16;

    return 0;
}
