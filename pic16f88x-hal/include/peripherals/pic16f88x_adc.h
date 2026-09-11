/* A/D converter driver, 88X flavor. Source: DS40001291H section 15.0.
 * Thin shim: the shared body (pic14-midrange-core/include/
 * peripherals/pic14_adc.h) carries the driver; this header picks the
 * family umbrella + SFR map. PIC14MIDRANGE_HAS_ADC_PCFG (in
 * pic14_midrange.h) selects the 88X reference/mux/clock flavors and
 * PIC14MIDRANGE_HAS_ANSEL exposes EPIC_ADC_ConfigChannel. */

#ifndef PIC16F88X_ADC_H
#define PIC16F88X_ADC_H

#include "pic16f88x.h"
#include "pic16f88x_sfr.h"
#include "peripherals/pic14_adc.h"

#endif /* PIC16F88X_ADC_H */
