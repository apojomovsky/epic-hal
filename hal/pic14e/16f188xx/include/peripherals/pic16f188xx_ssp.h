/**
 * PIC16F188xx MSSP1 driver: SPI master and I2C master (EDC register map;
 * the module matches the PIC16F193X MSSP, DS41364B). Slave modes and the
 * Timer2 clock mode are not driven. Pins come from PPS plus EPIC_GPIO_Init.
 * I2C calls start one bus operation and return: poll EPIC_SSP_I2C_IsIdle.
 * Full reference: MANUAL.md.
 */
#ifndef PIC16F188XX_SSP_H
#define PIC16F188XX_SSP_H

#include <stddef.h>
#include <stdint.h>
#include "pic16f188xx.h"

/** @brief Operating mode (SSP1CON1<SSPM3:SSPM0>). */
typedef enum {
    SSP_MODE_SPI_MASTER_FOSC_4  = 0x0U,   /**< SPI master, Fosc/4. */
    SSP_MODE_SPI_MASTER_FOSC_16 = 0x1U,   /**< SPI master, Fosc/16. */
    SSP_MODE_SPI_MASTER_FOSC_64 = 0x2U,   /**< SPI master, Fosc/64. */
    SSP_MODE_I2C_MASTER         = 0x8U,   /**< I2C master, Fosc/(4*(SSP1ADD+1)). */
    SSP_MODE_SPI_MASTER_ADD     = 0xAU,   /**< SPI master, Fosc/(4*(SSP1ADD+1)). */
} SSP_ModeTypeDef;

/** @brief SPI clock edge (SSP1STAT<CKE>); ignored in I2C mode. */
typedef enum {
    SSP_SPI_CKE_ACTIVE_IDLE = 0x0U,   /**< CKE=0: data changes on idle-to-active. */
    SSP_SPI_CKE_IDLE_ACTIVE = 0x1U,   /**< CKE=1: data changes on active-to-idle. */
} SSP_ClockEdgeTypeDef;

/** @brief SPI clock polarity (SSP1CON1<CKP>); ignored in I2C mode. */
typedef enum {
    SSP_SPI_CKP_IDLE_LOW  = 0x0U,
    SSP_SPI_CKP_IDLE_HIGH = 0x1U,
} SSP_ClockPolarityTypeDef;

/**
 * @brief SPI input sample point (SSP1STAT<SMP>). In I2C mode the same bit
 *        is the slew-rate control: END disables the limit, as 100 kHz and
 *        1 MHz buses need; MIDDLE enables it, for 400 kHz.
 */
typedef enum {
    SSP_SPI_SMP_MIDDLE = 0x0U,
    SSP_SPI_SMP_END    = 0x1U,
} SSP_SamplePhaseTypeDef;

/** Driver handle (Cube-style). */
typedef struct {
    SSP_ModeTypeDef          Mode;
    SSP_ClockEdgeTypeDef     ClockEdge;
    SSP_ClockPolarityTypeDef ClockPolarity;
    SSP_SamplePhaseTypeDef   SamplePhase;
    uint8_t                  Add;   /**< SSP1ADD for the two Fosc/(4*(n+1)) modes. */
    /** @brief Optional callback: runs on each SSP1 event (SSP1IF) and
     *         enables the interrupt in Init. */
    void (*TransferCallback)(void);
} SSP_HandleTypeDef;

#define SSP_HANDLE_DEFAULT {                                              \
    .Mode             = SSP_MODE_SPI_MASTER_FOSC_4,                        \
    .ClockEdge        = SSP_SPI_CKE_IDLE_ACTIVE,                           \
    .ClockPolarity    = SSP_SPI_CKP_IDLE_LOW,                              \
    .SamplePhase      = SSP_SPI_SMP_MIDDLE,                                \
    .Add              = 0U,                                                \
    .TransferCallback = NULL,                                              \
}

/**
 * @brief Compute SSP1ADD for a bus clock: Fosc / (4 * (n + 1)).
 * @details The divisor rounds up, so the bus never runs faster than asked.
 * @param fosc_hz System oscillator frequency in Hz.
 * @param clock_hz Desired SCK or SCL frequency in Hz.
 * @return n (0..255), or 0xFFFF when either input is 0 or n does not fit.
 */
uint16_t SSP_ComputeADD(uint32_t fosc_hz, uint32_t clock_hz);

/**
 * @brief Configure MSSP1 from the handle and enable it.
 * @details Writes SSP1STAT (CKE, SMP), SSP1ADD, clears SSP1CON2, then
 * SSP1CON1 last (SSPM, CKP, SSPEN) so the port goes live fully configured.
 * Enables the SSP1 interrupt only with a callback.
 * @param h Handle with mode, SPI timing, Add and callback.
 * @return EPIC_OK on success, EPIC_INVALID for a NULL handle or a mode or
 *         field outside the enums.
 */
EPIC_StatusTypeDef EPIC_SSP_Init(const SSP_HandleTypeDef *h);

