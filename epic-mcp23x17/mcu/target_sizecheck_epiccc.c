/* Pure footprint probe for epic-cc: reference the handle struct only.
 * The reg dispatch is stubbed under __EPIC_CC__ (iselcore gep gap on
 * the ops chain), so this links the module and reports its footprint
 * without the bus calls. XC8 keeps the full example. */
#include "epic_mcp23x17.h"
#include <stdint.h>

static epic_mcp23x17_handle_t g_h;

/** @brief Main. @return 0. */
int main(void)
{
    g_h.bus = EPIC_MCP23X17_BUS_I2C;
    g_h.dev = 0u;
    return (int)g_h.dev;
}
