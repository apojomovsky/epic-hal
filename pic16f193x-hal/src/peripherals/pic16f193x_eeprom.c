/**
 * @file    pic16f193x_eeprom.c
 * @brief   PIC16F193X data EEPROM driver (DS41364B §23.0).
 * @details Data space only. The unlock sequence (0x55/0xAA to EECON2)
 *          is required before WR. If the §4 gate shows WREN/WR not
 *          landing on real hardware, apply the __at(0x70) scratch +
 *          inline-asm movlb fix per ARCHITECTURE.md Finding 2.
 */

#include "peripherals/pic16f193x_eeprom.h"
#include "core/pic16f193x_irq.h"

HAL_StatusTypeDef HAL_EEPROM_Init(void)
{
    return HAL_OK;
}

HAL_StatusTypeDef HAL_EEPROM_DeInit(void)
{
    PIC8_BIT_CLR(PIC8_REG8(PIC_REG_EECON1), PIC_EECON1_WREN);
    return HAL_OK;
}

uint8_t HAL_EEPROM_ReadByte(uint8_t addr)
{
    PIC8_REG8(PIC_REG_EEADRL) = addr;
    PIC8_REG8(PIC_REG_EEADRH) = 0x00U;
    PIC8_BIT_SET(PIC8_REG8(PIC_REG_EECON1), PIC_EECON1_RD);
    return PIC8_REG8(PIC_REG_EEDATL);
}

HAL_StatusTypeDef HAL_EEPROM_WriteByte(uint8_t addr, uint8_t data)
{
    PIC8_REG8(PIC_REG_EEADRL) = addr;
    PIC8_REG8(PIC_REG_EEADRH) = 0x00U;
    PIC8_REG8(PIC_REG_EEDATL) = data;
    PIC8_BIT_SET(PIC8_REG8(PIC_REG_EECON1), PIC_EECON1_WREN);

    uint8_t gie = HAL_IRQ_Disable();
    PIC8_REG8(PIC_REG_EECON2) = 0x55U;
    PIC8_REG8(PIC_REG_EECON2) = 0xAAU;
    PIC8_BIT_SET(PIC8_REG8(PIC_REG_EECON1), PIC_EECON1_WR);
    HAL_IRQ_Restore(gie);

    while (PIC8_REG8(PIC_REG_EECON1) & PIC_EECON1_WR) { }
    PIC8_BIT_CLR(PIC8_REG8(PIC_REG_EECON1), PIC_EECON1_WREN);

    if (PIC8_REG8(PIC_REG_EECON1) & PIC_EECON1_WRERR) return HAL_ERROR;
    return HAL_OK;
}

uint8_t HAL_EEPROM_IsWriteComplete(void)
{
    return (PIC8_REG8(PIC_REG_EECON1) & PIC_EECON1_WR) ? 0U : 1U;
}

uint8_t HAL_EEPROM_HasWriteError(void)
{
    return (PIC8_REG8(PIC_REG_EECON1) & PIC_EECON1_WRERR) ? 1U : 0U;
}

void EEPROM_IRQHandler(void) {}
