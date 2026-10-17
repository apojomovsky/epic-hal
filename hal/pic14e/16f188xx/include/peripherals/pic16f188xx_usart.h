/**
 * PIC16F188xx EUSART driver: asynchronous mode, 8/9-bit frames, 8/16-bit
 * baud generator (EDC register map; matches the PIC16F193X EUSART, DS41364B).
 * Synchronous mode and auto-baud are not driven. Pins come from PPS plus
 * EPIC_GPIO_Init. API mirrors hal/pic18/18f1320 (9-bit accessors) and
 * hal/pic14e/16f193x. Full reference: MANUAL.md.
 */
#ifndef PIC16F188XX_USART_H
#define PIC16F188XX_USART_H

#include <stddef.h>
#include <stdint.h>
#include "pic16f188xx.h"

/** @brief High/low baud divisor select (TX1STA<BRGH>). */
typedef enum {
    USART_BRGH_LOW   = 0x0U,   /**< BRGH=0. */
    USART_BRGH_HIGH  = 0x1U,   /**< BRGH=1. */
} USART_BaudRateHighTypeDef;

/**
 * @brief Baud generator width (BAUD1CON<BRG16>): 8-bit uses SP1BRGL alone,
 *        16-bit uses SP1BRGH:SP1BRGL.
 */
typedef enum {
    USART_BAUDGEN_8BIT   = 0x0U,   /**< BRG16=0. */
    USART_BAUDGEN_16BIT  = 0x1U,   /**< BRG16=1. */
} USART_BaudGenTypeDef;

/** @brief Frame data width (TX1STA<TX9> and RC1STA<RX9>). */
typedef enum {
    USART_DATA_8BITS  = 0x0U,
    USART_DATA_9BITS  = 0x1U,
} USART_DataWidthTypeDef;

/**
 * @brief Compute the baud generator value for a baud rate (asynchronous).
 * @details Baud = Fosc / (D * (n + 1)) with D = 64 (BRGH=0, BRG16=0),
 * 16 (BRGH=1, BRG16=0 or BRGH=0, BRG16=1) or 4 (BRGH=1, BRG16=1). The
 * result rounds to the nearest divisor, not down.
 * @param fosc_hz System oscillator frequency in Hz.
 * @param baud Desired baud rate in bit/s.
 * @param brgh BRGH selection.
 * @param brg16 Baud generator width.
 * @return The value n, or 0xFFFF when baud or fosc_hz is 0, the divisor
 *         rounds to 0, or n does not fit the generator (255 or 65535).
 */
uint16_t USART_ComputeSPBRG(uint32_t fosc_hz, uint32_t baud,
                            USART_BaudRateHighTypeDef brgh,
                            USART_BaudGenTypeDef brg16);

/** Driver handle (Cube-style). */
typedef struct {
    USART_BaudRateHighTypeDef BaudHigh;
    USART_BaudGenTypeDef      BaudGen;
    USART_DataWidthTypeDef    DataWidth;
    uint16_t                  SPBRG;          /**< Generator value n. */
    uint8_t                   AddressDetect;  /**< ADDEN, 9-bit frames only. */
    /** @brief Optional TX callback: runs once when the transmit buffer
     *         empties after EPIC_USART_EnableTxInterrupt. */
    void (*TxCpltCallback)(void);
    /** @brief Optional RX callback: runs with each received byte; arming
     *         it enables the receive interrupt in Init. */
    void (*RxCpltCallback)(uint8_t data);
} USART_HandleTypeDef;

#define USART_HANDLE_DEFAULT {                                            \
    .BaudHigh       = USART_BRGH_HIGH,                                     \
    .BaudGen        = USART_BAUDGEN_8BIT,                                  \
    .DataWidth      = USART_DATA_8BITS,                                    \
    .SPBRG          = 0U,                                                  \
    .AddressDetect  = 0U,                                                  \
    .TxCpltCallback = NULL,                                                \
    .RxCpltCallback = NULL,                                                \
}

/**
 * @brief Configure the EUSART from the handle and enable it.
 * @details Loads the generator, TX1STA (BRGH, TX9, TXEN) and BAUD1CON
 * (BRG16), then RC1STA (SPEN, RX9, CREN, ADDEN) last so the port goes live
 * fully configured. TXIE stays off: TXIF is set whenever the buffer is
 * empty, so a callback fires only after EPIC_USART_EnableTxInterrupt.
 * RCIE is enabled only with an RX callback. AddressDetect is ignored unless
 * DataWidth is 9-bit.
 * @param h Handle with baud, width and callbacks.
 * @return EPIC_OK on success, EPIC_INVALID for a NULL or out-of-range handle
 *         (including SPBRG above 255 on the 8-bit generator).
 */
