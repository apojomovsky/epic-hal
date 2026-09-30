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

/**
 * @brief  Compute the SPBRG value for a desired baud rate, BRG16=0
 *         (8-bit SPBRGL only) this phase.
 *
 * @param  fosc_hz  system oscillator frequency in Hz
 * @param  baud     desired baud rate in bit/s
 * @param  brgh     USART_BRGH_LOW (divisor 64) or USART_BRGH_HIGH
 *                  (divisor 16)
 * @return The SPBRG value 0..255, or 0xFFFF if it does not fit the
 *         8-bit register (hard rejection, not silent truncation)
 */
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

/**
 * @brief  Configure the EUSART in asynchronous 8-bit mode from the
 *         handle: load SPBRGL, enable the transmitter (TXSTA<TXEN> and
 *         BRGH) and the receiver (RCSTA<SPEN,CREN>), BAUDCON cleared.
 *
 * @param  h  handle with BaudHigh and SPBRG values
 * @return EPIC_OK on success, EPIC_INVALID if `h` is NULL
 */
EPIC_StatusTypeDef EPIC_USART_Init(const USART_HandleTypeDef *h);

/**
 * @brief  Restore TXSTA, RCSTA and BAUDCON to their power-on reset
 *         values, disabling the EUSART.
 *
 * @return EPIC_OK
 */
EPIC_StatusTypeDef EPIC_USART_DeInit(void);

/**
 * @brief  Transmit one byte by writing it to TXREG.
 *
 * @param  data  byte to transmit
 */
void EPIC_USART_Transmit(uint8_t data);

/**
 * @brief  Report whether the transmit shift register is empty
 *         (TXSTA<TRMT>).
 *
 * @return 1 if the shift register is empty, 0 otherwise
 */
uint8_t EPIC_USART_IsTxShiftRegisterEmpty(void);

/**
 * @brief  Read the received byte from RCREG.
 *
 * @return The byte in the receive buffer
 */
uint8_t EPIC_USART_Receive(void);

/**
 * @brief  Report whether a receive overrun occurred (RCSTA<OERR>).
 *
 * @return 1 on overrun error, 0 otherwise
 */
uint8_t EPIC_USART_HasOverrunError(void);

/** @brief Weak USART TX interrupt handler; override in user code. */
void USART_TX_IRQHandler(void) EPIC_WEAK;

/** @brief Weak USART RX interrupt handler; override in user code. */
void USART_RX_IRQHandler(void) EPIC_WEAK;

#endif /* PIC16F193X_USART_H */
