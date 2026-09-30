/*
 * Data EEPROM driver, 256 bytes (DS39605F §7.0). Same shape and API as
 * `pic18fxx5x_eeprom.h`: the EEPROM registers are in the Access Bank
 * (EECON1/EECON2/EEDATA/EEADR at 0xFA6-0xFA9), no bank switching; a
 * byte-write performs the mandatory 0x55/0xAA unlock sequence. The
 * write-complete flag is PIR2<EEIF>.
 */

#ifndef PIC18F1320_EEPROM_H
#define PIC18F1320_EEPROM_H

#include "pic18f1320.h"
#include "pic18f1320_sfr.h"

/**
 * @brief  Initialize the data EEPROM driver: store the write-complete
 *         callback, clear EEIF and enable the EEPROM interrupt if a
 *         callback is given.
 * @param callback Function invoked from EEPROM_IRQHandler, or NULL.
 * @return EPIC_OK.
 */
EPIC_StatusTypeDef EPIC_EEPROM_Init(void (*callback)(void));

/**
 * @brief  Disable the EEPROM module: clear EEIF, disable its interrupt,
 *         restore EECON1 to 0x00 and drop the stored callback.
 * @return EPIC_OK.
 */
EPIC_StatusTypeDef EPIC_EEPROM_DeInit(void);

/**
 * @brief  Read one byte from data EEPROM. Loads EEADR, sets EECON1<RD>
 *         (with EEPGD = 0), and returns the byte from EEDATA. On the
 *         host sim this pulls from the flat-array model; on a real
 *         target the RD strobe loads the addressed byte into EEDATA.
 * @param addr Data EEPROM address to read.
 * @return The byte stored at `addr`.
 */
uint8_t EPIC_EEPROM_ReadByte(uint8_t addr);

/**
 * @brief  Write one byte to data EEPROM at `addr`. Performs the mandatory
 *         unlock sequence (0x55 -> 0xAA -> WR), with EEPGD = 0. The WR
 *         bit self-clears when the cycle completes; poll
 *         @ref EPIC_EEPROM_IsWriteComplete.
 * @param addr Data EEPROM address to write.
 * @param data Byte value to store.
 * @return EPIC_OK on success, EPIC_ERROR if a previous write was aborted
 *         (WRERR set).
 */
EPIC_StatusTypeDef EPIC_EEPROM_WriteByte(uint8_t addr, uint8_t data);

/**
 * @brief  Read a contiguous block of bytes from data EEPROM.
 * @param start First EEPROM address to read.
 * @param buf Destination buffer, at least `len` bytes.
 * @param len Number of bytes to read.
 */
void EPIC_EEPROM_ReadBuffer(uint8_t start, uint8_t *buf, uint8_t len);

/**
 * @brief  Write a contiguous block of bytes to data EEPROM.
 * @param start First EEPROM address to write.
 * @param buf Source buffer, at least `len` bytes.
 * @param len Number of bytes to write.
 * @return EPIC_OK on success, or the first EPIC_ERROR returned by a
 *         single-byte write.
 */
EPIC_StatusTypeDef EPIC_EEPROM_WriteBuffer(uint8_t start,
                                           const uint8_t *buf,
                                           uint8_t len);

/**
 * @brief  Return 1 if EEIF is set, i.e. the write cycle has completed.
 * @return 1 if the last write finished, else 0.
 */
uint8_t EPIC_EEPROM_IsWriteComplete(void);

/**
 * @brief  Clear EEIF; must be cleared in the user's IRQ handler.
 */
void EPIC_EEPROM_ClearITFlag(void);

/**
 * @brief  Weak EEPROM interrupt handler, override in user code.
 */
void EEPROM_IRQHandler(void) EPIC_WEAK;

#endif /* PIC18F1320_EEPROM_H */
