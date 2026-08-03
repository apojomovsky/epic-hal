/**
 * @file    peripherals/pic18fxx5x_eeprom.h
 * @brief   Data EEPROM driver.
 *
 * @details
 *   256 bytes of data EEPROM (DS39632E §7.0). Mirrors
 *   `pic16f87xa_eeprom.h`'s API; PIC18 moves the registers into the Access
 *   Bank and adds EEPGD/CFGS in EECON1 (kept 0 for data EEPROM). The
 *   driver hides the 0x55/0xAA unlock sequence; writes are non-blocking,
 *   the caller polls EEIF (PIR2<4>) for write-cycle completion.
 */

#ifndef PIC18FXX5X_EEPROM_H
#define PIC18FXX5X_EEPROM_H

#include "pic18fxx5x.h"
#include "pic18fxx5x_sfr.h"

/**
 * @brief  Initialize the EEPROM driver. Programs PIE2<EEIE> if
 *         `callback` is non-NULL. */
HAL_StatusTypeDef HAL_EEPROM_Init(void (*callback)(void));

/** Disable the EEPROM module and clear EEIF. */
HAL_StatusTypeDef HAL_EEPROM_DeInit(void);

/**
 * @brief  Read one byte from data EEPROM. Loads EEADR with `addr`,
 *         sets EECON1<RD> (with EEPGD=0), and returns the byte from
 *         EEDATA. Per §7.1: EEADR must be loaded first, then RD.
 */
uint8_t HAL_EEPROM_ReadByte(uint8_t addr);

/**
 * @brief  Write one byte to data EEPROM at `addr`. Performs the mandatory
 *         unlock sequence (0x55 -> 0xAA -> WR), with EEPGD=0.
 * @return HAL_OK on success, HAL_ERROR if a previous write was aborted
 *         (WRERR set).
 */
HAL_StatusTypeDef HAL_EEPROM_WriteByte(uint8_t addr, uint8_t data);

/** Read a contiguous block. */
void HAL_EEPROM_ReadBuffer(uint8_t start, uint8_t *buf, uint8_t len);

/** Write a contiguous block. */
HAL_StatusTypeDef HAL_EEPROM_WriteBuffer(uint8_t start,
                                              const uint8_t *buf,
                                              uint8_t len);

/** Returns 1 if EEIF is set (write cycle complete). */
uint8_t HAL_EEPROM_IsWriteComplete(void);

/** Clear EEIF (must be cleared in the user's IRQ). */
void HAL_EEPROM_ClearITFlag(void);

void EEPROM_IRQHandler(void) PIC8_WEAK;

#endif /* PIC18FXX5X_EEPROM_H */
