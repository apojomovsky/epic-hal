/**
 * PIC16F188xx data EEPROM driver (DS40001802D §10.4, Registers 10-3 to
 * 10-6). 256 bytes, reached through the NVM block: NVMREGS=1 with
 * NVMADRH:NVMADRL = 0xF000 + address. Reads return in the next
 * instruction. A write runs the 0x55/0xAA unlock, then proceeds on its
 * own while the CPU keeps executing: WR clears and NVMIF sets on
 * completion (about 5.6 ms, EDC wait time). The driver does not block on
 * it; poll EPIC_EEPROM_IsWriteComplete or take the callback. Program
 * Flash self-write is not covered. Full reference: MANUAL.md.
 */
#ifndef PIC16F188XX_EEPROM_H
#define PIC16F188XX_EEPROM_H

#include <stddef.h>
#include <stdint.h>
#include "pic16f188xx.h"

#define EEPROM_SIZE_BYTES 256U

/**
 * @brief Prepare the data EEPROM driver.
 * @details Clears WREN and NVMIF and stores the completion callback. With
 * a callback the NVM interrupt (plus PEIE) is enabled and the callback
 * runs from the ISR when a write finishes; without one the interrupt is
 * disabled and completion is polled.
 * @param callback Write-complete callback, or NULL to poll.
 * @return EPIC_OK on success.
 */
EPIC_StatusTypeDef EPIC_EEPROM_Init(void (*callback)(void));

/**
 * @brief Disable the NVM interrupt, clear WREN and NVMIF and drop the callback.
 * @return EPIC_OK on success.
 */
EPIC_StatusTypeDef EPIC_EEPROM_DeInit(void);

/**
 * @brief Read one byte.
 * @details Waits for a write in progress first, since NVMADR is in use
 * until WR clears, then loads the address and sets RD. A previous write's
 * WRERR flag is left as it was. On the host simulator with
 * pic16f188xx_sim_eeprom_set_write_ticks(n > 0), advance the simulator
 * (epic_harness_tick) until EPIC_EEPROM_IsWriteComplete reports 1 before
 * calling: the wait does not advance simulated time.
 * @param addr Byte address 0..255.
 * @return The byte stored at addr.
 */
uint8_t EPIC_EEPROM_ReadByte(uint8_t addr);

/**
 * @brief Start a byte write.
 * @details Clears WRERR, loads the address and data, sets WREN and runs the
 * unlock sequence with interrupts masked. Returns as soon as the write has
 * started: WR stays set until the hardware finishes. Check
 * EPIC_EEPROM_HasWriteError once EPIC_EEPROM_IsWriteComplete reports 1.
 * @param addr Byte address 0..255.
 * @param data Byte to store.
 * @return EPIC_OK when the write started, EPIC_BUSY when a write is still in
 *         progress (nothing is touched).
 */
EPIC_StatusTypeDef EPIC_EEPROM_WriteByte(uint8_t addr, uint8_t data);

/**
 * @brief Poll whether the write finished (NVMCON1<WR> clear).
 * @details Also clears WREN once the write is over, so no write stays
 * enabled between operations.
 * @return 1 when no write is in progress, 0 while WR is set.
 */
uint8_t EPIC_EEPROM_IsWriteComplete(void);

/**
 * @brief Report whether the last write failed (NVMCON1<WRERR>).
 * @details Set by hardware when the unlock sequence was interrupted or the
 * write hit a write-protected address (CPD).
 * @return 1 when the error flag is set, 0 otherwise.
 */
uint8_t EPIC_EEPROM_HasWriteError(void);

/**
 * @brief NVM interrupt handler: clears NVMIF, then runs the callback if set.
 * @details Called from epic_dispatch_all_irqs only when NVMIF and NVMIE are
 * both set. Keeps no cstack autos (ISR bank rule, family MANUAL.md).
 */
void EEPROM_IRQHandler(void);

#endif /* PIC16F188XX_EEPROM_H */