/**
 * @brief Disable MSSP1 and restore the POR state of its registers.
 * @details Clears SSP1CON1 first (port off), then SSP1STAT, SSP1CON2,
 * SSP1CON3 and SSP1ADD, masks the interrupt and drops the callback.
 * @return EPIC_OK.
 */
EPIC_StatusTypeDef EPIC_SSP_DeInit(void);

/**
 * @brief Write one byte to SSP1BUF, starting an SPI transfer or an I2C
 *        byte (address or data).
 * @details If WCOL is set before or after the write, nothing was sent.
 * @param data Byte to send.
 * @return The byte written, or 0xFFFF on a write collision.
 */
uint16_t EPIC_SSP_WriteByte(uint8_t data);

/**
 * @brief Read SSP1BUF (the byte shifted in), which clears BF.
 * @return The received byte.
 */
uint8_t EPIC_SSP_ReadByte(void);

/**
 * @brief Report whether SSP1BUF holds an unread byte (SSP1STAT<BF>).
 * @return 1 when full, 0 otherwise.
 */
uint8_t EPIC_SSP_IsBufferFull(void);

/**
 * @brief Report a write collision (SSP1CON1<WCOL>).
 * @return 1 when WCOL is set, 0 otherwise.
 */
uint8_t EPIC_SSP_HasWriteCollision(void);

/**
 * @brief Clear the write-collision flag (SSP1CON1<WCOL>).
 */
void EPIC_SSP_ClearWriteCollision(void);

/**
 * @brief Report a receive overflow (SSP1CON1<SSPOV>): a byte arrived while
 *        SSP1BUF still held the previous one.
 * @return 1 when SSPOV is set, 0 otherwise.
 */
uint8_t EPIC_SSP_HasOverflow(void);

/**
 * @brief Clear the receive-overflow flag (SSP1CON1<SSPOV>).
 */
void EPIC_SSP_ClearOverflow(void);

/**
 * @brief Generate an I2C START (sets SSP1CON2<SEN>).
 * @return EPIC_OK when started, EPIC_BUSY while a bus operation is running,
 *         EPIC_INVALID unless MSSP1 is enabled as an I2C master.
 */
EPIC_StatusTypeDef EPIC_SSP_I2C_Start(void);

/**
 * @brief Generate an I2C repeated START (sets SSP1CON2<RSEN>).
 * @return EPIC_OK when started, EPIC_BUSY while a bus operation is running,
 *         EPIC_INVALID unless MSSP1 is enabled as an I2C master.
 */
EPIC_StatusTypeDef EPIC_SSP_I2C_RepeatedStart(void);

/**
 * @brief Generate an I2C STOP (sets SSP1CON2<PEN>).
 * @return EPIC_OK when started, EPIC_BUSY while a bus operation is running,
 *         EPIC_INVALID unless MSSP1 is enabled as an I2C master.
 */
EPIC_StatusTypeDef EPIC_SSP_I2C_Stop(void);

/**
 * @brief Clock in one byte from the slave (sets SSP1CON2<RCEN>).
 * @details Poll EPIC_SSP_I2C_IsIdle, then EPIC_SSP_ReadByte, then answer
 * with EPIC_SSP_I2C_SendAck.
 * @return EPIC_OK when started, EPIC_BUSY while a bus operation is running,
 *         EPIC_INVALID unless MSSP1 is enabled as an I2C master.
 */
EPIC_StatusTypeDef EPIC_SSP_I2C_ReceiveEnable(void);

/**
 * @brief Send the acknowledge bit after a received byte (ACKDT then ACKEN).
 * @param nack Nonzero sends NACK (ends a read), zero sends ACK.
 * @return EPIC_OK when started, EPIC_BUSY while a bus operation is running,
 *         EPIC_INVALID unless MSSP1 is enabled as an I2C master.
 */
EPIC_StatusTypeDef EPIC_SSP_I2C_SendAck(uint8_t nack);

/**
 * @brief Report whether the I2C master is idle: no START, repeated START,
 *        STOP, receive or acknowledge running, and no byte being sent.
 * @return 1 when idle, 0 otherwise.
 */
uint8_t EPIC_SSP_I2C_IsIdle(void);

/**
 * @brief Report whether the slave acknowledged the last byte written
 *        (SSP1CON2<ACKSTAT> clear).
 * @return 1 when the slave sent ACK, 0 on NACK.
 */
uint8_t EPIC_SSP_I2C_SlaveAcked(void);

/**
 * @brief Report a bus collision (PIR3<BCL1IF>).
 * @return 1 when the flag is set, 0 otherwise.
 */
uint8_t EPIC_SSP_I2C_HasBusCollision(void);

/**
 * @brief Clear the bus-collision flag (PIR3<BCL1IF>).
 */
void EPIC_SSP_I2C_ClearBusCollision(void);

/**
 * @brief SSP1 interrupt handler: clears SSP1IF, then runs the callback.
 * @details Called from epic_dispatch_all_irqs only when SSP1IF and SSP1IE
 * are both set. Keeps no cstack autos (ISR bank rule, family MANUAL.md).
 */
void SSP_IRQHandler(void);

#endif
