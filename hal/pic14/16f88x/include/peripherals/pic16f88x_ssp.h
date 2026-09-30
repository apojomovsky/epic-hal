/* MSSP driver, 88X flavor. Source: DS40001291H section 13.0. Thin shim:
 * the shared body (hal/pic14/core/include/peripherals/pic14_ssp.h)
 * carries the driver; this header picks the family umbrella + SFR map.
 * PIC14MIDRANGE_HAS_SSPMSK (in pic14_midrange.h) selects the 88X enum
 * flavor and exposes EPIC_SSP_LoadAddressMask. */

#ifndef PIC16F88X_SSP_H
#define PIC16F88X_SSP_H

#include "pic16f88x.h"
#include "pic16f88x_sfr.h"
#include "peripherals/pic14_ssp.h"

#endif /* PIC16F88X_SSP_H */
