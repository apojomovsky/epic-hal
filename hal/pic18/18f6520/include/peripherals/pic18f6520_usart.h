/*
 * EUSART driver, async + sync master/slave (DS39609B §18.0, DS39661
 * §18.0 for the ECAN quads). Instance selector over the two modules
 * (single EUSART on ECAN quads: INSTANCE_1 only); 8-bit BRG, no
 * BAUDCON/SPBRGH. RMW uses split read+write: XC8 cannot lower compound
 * assignment on a volatile cast-lvalue.
 */

#ifndef PIC18F6520_USART_H
#define PIC18F6520_USART_H

#include "pic18f6520_hal.h"
#include "pic18f6520_sfr.h"

/**
 * @brief Which EUSART module a handle refers to.
 */
typedef enum {
    USART_INSTANCE_1 = 1,   /**< EUSART1: RCSTA1/TXSTA1/TXREG1/RCREG1/SPBRG1 (0xFAB-0xFAF). */
    USART_INSTANCE_2 = 2,   /**< EUSART2: RCSTA2/TXSTA2/TXREG2/RCREG2/SPBRG2 (0xF6B-0xF6F). Absent on ECAN quads. */
} USART_InstanceTypeDef;

/**
 * @brief USART mode (TXSTAx<SYNC>, DS39609B Register 18-1).
 */
typedef enum {
    USART_MODE_ASYNCHRONOUS  = 0x0U,   /**< SYNC = 0. */
    USART_MODE_SYNCHRONOUS   = 0x1U,   /**< SYNC = 1. */
} USART_ModeTypeDef;

/**
 * @brief Synchronous clock source (TXSTAx<CSRC>, Register 18-1).
 *        Only meaningful in synchronous mode; ignored otherwise.
 */
typedef enum {
    USART_CLOCK_SLAVE        = 0x0U,   /**< CSRC = 0, clock from external. */
    USART_CLOCK_MASTER       = 0x1U,   /**< CSRC = 1, clock from BRG. */
} USART_ClockSourceTypeDef;

/**
 * @brief High/low baud-rate divisor (TXSTAx<BRGH>, DS39609B §18.1).
 *        Combined with the 8-bit BRG per Table 18-1.
 */
typedef enum {
    USART_BRGH_LOW           = 0x0U,   /**< BRGH = 0. */
    USART_BRGH_HIGH          = 0x1U,   /**< BRGH = 1. */
} USART_BaudRateHighTypeDef;

/**
 * @brief Receive / transmit data width (RCSTAx<RX9>, TXSTAx<TX9>).
 */
typedef enum {
    USART_DATA_8BITS         = 0x0U,
    USART_DATA_9BITS         = 0x1U,
} USART_DataWidthTypeDef;

/**
 * @brief Compute the BRG divisor for a desired baud rate.
 *
 *   Async (Table 18-1, 8-bit BRG only, X = value in SPBRGx 0..255):
 *     BRGH=0: rate = FOSC / (64 x (X+1))
 *     BRGH=1: rate = FOSC / (16 x (X+1))
 *   Sync:        rate = FOSC / (4  x (X+1))
 *
 * Returns 0..255, or 0xFFFF if the requested baud rate is unattainable
 * (X would exceed the 8-bit SPBRGx range).
 * @param fosc_hz the system oscillator frequency in Hz.
 * @param baud the desired baud rate in bits per second.
 * @param mode the USART mode (async or sync).
 * @param brgh the high/low baud-rate divisor select.
 * @return the BRG divisor (0..255), or 0xFFFF if unattainable.
 */
uint16_t USART_ComputeSPBRG(uint32_t fosc_hz, uint32_t baud,
                            USART_ModeTypeDef mode,
                            USART_BaudRateHighTypeDef brgh);

/** Driver handle (Cube-style). One handle per EUSART module. */
typedef struct {
    USART_InstanceTypeDef      Instance;
    USART_ModeTypeDef          Mode;
    USART_ClockSourceTypeDef   ClockSource;     /**< Sync only. */
    USART_BaudRateHighTypeDef  BaudHigh;
    USART_DataWidthTypeDef     DataWidth;
    uint8_t                    SPBRG;            /**< 0..255, BRG divisor. */
    uint8_t                    AddressDetect;    /**< ADDEN: 9-bit address detect. */
    /** @brief  Optional TX-complete callback (fires on TXIF). */
    void (*TxCpltCallback)(void);
    /** @brief  Optional RX-complete callback (fires on RCIF). */
    void (*RxCpltCallback)(uint8_t data);
} USART_HandleTypeDef;

