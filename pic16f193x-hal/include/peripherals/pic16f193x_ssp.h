/**
 * @file    peripherals/pic16f193x_ssp.h
 * @brief   PIC16F193X MSSP driver, SPI master mode only this phase
 *          (I2C and SPI slave deferred). CKE=0 carries a real
 *          DS80000479 errata (BF/SSPIF set half SCK early); the default
 *          handle uses CKE=1, away from it, but CKE=0 is not forbidden.
 *
 * @details Source: DS41364B MSSP chapter. Full reference: MANUAL.md.
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

EPIC_StatusTypeDef EPIC_SSP_Init(const SSP_HandleTypeDef *h);
EPIC_StatusTypeDef EPIC_SSP_DeInit(void);
uint16_t EPIC_SSP_WriteByte(uint8_t data);
uint8_t  EPIC_SSP_ReadByte(void);
uint8_t  EPIC_SSP_IsBufferFull(void);
uint8_t  EPIC_SSP_HasWriteCollision(void);
void     EPIC_SSP_ClearWriteCollision(void);

void SSP_IRQHandler(void) EPIC_WEAK;

#endif /* PIC16F193X_SSP_H */
