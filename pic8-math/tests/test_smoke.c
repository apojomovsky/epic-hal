/**
 * @file    test_smoke.c
 * @brief   Minimal link/compile smoke test: proves the CMake host build
 *          links the pic_math static library and the public API types
 *          are visible from the header alone.
 */

#include "pic_math.h"

int main(void)
{
    /* The API types must be visible from the header alone. */
    pic_math_udiv16_t u16;
    pic_math_sdiv16_t s16;
    (void)u16;
    (void)s16;

    /* No routines are implemented yet in Phase 0; reaching here means the
     * library and header both link and compile. */
    return 0;
}
