/**
 * PIC16F18855 HAL umbrella: device select, port widths, and the SFR map.
 * Family headers include this; consumers use epic_hal.h.
 */
#ifndef PIC16F188XX_H
#define PIC16F188XX_H

#include "core/hal_status.h"
#include "pic16f188xx_platform.h"

#if !defined(PIC16F18855)
#define PIC16F18855
#endif

#define PIC16F188XX_DEVICE_NAME "PIC16F18855"
/* Implemented pins per port, from the pinned DFP EDC (PIC16F18855.PIC).
 * Every PORTx/TRISx/LATx/WPUx/ANSELx/IOCx register reports impl 0xff,
 * except the PORTC read register which reports impl 0xc0 (all eight RC
 * fields are defined and TRISC/LATC report 0xff, so the 0xFF mask
 * stands). RA6/RA7 are digital I/O under the manifest config
 * (FEXTOSC=OFF), so the 28-pin part implements all of PORTA..PORTC.
 * RE3 is input-only MCLR and has no driver. Sibling 28-pin parts share
 * this map; the 40-pin parts add PORTD/PORTE in a follow-up. */
#define PIC16F188XX_FAMILY_PORTA_MASK 0xFFU
#define PIC16F188XX_FAMILY_LATA_MASK  0xFFU
#define PIC16F188XX_FAMILY_PORTB_MASK 0xFFU
#define PIC16F188XX_FAMILY_PORTC_MASK 0xFFU
/* IOCxP/IOCxN/IOCxF are implemented for every pin of PORTA..PORTC. */
#define PIC16F188XX_FAMILY_IOCA_MASK  0xFFU
#define PIC16F188XX_FAMILY_IOCB_MASK  0xFFU
#define PIC16F188XX_FAMILY_IOCC_MASK  0xFFU
/* Timer drivers are not ported yet; demos gate timer use on these so the
 * same demo source builds once they land. */
#define PIC16F188XX_HAS_TMR1 0
#define PIC16F188XX_HAS_TMR2 0

#include "pic16f188xx_sfr.h"

#endif
