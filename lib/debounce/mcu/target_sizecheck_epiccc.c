/* Pure footprint probe for epic-cc: no tick. */
#include "debounce.h"
#include <stdbool.h>
#include <stddef.h>

static epic_debounce_t g_db;

/** @brief Main. @return 0. */
int main(void)
{
    // No tick, no indirect, no null call args - just reference struct
    // to pull debounce.c. Avoids isel gaps (null sret, function ptr).
    (void)g_db.flags;
    return 0;
}
