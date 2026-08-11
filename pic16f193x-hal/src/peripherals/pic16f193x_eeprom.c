/**
 * PIC16F193X data EEPROM driver (DS41364B §23.0). Data space only. The
 * unlock sequence (0x55/0xAA to EECON2) is required before WR.
 */

#include "peripherals/pic16f193x_eeprom.h"
#include "core/pic16f193x_irq.h"

/**
 * @brief Initialize the data EEPROM module. No setup required on this
 *        device, so this is a no-op success.
 * @return EPIC_OK on success
 */
EPIC_StatusTypeDef EPIC_EEPROM_Init(void)
{
    return EPIC_OK;
}

/**
 * @brief Deinitialize the data EEPROM by clearing the write-enable bit.
 * @return EPIC_OK on success
 */
EPIC_StatusTypeDef EPIC_EEPROM_DeInit(void)
{
    EPIC_BIT_CLR(EPIC_REG8(PIC_REG_EECON1), PIC_EECON1_WREN);
    return EPIC_OK;
}

/**
 * @brief Read one byte from data EEPROM at the given address.
 * @param addr EEPROM address (0-255)
 * @return the byte stored at `addr`
 */
uint8_t EPIC_EEPROM_ReadByte(uint8_t addr)
{
    EPIC_REG8(PIC_REG_EEADRL) = addr;
    EPIC_REG8(PIC_REG_EEADRH) = 0x00U;
    EPIC_BIT_SET(EPIC_REG8(PIC_REG_EECON1), PIC_EECON1_RD);
    return EPIC_REG8(PIC_REG_EEDATL);
}

/**
 * @brief Write one byte to data EEPROM, running the 0x55/0xAA unlock
 *        sequence with interrupts masked and waiting for completion.
 * @param addr EEPROM address (0-255)
 * @param data byte to write
 * @return EPIC_OK on success, EPIC_ERROR if the write error flag is set
 */
EPIC_StatusTypeDef EPIC_EEPROM_WriteByte(uint8_t addr, uint8_t data)
{
    EPIC_REG8(PIC_REG_EEADRL) = addr;
    EPIC_REG8(PIC_REG_EEADRH) = 0x00U;
    EPIC_REG8(PIC_REG_EEDATL) = data;
    EPIC_BIT_SET(EPIC_REG8(PIC_REG_EECON1), PIC_EECON1_WREN);

    uint8_t gie = EPIC_IRQ_Disable();
    EPIC_REG8(PIC_REG_EECON2) = 0x55U;
    EPIC_REG8(PIC_REG_EECON2) = 0xAAU;
    EPIC_BIT_SET(EPIC_REG8(PIC_REG_EECON1), PIC_EECON1_WR);
    EPIC_IRQ_Restore(gie);

    while (EPIC_REG8(PIC_REG_EECON1) & PIC_EECON1_WR) { }
    EPIC_BIT_CLR(EPIC_REG8(PIC_REG_EECON1), PIC_EECON1_WREN);

    if (EPIC_REG8(PIC_REG_EECON1) & PIC_EECON1_WRERR) return EPIC_ERROR;
    return EPIC_OK;
}

/**
 * @brief Poll whether an EEPROM write has finished.
 * @return 1 if no write is in progress, 0 while a write is running
 */
uint8_t EPIC_EEPROM_IsWriteComplete(void)
{
    return (EPIC_REG8(PIC_REG_EECON1) & PIC_EECON1_WR) ? 0U : 1U;
}

/**
 * @brief Poll whether the last EEPROM write ended in an error.
 * @return 1 if the write error flag is set, 0 otherwise
 */
uint8_t EPIC_EEPROM_HasWriteError(void)
{
    return (EPIC_REG8(PIC_REG_EECON1) & PIC_EECON1_WRERR) ? 1U : 0U;
}

/**
 * @brief EEPROM interrupt handler (weak, override in user code).
 */
void EEPROM_IRQHandler(void) {}
