/* Oscillator driver: internal HFINTOSC/LFINTOSC selection, tuning, and
 * fail-safe clock monitor. Source: DS40001291H §4.0; full reference:
 * MANUAL.md §Oscillator. The 88X has a factory-calibrated HFINTOSC
 * (8 MHz..31 kHz via OSCCON<IRCF2:IRCF0>), software-tunable through
 * OSCTUNE, and a Fail-Safe Clock Monitor (FCMEN config bit) that
 * switches to the internal oscillator and sets PIR2<OSFIF> when the
 * external clock fails. */

#ifndef PIC16F88X_OSC_H
#define PIC16F88X_OSC_H

#include "pic16f88x.h"
#include "pic16f88x_sfr.h"

/**
 * @brief Internal oscillator frequency select (OSCCON<IRCF2:IRCF0>,
 *        DS40001291H Register 4-1).
 */
typedef enum {
    OSC_IRCF_31KHZ   = 0x0U,   /**< 000: 31 kHz (LFINTOSC). */
    OSC_IRCF_125KHZ  = 0x1U,   /**< 001: 125 kHz. */
    OSC_IRCF_250KHZ  = 0x2U,   /**< 010: 250 kHz. */
    OSC_IRCF_500KHZ  = 0x3U,   /**< 011: 500 kHz. */
    OSC_IRCF_1MHZ    = 0x4U,   /**< 100: 1 MHz. */
    OSC_IRCF_2MHZ    = 0x5U,   /**< 101: 2 MHz. */
    OSC_IRCF_4MHZ    = 0x6U,   /**< 110: 4 MHz (reset default). */
    OSC_IRCF_8MHZ    = 0x7U,   /**< 111: 8 MHz. */
} OSC_InternalFreqTypeDef;

/**
 * @brief  Set the internal oscillator frequency (OSCCON<IRCF2:IRCF0>).
 *         Only effective when the system clock runs from the internal
 *         oscillator (SCS = 1, or after a Fail-Safe switchover).
 * @param freq the frequency select.
 */
void EPIC_OSC_SetInternalFreq(OSC_InternalFreqTypeDef freq);

/**
 * @brief  Read the current internal-oscillator frequency select.
 * @return the IRCF<2:0> value.
 */
uint8_t EPIC_OSC_GetInternalFreq(void);

/**
 * @brief  Select the system clock source (OSCCON<SCS>).
 * @param internal 1 = internal oscillator (HFINTOSC/LFINTOSC at the
 *        IRCF frequency), 0 = clock defined by the FOSC<2:0> config
 *        bits.
 */
void EPIC_OSC_SetSystemClockSource(uint8_t internal);

/**
 * @brief  Tune the HFINTOSC frequency (OSCTUNE<TUN4:TUN0>, 5-bit
 *         two's-complement, 0 = factory calibration).
 * @param tune the tuning value, 0x10..0x1F (negative) .. 0x0F (positive).
 */
void EPIC_OSC_Tune(uint8_t tune);

/**
 * @brief  Enable the fail-safe clock monitor interrupt (PIE2<OSFIE>).
 *         FCMEN must be set in the config word for the monitor itself
 *         to run; this arms the interrupt on OSFIF.
 * @param callback optional fail-safe callback (fires on OSFIF), or
 *        NULL to run in polling mode.
 * @return EPIC_OK on success.
 */
EPIC_StatusTypeDef EPIC_OSC_FailSafeInit(void (*callback)(void));

/**
 * @brief  De-initialize the fail-safe interrupt.
 * @return EPIC_OK on success.
 */
EPIC_StatusTypeDef EPIC_OSC_FailSafeDeInit(void);

/**
 * @brief  Returns 1 if an oscillator failure was detected (PIR2<OSFIF>).
 * @return 1 if the fail flag is set, 0 otherwise.
 */
uint8_t EPIC_OSC_IsFailSafe(void);

/**
 * @brief Clear the oscillator-fail flag (PIR2<OSFIF>). The fail-safe
 *        condition itself must be cleared first (Reset, SLEEP, or
 *        toggling SCS; DS40001291H §4.8).
 */
void EPIC_OSC_ClearFailSafeFlag(void);

/**
 * @brief Weak oscillator-fail ISR, override in user code.
 */
void OSF_IRQHandler(void) EPIC_WEAK;

#endif /* PIC16F88X_OSC_H */
