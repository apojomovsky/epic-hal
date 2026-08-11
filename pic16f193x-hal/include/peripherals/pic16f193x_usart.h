/**
 * PIC16F193X EUSART driver (DS41364B §23.0, Registers 23-2/23-3/23-4),
 * asynchronous 8-bit mode only this phase (9-bit, auto-baud, synchronous
 * deferred; auto-baud specifically due to a real DS80000479 SPBRG
 * errata, not convenience). API shape mirrors pic16f87xa_usart.h. Full
 * reference: MANUAL.md.
 */
#ifndef PIC16F193X_USART_H
#define PIC16F193X_USART_H

#include "pic16f193x.h"
#include "pic16f193x_sfr.h"

typedef enum {
    USART_BRGH_LOW  = 0x0U,   /**< BRGH=0, BRG16=0: divisor 64. */
    USART_BRGH_HIGH = 0x1U,   /**< BRGH=1, BRG16=0: divisor 16. */
} USART_BaudRateHighTypeDef;

/** Compute the SPBRG value for a desired baud rate, BRG16=0 (8-bit
 *  SPBRGL only) this phase. Returns 0..255, or 0xFFFF if it doesn't
 *  fit the 8-bit register (hard rejection, not silent truncation). */
uint16_t USART_ComputeSPBRG(uint32_t fosc_hz, uint32_t baud,
                            USART_BaudRateHighTypeDef brgh);

typedef struct {
    USART_BaudRateHighTypeDef BaudHigh;
    uint8_t                   SPBRG;
    void (*TxCpltCallback)(void);
    void (*RxCpltCallback)(uint8_t data);
} USART_HandleTypeDef;

#define USART_HANDLE_DEFAULT { \
    .BaudHigh = USART_BRGH_HIGH, .SPBRG = 0U, \
    .TxCpltCallback = 0, .RxCpltCallback = 0, \
}

EPIC_StatusTypeDef EPIC_USART_Init(const USART_HandleTypeDef *h);
EPIC_StatusTypeDef EPIC_USART_DeInit(void);
void EPIC_USART_Transmit(uint8_t data);
uint8_t EPIC_USART_IsTxShiftRegisterEmpty(void);
uint8_t EPIC_USART_Receive(void);
uint8_t EPIC_USART_HasOverrunError(void);

void USART_TX_IRQHandler(void) EPIC_WEAK;
void USART_RX_IRQHandler(void) EPIC_WEAK;

#endif /* PIC16F193X_USART_H */
