/** PIC16F193X MSSP driver implementation, SPI master only (DS41364B MSSP chapter). */

#include "peripherals/pic16f193x_ssp.h"
#include "core/pic16f193x_irq.h"

EPIC_StatusTypeDef EPIC_SSP_Init(const SSP_HandleTypeDef *h)
{
    if (!h) return EPIC_INVALID;
    if ((uint8_t)h->Mode > 0x03U) return EPIC_INVALID;

    uint8_t stat = 0U;
    if (h->ClockEdge == SSP_SPI_CKE_IDLE_ACTIVE) stat |= PIC_SSPSTAT_CKE;
    if (h->SamplePhase == SSP_SPI_SMP_END) stat |= PIC_SSPSTAT_SMP;
    EPIC_REG8(PIC_REG_SSPSTAT) = stat;

    uint8_t con1 = (uint8_t)h->Mode & PIC_SSPCON1_SSPM_MASK;
    if (h->ClockPolarity == SSP_SPI_CKP_IDLE_HIGH) con1 |= PIC_SSPCON1_CKP;
    con1 |= PIC_SSPCON1_SSPEN;
    EPIC_REG8(PIC_REG_SSPCON1) = con1;

    return EPIC_OK;
}

EPIC_StatusTypeDef EPIC_SSP_DeInit(void)
{
    EPIC_REG8(PIC_REG_SSPCON1) = 0x00U;
    EPIC_REG8(PIC_REG_SSPSTAT) = 0x00U;
    return EPIC_OK;
}

uint16_t EPIC_SSP_WriteByte(uint8_t data)
{
    if (EPIC_REG8(PIC_REG_SSPCON1) & PIC_SSPCON1_WCOL) return 0xFFFFU;
    EPIC_REG8(PIC_REG_SSPBUF) = data;
    if (EPIC_REG8(PIC_REG_SSPCON1) & PIC_SSPCON1_WCOL) return 0xFFFFU;
    return (uint16_t)data;
}

uint8_t EPIC_SSP_ReadByte(void)
{
    return EPIC_REG8(PIC_REG_SSPBUF);
}

uint8_t EPIC_SSP_IsBufferFull(void)
{
    return (EPIC_REG8(PIC_REG_SSPSTAT) & PIC_SSPSTAT_BF) ? 1U : 0U;
}

uint8_t EPIC_SSP_HasWriteCollision(void)
{
    return (EPIC_REG8(PIC_REG_SSPCON1) & PIC_SSPCON1_WCOL) ? 1U : 0U;
}

void EPIC_SSP_ClearWriteCollision(void)
{
    EPIC_BIT_CLR(EPIC_REG8(PIC_REG_SSPCON1), PIC_SSPCON1_WCOL);
}

void SSP_IRQHandler(void) {}
