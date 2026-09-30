/* Pure footprint probe for epic-cc: reference the filter state only.
 * oversample/avg_push are stubbed under __EPIC_CC__ (callback-param
 * call and runtime-indexed buffer deref hit isel gaps), so this links
 * the module and reports its footprint without the filter bodies.
 * XC8 keeps the full sizecheck. */
#include "epic_adcfilter.h"
#include <stdint.h>

static epic_adcfilter_avg_t g_f;

/** @brief Main. @return 0. */
int main(void)
{
    g_f.count = 4u;
    return (int)g_f.count;
}
