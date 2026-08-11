/**
 * PIC16F193X MSSP driver (DS41364B MSSP chapter), SPI master mode only
 * this phase (I2C and SPI slave deferred). CKE=0 carries a real
 * DS80000479 errata (BF/SSPIF set half SCK early); the default handle
 * uses CKE=1, away from it, but CKE=0 is not forbidden. Full reference:
 * MANUAL.md.
 */
#ifndef PIC16F193X_SSP_H
#define PIC16F193X_SSP_H

#include "pic16f193x.h"
#include "pic16f193x_sfr.h"

typedef enum {
    SSP_MODE_SPI_MASTER_FOSC_4  = 0x0U,
    SSP_MODE_SPI_MASTER_FOSC_16 = 0x1U,
    SSP_MODE_SPI_MASTER_FOSC_64 = 0x2U,
    SSP_MODE_SPI_MASTER_TMR2    = 0x3U,
} SSP_ModeTypeDef;

typedef enum {
    SSP_SPI_CKE_ACTIVE_IDLE = 0x0U,  /**< CKE=0, DS80000479 errata. */
    SSP_SPI_CKE_IDLE_ACTIVE = 0x1U,  /**< CKE=1, errata-safe default. */
} SSP_ClockEdgeTypeDef;

typedef enum {
    SSP_SPI_CKP_IDLE_LOW  = 0x0U,
    SSP_SPI_CKP_IDLE_HIGH = 0x1U,
} SSP_ClockPolarityTypeDef;

typedef enum {
    SSP_SPI_SMP_MIDDLE = 0x0U,
    SSP_SPI_SMP_END    = 0x1U,
} SSP_SamplePhaseTypeDef;

typedef struct {
    SSP_ModeTypeDef          Mode;
    SSP_ClockEdgeTypeDef     ClockEdge;
    SSP_ClockPolarityTypeDef ClockPolarity;
    SSP_SamplePhaseTypeDef   SamplePhase;
    void (*TransferCallback)(void);
} SSP_HandleTypeDef;

#define SSP_HANDLE_DEFAULT { \
    .Mode = SSP_MODE_SPI_MASTER_FOSC_4, \
    .ClockEdge = SSP_SPI_CKE_IDLE_ACTIVE, \
    .ClockPolarity = SSP_SPI_CKP_IDLE_LOW, \
    .SamplePhase = SSP_SPI_SMP_MIDDLE, \
    .TransferCallback = 0, \
}

/**
 * @brief  Configure the MSSP as an SPI master from the handle: writes
 *         SSPSTAT (CKE, SMP) and SSPCON1 (SSPM mode, CKP, SSPEN).
 *
 * @param  h  handle with Mode, ClockEdge, ClockPolarity, SamplePhase
 * @return EPIC_OK on success, EPIC_INVALID if `h` is NULL or the mode
 *         is out of range
 */
EPIC_StatusTypeDef EPIC_SSP_Init(const SSP_HandleTypeDef *h);

/**
 * @brief  Reset the MSSP: clear SSPCON1 and SSPSTAT.
 *
 * @return EPIC_OK
 */
EPIC_StatusTypeDef EPIC_SSP_DeInit(void);

/**
 * @brief  Write one byte to the SSPBUF register. If the WCOL write-
 *         collision flag is set (before or after the write), no data is
 *         transferred and 0xFFFF is returned.
 *
 * @param  data  byte to transmit
 * @return the written byte, or 0xFFFF on a write collision
 */
uint16_t EPIC_SSP_WriteByte(uint8_t data);

/**
 * @brief  Read the byte currently in SSPBUF.
 *
 * @return the received byte
 */
uint8_t  EPIC_SSP_ReadByte(void);

/**
 * @brief  Report whether the receive buffer is full (SSPSTAT<BF>).
 *
 * @return 1 if the buffer holds a received byte, 0 otherwise
 */
uint8_t  EPIC_SSP_IsBufferFull(void);

/**
 * @brief  Report whether a write collision occurred (SSPCON1<WCOL>).
 *
 * @return 1 on write collision, 0 otherwise
 */
uint8_t  EPIC_SSP_HasWriteCollision(void);

/**
 * @brief  Clear the write-collision flag (SSPCON1<WCOL>).
 */
void     EPIC_SSP_ClearWriteCollision(void);

/** @brief Weak MSSP interrupt handler; override in user code. */
void SSP_IRQHandler(void) EPIC_WEAK;

#endif /* PIC16F193X_SSP_H */
