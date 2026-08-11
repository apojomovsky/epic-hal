/* Cross-compile sizecheck: a minimal main exercising both filters so
 * the XC8 build links the real code and reports flash/RAM footprint.
 * No HAL, no config word. */
#include "epic_adcfilter.h"
#include <stddef.h>     /* NULL */

static uint16_t stub_read(void *ctx) { (void)ctx; return 512u; }

void main(void)
{
    uint16_t r = epic_adcfilter_oversample(stub_read, NULL, 2);
    uint16_t buf[4];
    epic_adcfilter_avg_t f;
    epic_adcfilter_avg_init(&f, buf, 4);
    epic_adcfilter_avg_push(&f, r);
    while (1) { }
}
