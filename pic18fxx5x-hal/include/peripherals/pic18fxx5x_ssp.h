/*
 * MSSP driver: SPI master/slave + I2C master/slave (DS39632E §19.0).
 * The PIC18 MSSP registers are all in the Access Bank (no bank switching)
 * and the control register is SSPCON1 (PIC16's is SSPCON). Register-level
 * only: the I2C Start/Stop/ACK state machine is left to the caller; SPI
 * transfers complete automatically once SSPBUF is written, poll
 * SSPSTAT<BF> for a byte's arrival.
 */

#ifndef PIC18FXX5X_SSP_H
#define PIC18FXX5X_SSP_H

#include "pic18fxx5x.h"
#include "pic18fxx5x_sfr.h"

/**
 * @brief SSP mode select (SSPCON1<3:0>, DS39632E Registers 19-2/19-4).
 */
typedef enum {
    SSP_MODE_SPI_MASTER_FOSC_4   = 0x0U,   /**< 0000, SPI master, Fosc/4.   */
    SSP_MODE_SPI_MASTER_FOSC_16  = 0x1U,   /**< 0001, SPI master, Fosc/16.  */
    SSP_MODE_SPI_MASTER_FOSC_64  = 0x2U,   /**< 0010, SPI master, Fosc/64.  */
    SSP_MODE_SPI_MASTER_TMR2     = 0x3U,   /**< 0011, SPI master, TMR2/2.   */
    SSP_MODE_SPI_SLAVE_SS_DIS    = 0x4U,   /**< 0100, SPI slave, SS disabled. */
    SSP_MODE_SPI_SLAVE_SS_EN     = 0x5U,   /**< 0101, SPI slave, SS enabled. */
    SSP_MODE_I2C_SLAVE_7BIT      = 0x6U,   /**< 0110, I²C slave, 7-bit. */
    SSP_MODE_I2C_SLAVE_10BIT     = 0x7U,   /**< 0111, I²C slave, 10-bit. */
    SSP_MODE_I2C_MASTER_FW       = 0x8U,   /**< 1000, I²C master, firmware. */
    SSP_MODE_I2C_SLAVE_7BIT_SS   = 0x9U,   /**< 1001, I²C slave, 7-bit + S/S. */
    SSP_MODE_I2C_SLAVE_10BIT_SS  = 0xAU,   /**< 1010, I²C slave, 10-bit + S/S. */
    SSP_MODE_I2C_MASTER_FOSC     = 0xBU,   /**< 1011, I²C master, hardware. */
} SSP_ModeTypeDef;

/**
 * @brief SPI clock-edge select (SSPSTAT<CKE>, DS39632E §19.2.1).
 */
typedef enum {
    SSP_SPI_CKE_ACTIVE_IDLE = 0x0U,   /**< CKE=0: transmit on idle->active. */
    SSP_SPI_CKE_IDLE_ACTIVE = 0x1U,   /**< CKE=1: transmit on active->idle. */
} SSP_ClockEdgeTypeDef;

/**
 * @brief SPI clock polarity (SSPCON1<CKP>, DS39632E §19.2.4).
 */
typedef enum {
    SSP_SPI_CKP_IDLE_LOW   = 0x0U,
    SSP_SPI_CKP_IDLE_HIGH  = 0x1U,
} SSP_ClockPolarityTypeDef;

/**
 * @brief SPI input-data sample phase (SSPSTAT<SMP>, DS39632E §19.2.4).
 *        Master mode only.
 */
typedef enum {
    SSP_SPI_SMP_MIDDLE = 0x0U,   /**< SMP=0: sample at middle of data. */
    SSP_SPI_SMP_END    = 0x1U,   /**< SMP=1: sample at end of data. */
} SSP_SamplePhaseTypeDef;

/** Driver handle (Cube-style). */
typedef struct {
    SSP_ModeTypeDef           Mode;
    SSP_ClockEdgeTypeDef      ClockEdge;       /**< SPI only. */
    SSP_ClockPolarityTypeDef  ClockPolarity;   /**< SPI only. */
    SSP_SamplePhaseTypeDef    SamplePhase;     /**< SPI master only. */
    uint8_t                   SSPADD;          /**< I²C slave address or master baud reload. */
    /** @brief Optional transfer callback (fires on SSPIF). */
    void (*TransferCallback)(void);
} SSP_HandleTypeDef;

#define SSP_HANDLE_DEFAULT {                                              \
    .Mode          = SSP_MODE_SPI_MASTER_FOSC_4,                           \
    .ClockEdge     = SSP_SPI_CKE_IDLE_ACTIVE,                              \
    .ClockPolarity = SSP_SPI_CKP_IDLE_LOW,                                 \
    .SamplePhase   = SSP_SPI_SMP_MIDDLE,                                   \
    .SSPADD        = 0,                                                    \
    .TransferCallback = NULL,                                              \
}

EPIC_StatusTypeDef EPIC_SSP_Init(const SSP_HandleTypeDef *h);
EPIC_StatusTypeDef EPIC_SSP_DeInit(void);

/**
 * @brief  Write a byte to SSPBUF. Returns 0xFFFF if WCOL (write collision)
 *         was set, in which case the byte was *not* written; retry.
 */
uint16_t EPIC_SSP_WriteByte(uint8_t data);

/** Read the most recently received byte from SSPBUF (clears BF). */
uint8_t  EPIC_SSP_ReadByte(void);

/** Returns 1 if SSPBUF holds an unread byte (BF = 1). */
uint8_t  EPIC_SSP_IsBufferFull(void);

/** Returns 1 if a write collision was detected. */
uint8_t  EPIC_SSP_HasWriteCollision(void);

/** Clear the WCOL flag (must be done in software per §19.2.2). */
void     EPIC_SSP_ClearWriteCollision(void);

/**
 * @brief  Compute SSPADD for an I²C master baud rate.
 *         DS39632E §19.4.2: Fscl = Fosc / (4 x (SSPADD + 1))
 *         -> SSPADD = (Fosc / (4 x Fscl)) - 1.
 */
uint16_t SSP_ComputeSSPADD(uint32_t fosc_hz, uint32_t fscl_hz);

/** Issue a Start condition (sets SSPCON2<SEN>). */
void EPIC_SSP_Start(void);

/** Issue a Repeated Start condition. */
void EPIC_SSP_RepeatedStart(void);

/** Issue a Stop condition. */
void EPIC_SSP_Stop(void);

/** Begin a receive (master mode). Sets SSPCON2<RCEN>. */
void EPIC_SSP_ReceiveEnable(void);

/** Transmit an ACK (master receive). */
void EPIC_SSP_AcknowledgeEnable(void);

/** Returns 1 if an ACK was received from the slave (ACKSTAT). */
uint8_t EPIC_SSP_AcknowledgeStatus(void);

void SSP_IRQHandler(void) EPIC_WEAK;

#endif /* PIC18FXX5X_SSP_H */
