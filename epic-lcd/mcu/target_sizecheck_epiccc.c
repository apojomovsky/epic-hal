/* Pure footprint probe for epic-cc: reference the LCD state only.
 * The ops dispatch is stubbed under __EPIC_CC__ (iselcore gep gap on
 * the ops chain), so this links the core and reports its footprint
 * without the transport calls. XC8 keeps the full example. */
#include "epic_lcd.h"
#include <stdint.h>

static epic_lcd_t g_lcd;

/** @brief Main. @return 0. */
int main(void)
{
    g_lcd.cols = 16u;
    return (int)g_lcd.cols;
}
