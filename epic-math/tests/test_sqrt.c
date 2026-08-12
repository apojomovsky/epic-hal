/*
 * Host test for epic_math_sqrt_u16: exhaustive over 0..65535 against
 * (uint16_t)floor(sqrt((double)v)).
 */

#include "epic_math.h"
#include "epic_math_test.h"
#include <math.h>

/** @brief Exhaustive 0..65535 check of epic_math_sqrt_u16 against (uint16_t)floor(sqrt((double)v)). */
static void test_sqrt_exhaustive(void)
{
    for (uint32_t v = 0; v <= 0xFFFFu; v++) {
        uint16_t got = epic_math_sqrt_u16((uint16_t)v);
        uint16_t exp = (uint16_t)floor(sqrt((double)v));
        if (got != exp) {
            CHECK(0, "sqrt mismatch");
            if (g_epic_math_failures < 5)
                printf("  v=%lu got=%u exp=%u\n",
                       (unsigned long)v, got, exp);
        }
    }
}

/** @brief Run the sqrt test and report the failure count. */
int main(void)
{
    test_sqrt_exhaustive();
    printf("test_sqrt: %u checks failed\n", (unsigned)g_epic_math_failures);
    return epic_math_test_report();
}
