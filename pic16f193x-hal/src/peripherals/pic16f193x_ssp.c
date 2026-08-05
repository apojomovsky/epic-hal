/**
 * @file    pic16f193x_ssp.c
 * @brief   PIC16F193X MSSP driver implementation, SPI master only
 *          (DS41364B MSSP chapter).
 */

#include "peripherals/pic16f193x_ssp.h"
#include "core/pic16f193x_irq.h"

HAL_StatusTypeDef HAL_SSP_Init(const SSP_HandleTypeDef *h)
{
    if (!h) return HAL_INVALID;
    if ((uint8_t)h->Mode > 0x03U) return HAL_INVALID;

    uint8_t stat = 0U;
    if (h->ClockEdge == SSP_SPI_CKE_IDLE_ACTIVE) stat |= PIC_SSPSTAT_CKE;
    if (h->SamplePhase == SSP_SPI_SMP_END) stat |= PIC_SSPSTAT_SMP;
    PIC8_REG8(PIC_REG_SSPSTAT) = stat;

    uint8_t con1 = (uint8_t)h->Mode & PIC_SSPCON1_SSPM_MASK;
    if (h->ClockPolarity == SSP_SPI_CKP_IDLE_HIGH) con1 |= PIC_SSPCON1_CKP;
    con1 |= PIC_SSPCON1_SSPEN;
    PIC8_REG8(PIC_REG_SSPCON1) = con1;

    return HAL_OK;
}

HAL_StatusTypeDef HAL_SSP_DeInit(void)
{
    PIC8_REG8(PIC_REG_SSPCON1) = 0x00U;
    PIC8_REG8(PIC_REG_SSPSTAT) = 0x00U;
    return HAL_OK;
}

uint16_t HAL_SSP_WriteByte(uint8_t data)
{
    if (PIC8_REG8(PIC_REG_SSPCON1) & PIC_SSPCON1_WCOL) return 0xFFFFU;
    PIC8_REG8(PIC_REG_SSPBUF) = data;
    if (PIC8_REG8(PIC_REG_SSPCON1) & PIC_SSPCON1_WCOL) return 0xFFFFU;
    return (uint16_t)data;
}

uint8_t HAL_SSP_ReadByte(void)
{
    return PIC8_REG8(PIC_REG_SSPBUF);
}

uint8_t HAL_SSP_IsBufferFull(void)
{
    return (PIC8_REG8(PIC_REG_SSPSTAT) & PIC_SSPSTAT_BF) ? 1U : 0U;
}

uint8_t HAL_SSP_HasWriteCollision(void)
{
    return (PIC8_REG8(PIC_REG_SSPCON1) & PIC_SSPCON1_WCOL) ? 1U : 0U;
}

void HAL_SSP_ClearWriteCollision(void)
{
    PIC8_BIT_CLR(PIC8_REG8(PIC_REG_SSPCON1), PIC_SSPCON1_WCOL);
}

void SSP_IRQHandler(void) {}
