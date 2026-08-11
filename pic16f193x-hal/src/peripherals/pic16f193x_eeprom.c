/**
 * PIC16F193X data EEPROM driver (DS41364B §23.0). Data space only. The
 * unlock sequence (0x55/0xAA to EECON2) is required before WR.
 */

#include "peripherals/pic16f193x_eeprom.h"
#include "core/pic16f193x_irq.h"

EPIC_StatusTypeDef EPIC_EEPROM_Init(void)
{
    return EPIC_OK;
}

EPIC_StatusTypeDef EPIC_EEPROM_DeInit(void)
{
    EPIC_BIT_CLR(EPIC_REG8(PIC_REG_EECON1), PIC_EECON1_WREN);
    return EPIC_OK;
}

uint8_t EPIC_EEPROM_ReadByte(uint8_t addr)
{
    EPIC_REG8(PIC_REG_EEADRL) = addr;
    EPIC_REG8(PIC_REG_EEADRH) = 0x00U;
    EPIC_BIT_SET(EPIC_REG8(PIC_REG_EECON1), PIC_EECON1_RD);
    return EPIC_REG8(PIC_REG_EEDATL);
}

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

uint8_t EPIC_EEPROM_IsWriteComplete(void)
{
    return (EPIC_REG8(PIC_REG_EECON1) & PIC_EECON1_WR) ? 0U : 1U;
}

uint8_t EPIC_EEPROM_HasWriteError(void)
{
    return (EPIC_REG8(PIC_REG_EECON1) & PIC_EECON1_WRERR) ? 1U : 0U;
}

void EEPROM_IRQHandler(void) {}
