/* USART driver, async + sync master/slave. Source: DS39582B §10.0,
 * §10.1 (BRG); full reference: MANUAL.md §14. One USART instance on
 * this family; 8/9-bit data, address-detect mode via ADDEN. */

#ifndef PIC16F87XA_USART_H
#define PIC16F87XA_USART_H

#include "pic16f87xa.h"
#include "pic16f87xa_sfr.h"

/**
 * @brief USART mode (TXSTA<SYNC>, DS39582B Register 10-1).
 */
typedef enum {
    USART_MODE_ASYNCHRONOUS  = 0x0U,   /**< SYNC = 0. */
    USART_MODE_SYNCHRONOUS   = 0x1U,   /**< SYNC = 1. */
} USART_ModeTypeDef;

/**
 * @brief Synchronous clock source (TXSTA<CSRC>, Register 10-1).
 *        Only meaningful in synchronous mode; ignored otherwise.
 */
typedef enum {
    USART_CLOCK_SLAVE        = 0x0U,   /**< CSRC = 0, clock from external. */
    USART_CLOCK_MASTER       = 0x1U,   /**< CSRC = 1, clock from BRG. */
} USART_ClockSourceTypeDef;

/**
 * @brief High/low baud-rate divisor (TXSTA<BRGH>, DS39582B §10.1).
 *        Async mode only. Synchronous mode always uses /4.
 */
typedef enum {
    USART_BRGH_LOW           = 0x0U,   /**< BRGH = 0, divisor 64. */
    USART_BRGH_HIGH          = 0x1U,   /**< BRGH = 1, divisor 16. */
} USART_BaudRateHighTypeDef;

/**
 * @brief Receive / transmit data width (RCSTA<RX9>, TXSTA<TX9>).
 */
typedef enum {
    USART_DATA_8BITS         = 0x0U,
    USART_DATA_9BITS         = 0x1U,
} USART_DataWidthTypeDef;

/**
 * @brief Compute the SPBRG value for a desired baud rate.
 *
 *   Async: rate = FOSC / (64 × (X+1))  (BRGH=0)
 *          rate = FOSC / (16 × (X+1))  (BRGH=1)
 *   Sync:  rate = FOSC / (4  × (X+1))
 *
 * Returns 0..255 (the SPBRG range), or 0xFFFF if the requested baud
 * rate is unattainable (X would have to exceed 255).
 * @param fosc_hz the oscillator frequency in Hz.
 * @param baud the desired baud rate in bits/s.
 * @param mode USART_MODE_ASYNCHRONOUS or USART_MODE_SYNCHRONOUS.
 * @param brgh USART_BRGH_LOW or USART_BRGH_HIGH (async only).
 * @return the SPBRG value 0..255, or 0xFFFF if unattainable.
 */
uint16_t USART_ComputeSPBRG(uint32_t fosc_hz, uint32_t baud,
                            USART_ModeTypeDef mode,
                            USART_BaudRateHighTypeDef brgh);

/** Driver handle (Cube-style). */
typedef struct {
    USART_ModeTypeDef          Mode;
    USART_ClockSourceTypeDef   ClockSource;
    USART_BaudRateHighTypeDef  BaudHigh;
    USART_DataWidthTypeDef     DataWidth;
    uint8_t                    SPBRG;        /**< 0..255, pre-computed. */
    /** @brief  Optional TX-complete callback (fires on TXIF). */
    void (*TxCpltCallback)(void);
    /** @brief  Optional RX-complete callback (fires on RCIF). */
    void (*RxCpltCallback)(uint8_t data);
} USART_HandleTypeDef;

#define USART_HANDLE_DEFAULT {                                          \
    .Mode          = USART_MODE_ASYNCHRONOUS,                           \
    .ClockSource   = USART_CLOCK_MASTER,                                \
    .BaudHigh      = USART_BRGH_HIGH,                                   \
    .DataWidth     = USART_DATA_8BITS,                                  \
    .SPBRG         = 0,                                                 \
    .TxCpltCallback = NULL,                                            \
    .RxCpltCallback = NULL,                                            \
}

/* init / deinit. */

/**
 * @brief  Initialize the USART with the given handle. Programs TXSTA,
 *         RCSTA, SPBRG and the interrupt enables for the callbacks.
 * @param h handle with Mode, ClockSource, BaudHigh, DataWidth, SPBRG,
 *        TxCpltCallback, RxCpltCallback.
 * @return EPIC_OK on success, EPIC_ERROR if `h` is NULL.
 */
EPIC_StatusTypeDef EPIC_USART_Init(const USART_HandleTypeDef *h);

/**
 * @brief  De-initialize the USART. Disables the module and returns
 *         TXSTA/RCSTA to reset.
 * @return EPIC_OK on success.
 */
EPIC_StatusTypeDef EPIC_USART_DeInit(void);

/* transmit. */

/**
 * @brief  Write one byte to TXREG. The write:
 *    - loads the byte into the TSR if it's empty (back-to-back transfer),
 *    - else parks it in TXREG until TSR drains,
 *    - sets TXIF = 0 (TXIF is read-only, cleared on TXREG write).
 *
 * @note   TXIF is NOT cleared by reading, only by writing TXREG.
 *         DS39582B §10.2.1.
 * @param data the byte to transmit.
 */
void EPIC_USART_Transmit(uint8_t data);

/**
 * @brief Read the 9th bit (TX9D) just transmitted.
 * @return the TX9D bit value.
 */
uint8_t EPIC_USART_GetTX9D(void);

/**
 * @brief Set the 9th bit to send NEXT. Must be set BEFORE writing TXREG.
 * @param bit9 the 9th data bit value (0 or 1).
 */
void EPIC_USART_SetTX9D(uint8_t bit9);

/**
 * @brief Returns 1 if the TSR is empty (TRMT = 1).
 * @return 1 if the transmit shift register is empty, 0 otherwise.
 */
uint8_t EPIC_USART_IsTxShiftRegisterEmpty(void);

/* receive. */

/**
 * @brief  Read the latest byte from RCREG. Reading:
 *    - clears RCIF,
 *    - advances the 2-deep FIFO.
 * @return the received byte.
 */
uint8_t EPIC_USART_Receive(void);

/**
 * @brief Read RX9D, the 9th bit of the most recently received byte.
 * @return the RX9D bit value.
 */
uint8_t EPIC_USART_GetRX9D(void);

/* interrupts. */

/**
 * @brief Weak USART RX ISR, override in user code.
 */
void USART_RX_IRQHandler(void) EPIC_WEAK;
/**
 * @brief Weak USART TX ISR, override in user code.
 */
void USART_TX_IRQHandler(void) EPIC_WEAK;

#endif /* PIC16F87XA_USART_H */