EPIC_StatusTypeDef EPIC_USART_Init(const USART_HandleTypeDef *h);

/**
 * @brief Disable the EUSART and restore the POR state of its registers.
 * @details Masks both interrupts, drops both callbacks, and writes
 * RC1STA 0x00, TX1STA 0x02, BAUD1CON 0x00 and the generator to 0.
 * @return EPIC_OK.
 */
EPIC_StatusTypeDef EPIC_USART_DeInit(void);

/**
 * @brief Queue one byte for transmission by writing TX1REG.
 * @details Writing TX1REG clears TXIF in hardware. In 9-bit mode call
 * EPIC_USART_SetTX9D first.
 * @param data Byte to send.
 */
void EPIC_USART_Transmit(uint8_t data);

/**
 * @brief Set the ninth transmit bit (TX1STA<TX9D>) for the next byte.
 * @param bit9 Nonzero sets the bit, zero clears it.
 */
void EPIC_USART_SetTX9D(uint8_t bit9);

/**
 * @brief Report whether the transmit buffer can take a byte (PIR3<TXIF>).
 * @return 1 when TX1REG is empty, 0 otherwise.
 */
uint8_t EPIC_USART_IsTxBufferEmpty(void);

/**
 * @brief Report whether the transmit shift register is empty (TX1STA<TRMT>).
 * @return 1 when the last frame has left the pin, 0 otherwise.
 */
uint8_t EPIC_USART_IsTxShiftRegisterEmpty(void);

/**
 * @brief Arm the one-shot transmit interrupt (sets PIE3<TXIE> and PEIE).
 * @details The handler clears TXIE before it runs TxCpltCallback, because
 * TXIF cannot be cleared by software. Call again from the callback to keep
 * feeding bytes.
 */
void EPIC_USART_EnableTxInterrupt(void);

/**
 * @brief Report whether a received byte is waiting (PIR3<RCIF>).
 * @return 1 when the receive FIFO holds a byte, 0 otherwise.
 */
uint8_t EPIC_USART_IsRxReady(void);

/**
 * @brief Read the oldest received byte from RC1REG.
 * @details Reading pops the two-deep receive FIFO. In 9-bit mode read
 * EPIC_USART_GetRX9D first, because it belongs to the byte at the head.
 * @return The byte.
 */
uint8_t EPIC_USART_Receive(void);

/**
 * @brief Read the ninth bit of the byte at the head of the FIFO (RC1STA<RX9D>).
 * @details Valid only before the matching EPIC_USART_Receive.
 * @return 0 or 1.
 */
uint8_t EPIC_USART_GetRX9D(void);

/**
 * @brief Ninth bit of the byte the receive interrupt last delivered.
 * @details The handler latches RX9D before it reads RC1REG, so a callback
 * reads the bit here, not through EPIC_USART_GetRX9D.
 * @return 0 or 1.
 */
uint8_t EPIC_USART_GetLastRX9D(void);

/**
 * @brief Report a receive overrun (RC1STA<OERR>).
 * @return 1 on overrun, 0 otherwise.
 */
uint8_t EPIC_USART_HasOverrunError(void);

/**
 * @brief Clear an overrun by dropping and re-setting RC1STA<CREN>.
 */
void EPIC_USART_ClearOverrun(void);

/**
 * @brief Report a framing error on the byte at the FIFO head (RC1STA<FERR>).
 * @return 1 on framing error, 0 otherwise.
 */
uint8_t EPIC_USART_HasFramingError(void);

/**
 * @brief Transmit interrupt handler: masks TXIE, then runs the TX callback.
 * @details Called from epic_dispatch_all_irqs only when TXIF and TXIE are
 * both set. Keeps no cstack autos (ISR bank rule, family MANUAL.md).
 */
void USART_TX_IRQHandler(void);

/**
 * @brief Receive interrupt handler: reads RC1REG and runs the RX callback.
 * @details Called from epic_dispatch_all_irqs only when RCIF and RCIE are
 * both set. Latches RX9D first, then the byte, in file-scope statics (ISR
 * bank rule, family MANUAL.md); the RC1REG read clears RCIF.
 */
void USART_RX_IRQHandler(void);

#endif
