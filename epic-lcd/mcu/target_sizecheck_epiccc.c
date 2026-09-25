/* Pure footprint probe for epic-cc: reference the LCD state only.
 * This links the core and reports its footprint without transport
 * calls. XC8 keeps the full example. */
#include "epic_lcd.h"
#include <stdint.h>

static epic_lcd_t g_lcd;

/** @brief Main. @return 0. */
int main(void)
{
    g_lcd.cols = 16u;
    return (int)g_lcd.cols;
}
