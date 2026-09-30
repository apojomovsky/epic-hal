/* Oscillator-fail support (DS40001262F §3.0, fail-safe clock
 * monitor). This family has no IRCF driver on this ticket; this file
 * exists to provide the weak OSF_IRQHandler the shared dispatcher
 * routes PIR2<OSFIF> to. */

#ifndef PIC16F63X_67X_68X_OSC_H
#define PIC16F63X_67X_68X_OSC_H

#include "pic16f63x_67x_68x_hal.h"
#include "pic16f63x_67x_68x_sfr.h"

/**
 * @brief Weak oscillator-fail ISR, override in user code.
 */
void OSF_IRQHandler(void) EPIC_WEAK;

#endif /* PIC16F63X_67X_68X_OSC_H */
