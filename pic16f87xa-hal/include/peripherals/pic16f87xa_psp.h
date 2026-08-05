/**
 * @file    peripherals/pic16f87xa_psp.h
 * @brief   Parallel Slave Port driver (40/44-pin only).
 *
 * @details
 *   Source: DS39582B §4.5. Full reference: MANUAL.md §20. 40/44-pin
 *   only (PIC16F873A/876A have no PSP); gated by
 *   `PIC16F87XA_FAMILY_HAS_PSP`. Manages PSPMODE, IBF/OBF/IBOV flags,
 *   and the PSP interrupt; an external master drives CS/RD/WR on real
 *   silicon, the sim backend drives them via pic16f87xa_sim.h.
 */

#ifndef PIC16F87XA_PSP_H
#define PIC16F87XA_PSP_H

#include "pic16f87xa.h"
#include "pic16f87xa_sfr.h"

#if !PIC16F87XA_FAMILY_HAS_PSP
#error "pic16f87xa_psp.h is for 40/44-pin PIC16F87XA parts only"
#endif

EPIC_StatusTypeDef EPIC_PSP_Init(void (*callback)(void));
EPIC_StatusTypeDef EPIC_PSP_DeInit(void);

/** Enable Parallel Slave Port mode (TRISE<PSPMODE> = 1). */
void EPIC_PSP_Enable(void);

/** Disable Parallel Slave Port mode. */
void EPIC_PSP_Disable(void);

/** Returns 1 if the PSP input buffer is full (TRISE<IBF>). */
uint8_t EPIC_PSP_IsInputBufferFull(void);

/** Returns 1 if the PSP output buffer is full (TRISE<OBF>). */
uint8_t EPIC_PSP_IsOutputBufferFull(void);

/** Returns 1 if an input buffer overflow occurred (TRISE<IBOV>). */
uint8_t EPIC_PSP_HasInputOverflow(void);

/** Clear the IBOV flag. Must be done in software. */
void EPIC_PSP_ClearInputOverflow(void);

void PSP_IRQHandler(void) EPIC_WEAK;

#endif /* PIC16F87XA_PSP_H */
