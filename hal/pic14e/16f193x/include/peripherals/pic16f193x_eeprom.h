/**
 * PIC16F193X data EEPROM driver (DS41364B §23.0). 256 bytes on every
 * variant. Data space only this phase (program-memory self-write
 * deferred). Full reference: MANUAL.md.
 */
#ifndef PIC16F193X_EEPROM_H
#define PIC16F193X_EEPROM_H

#include "pic16f193x.h"
#include "pic16f193x_sfr.h"

#define EEPROM_SIZE_BYTES 256U

/**
 * @brief Initialize the data EEPROM driver. No hardware setup is
 *        required this phase; provided for API symmetry.
 * @return EPIC_OK on success
 */
EPIC_StatusTypeDef EPIC_EEPROM_Init(void);
/**
 * @brief Deinitialize the EEPROM driver: clears the WREN write-enable
 *        bit.
 * @return EPIC_OK on success
 */
EPIC_StatusTypeDef EPIC_EEPROM_DeInit(void);
/**
 * @brief Read one byte: sets the address, starts a read, and returns
 *        the data register value.
 * @param addr byte address (0-255)
 * @return the byte stored at addr
 */
uint8_t EPIC_EEPROM_ReadByte(uint8_t addr);
/**
 * @brief Write one byte: loads address and data, performs the 0x55/0xAA
 *        unlock sequence with interrupts disabled, then waits for the
 *        write to finish and clears WREN.
 * @param addr byte address (0-255)
 * @param data byte to store
 * @return EPIC_OK on success, EPIC_ERROR if the write error flag (WRERR)
 *         is set
 */
EPIC_StatusTypeDef EPIC_EEPROM_WriteByte(uint8_t addr, uint8_t data);
/**
 * @brief Poll whether an in-progress write has finished.
 * @return 1 when no write is in progress, 0 while WR is still set
 */
uint8_t EPIC_EEPROM_IsWriteComplete(void);
/**
 * @brief Check whether the last write failed (EECON1<WRERR>).
 * @return 1 if a write error occurred, 0 otherwise
 */
uint8_t EPIC_EEPROM_HasWriteError(void);

/**
 * @brief Weak EEPROM ISR, override in user code.
 */
void EEPROM_IRQHandler(void) EPIC_WEAK;

#endif /* PIC16F193X_EEPROM_H */
