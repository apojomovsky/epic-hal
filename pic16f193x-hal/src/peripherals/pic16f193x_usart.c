/**
 * @file    pic16f193x_usart.c
 * @brief   PIC16F193X EUSART driver implementation (DS41364B §23.0).
 * @details Async 8-bit mode only. 9-bit, auto-baud, sync deferred.
 */

#include "peripherals/pic16f193x_usart.h"
#include "core/pic16f193x_irq.h"

uint16_t USART_ComputeSPBRG(uint32_t fosc_hz, uint32_t baud,
                            USART_BaudRateHighTypeDef brgh)
{
    if (baud == 0U) return 0xFFFFU;
    uint32_t divisor = (brgh == USART_BRGH_HIGH) ? 16U : 64U;
    uint32_t x = (fosc_hz / (divisor * baud));
    if (x == 0U) return 0xFFFFU;
    x -= 1U;
    if (x > 255U) return 0xFFFFU;
    return (uint16_t)x;
}

EPIC_StatusTypeDef EPIC_USART_Init(const USART_HandleTypeDef *h)
{
    if (!h) return EPIC_INVALID;

    PIC8_REG8(PIC_REG_SPBRGL) = h->SPBRG;
    PIC8_REG8(PIC_REG_SPBRGH) = 0x00U;

    uint8_t txsta = PIC_TXSTA_TXEN;
    if (h->BaudHigh == USART_BRGH_HIGH) txsta |= PIC_TXSTA_BRGH;
    PIC8_REG8(PIC_REG_TXSTA) = txsta;

    PIC8_REG8(PIC_REG_RCSTA) = (uint8_t)(PIC_RCSTA_SPEN | PIC_RCSTA_CREN);
    PIC8_REG8(PIC_REG_BAUDCON) = 0x00U;

    return EPIC_OK;
}

EPIC_StatusTypeDef EPIC_USART_DeInit(void)
{
    PIC8_REG8(PIC_REG_TXSTA) = PIC_TXSTA_POR_VALUE;
    PIC8_REG8(PIC_REG_RCSTA) = PIC_RCSTA_POR_VALUE;
    PIC8_REG8(PIC_REG_BAUDCON) = PIC_BAUDCON_POR_VALUE;
    return EPIC_OK;
}

void EPIC_USART_Transmit(uint8_t data)
{
    PIC8_REG8(PIC_REG_TXREG) = data;
}

uint8_t EPIC_USART_IsTxShiftRegisterEmpty(void)
{
    return (PIC8_REG8(PIC_REG_TXSTA) & PIC_TXSTA_TRMT) ? 1U : 0U;
}

uint8_t EPIC_USART_Receive(void)
{
    return PIC8_REG8(PIC_REG_RCREG);
}

uint8_t EPIC_USART_HasOverrunError(void)
{
    return (PIC8_REG8(PIC_REG_RCSTA) & PIC_RCSTA_OERR) ? 1U : 0U;
}

void USART_TX_IRQHandler(void) {}
void USART_RX_IRQHandler(void) {}
