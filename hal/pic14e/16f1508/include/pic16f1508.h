/**
 * PIC16F1508/1509 HAL umbrella: device select, port widths, and the SFR map.
 * Family headers include this; consumers use epic_hal.h.
 */
#ifndef PIC16F1508_H
#define PIC16F1508_H

#include "core/hal_status.h"
#include "pic16f1508_platform.h"

#if defined(PIC16F1508) && defined(PIC16F1509)
#error "define only one of PIC16F1508 or PIC16F1509"
#endif

#if !defined(PIC16F1508) && !defined(PIC16F1509)
#define PIC16F1509
#endif

#if defined(PIC16F1508)
#define PIC16F1508_DEVICE_NAME "PIC16F1508"
#else
#define PIC16F1508_DEVICE_NAME "PIC16F1509"
#endif

/* Implemented pins per port, from the pinned DFP/EDC. PORTB is RB4..RB7 only
 * (PORTB impl=0xF0). LATA is 0x37 (LATA3 unimplemented, RA3 is input-only), so
 * the latch mask is narrower than the pin mask. PORTC width is unverified
 * against the datasheet. */
#define PIC16F1508_FAMILY_PORTA_MASK 0x3FU
#define PIC16F1508_FAMILY_LATA_MASK  0x37U
#define PIC16F1508_FAMILY_PORTB_MASK 0xF0U
#define PIC16F1508_FAMILY_PORTC_MASK 0xFFU
/* IOCBP/IOCBN/IOCBF are implemented for RB4..RB7 only. */
#define PIC16F1508_FAMILY_IOCB_MASK  0xF0U

#include "pic16f1508_sfr.h"

#endif
