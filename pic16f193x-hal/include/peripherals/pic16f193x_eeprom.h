/**
 * @file    peripherals/pic16f193x_eeprom.h
 * @brief   PIC16F193X data EEPROM driver (DS41364B §23.0).
 * @details 256 bytes on every variant. Data space only this phase
 *          (program-memory self-write deferred). Full reference:
 *          MANUAL.md.
 */
#ifndef PIC16F193X_EEPROM_H
#define PIC16F193X_EEPROM_H

#include "pic16f193x.h"
#include "pic16f193x_sfr.h"

#define EEPROM_SIZE_BYTES 256U

EPIC_StatusTypeDef EPIC_EEPROM_Init(void);
EPIC_StatusTypeDef EPIC_EEPROM_DeInit(void);
uint8_t EPIC_EEPROM_ReadByte(uint8_t addr);
EPIC_StatusTypeDef EPIC_EEPROM_WriteByte(uint8_t addr, uint8_t data);
uint8_t EPIC_EEPROM_IsWriteComplete(void);
uint8_t EPIC_EEPROM_HasWriteError(void);

void EEPROM_IRQHandler(void) EPIC_WEAK;

#endif /* PIC16F193X_EEPROM_H */