#define USART_HANDLE_DEFAULT {                                          \
    .Instance        = USART_INSTANCE_1,                                \
    .Mode            = USART_MODE_ASYNCHRONOUS,                         \
    .ClockSource     = USART_CLOCK_MASTER,                              \
    .BaudHigh        = USART_BRGH_HIGH,                                 \
    .DataWidth       = USART_DATA_8BITS,                                \
    .SPBRG           = 0,                                               \
    .AddressDetect   = 0,                                               \
    .TxCpltCallback  = NULL,                                            \
    .RxCpltCallback  = NULL,                                            \
}

/**
 * @brief  Configure an EUSART module: mode, baud-rate generator (SPBRGx),
 *         data width and address-detect, then enable the
 *         transmitter/receiver and optional callbacks.
 * @param h the USART handle describing the desired configuration.
 * @return EPIC_OK on success, EPIC_INVALID on bad handle/instance.
 */
EPIC_StatusTypeDef EPIC_USART_Init(const USART_HandleTypeDef *h);

/**
 * @brief  Disable an EUSART module (transmitter, receiver and
 *         interrupts) and restore its registers to POR.
 * @param inst which EUSART module to de-initialize.
 * @return EPIC_OK on success, EPIC_INVALID for an unknown instance.
 */
EPIC_StatusTypeDef EPIC_USART_DeInit(USART_InstanceTypeDef inst);

/**
 * @brief  Write one byte to TXREGx. The write:
 *    - loads the byte into the TSR if it's empty (back-to-back transfer),
 *    - else parks it in TXREGx until TSR drains,
 *    - clears TXIF (TXIF is read-only, cleared on TXREGx write).
 *
 * @note   TXIF is NOT cleared by reading, only by writing TXREGx.
 *         DS39609B §18.2.1.
 * @param inst which EUSART module to transmit on.
 * @param data the byte to transmit.
 */
void EPIC_USART_Transmit(USART_InstanceTypeDef inst, uint8_t data);

/**
 * @brief Read the 9th bit (TX9D) just transmitted.
 * @param inst which EUSART module to query.
 * @return the 9th data bit of the last transmitted byte.
 */
uint8_t EPIC_USART_GetTX9D(USART_InstanceTypeDef inst);

/**
 * @brief Set the 9th bit to send NEXT. Must be set BEFORE writing TXREGx.
 * @param inst which EUSART module to configure.
 * @param bit9 the 9th data bit value (0 or 1).
 */
void EPIC_USART_SetTX9D(USART_InstanceTypeDef inst, uint8_t bit9);

/**
 * @brief Returns 1 if the TSR is empty (TRMT = 1).
 * @param inst which EUSART module to query.
 * @return 1 when the transmit shift register is empty, else 0.
 */
uint8_t EPIC_USART_IsTxShiftRegisterEmpty(USART_InstanceTypeDef inst);

/**
 * @brief  Read the latest byte from RCREGx. Reading clears RCIF and
 *         advances the 2-deep FIFO.
 * @param inst which EUSART module to read.
 * @return the received byte.
 */
uint8_t EPIC_USART_Receive(USART_InstanceTypeDef inst);

/**
 * @brief Read RX9D, the 9th bit of the most recently received byte.
 * @param inst which EUSART module to query.
 * @return the 9th data bit of the last received byte.
 */
uint8_t EPIC_USART_GetRX9D(USART_InstanceTypeDef inst);

/**
 * @brief Returns 1 if an overrun was detected (RCSTAx<OERR>). Clear it with
 *        @ref EPIC_USART_ClearOverrun (which cycles CREN).
 * @param inst which EUSART module to query.
 * @return 1 when an overrun occurred, else 0.
 */
uint8_t EPIC_USART_HasOverrun(USART_InstanceTypeDef inst);

/**
 * @brief Clear an overrun: clear CREN, then re-set it (DS39609B §18.2.2).
 * @param inst which EUSART module to reset.
 */
void EPIC_USART_ClearOverrun(USART_InstanceTypeDef inst);

/** @brief Weak EUSART1 TX ISR (PIR1<TXIF>, gated on TXIE). */
void USART_TX_IRQHandler(void) EPIC_WEAK;
/** @brief Weak EUSART1 RX ISR (PIR1<RCIF>). */
void USART_RX_IRQHandler(void) EPIC_WEAK;
/** @brief Weak EUSART2 TX ISR (PIR3<TX2IF>, gated on TX2IE). */
void USART2_TX_IRQHandler(void) EPIC_WEAK;
/** @brief Weak EUSART2 RX ISR (PIR3<RC2IF>). */
void USART2_RX_IRQHandler(void) EPIC_WEAK;

#endif /* PIC18F6520_USART_H */
