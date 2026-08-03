/**
 * @file    pic18fxx5x_ssp.c
 * @brief   MSSP driver, implementation (DS39632E §19.0).
 *
 *   Register-level: the I2C state machine (Start/Stop/ACK, address
 *   matching) is left to the user. All MSSP registers are in the Access
 *   Bank, no bank switching. RMW uses split read+write because XC8 cannot
 *   lower a compound assignment on a volatile cast-lvalue.
 */

#include "peripherals/pic18fxx5x_ssp.h"
#include "core/pic18_irq.h"

/* ───────────────────────── handle storage ───────────────────────── */

static SSP_HandleTypeDef        g_ssp_storage;
static const SSP_HandleTypeDef *g_ssp = NULL;

/** Set bits in SSPCON2 (RMW via split read+write). */
static void sspcon2_set(uint8_t mask)
{
    uint8_t v = pic8_sfr_read8(PIC_REG_SSPCON2);
    v |= (uint8_t)mask;
    pic8_sfr_write8(PIC_REG_SSPCON2, v);
}

/* ───────────────────────── SSPADD computation ───────────────────── */

uint16_t SSP_ComputeSSPADD(uint32_t fosc_hz, uint32_t fscl_hz)
{
    if (fscl_hz == 0) return 0xFFFFU;
    uint32_t x = (fosc_hz / (4U * fscl_hz)) - 1U;
    if (x > 255U) return 0xFFFFU;
    return (uint16_t)x;
}

/* ───────────────────────── public API ───────────────────────────── */

HAL_StatusTypeDef HAL_SSP_Init(const SSP_HandleTypeDef *h)
{
    if (!h) return HAL_INVALID;
    g_ssp_storage = *h;
    g_ssp = &g_ssp_storage;

    /* SSPADD (I2C slave address, or I2C master baud reload). */
    pic8_sfr_write8(PIC_REG_SSPADD, h->SSPADD);

    /* SSPSTAT: SMP + CKE (SPI). */
    uint8_t stat = 0U;
    if (h->ClockEdge   == SSP_SPI_CKE_IDLE_ACTIVE) stat |= PIC_SSPSTAT_CKE;
    if (h->SamplePhase == SSP_SPI_SMP_END)        stat |= PIC_SSPSTAT_SMP;
    pic8_sfr_write8(PIC_REG_SSPSTAT, stat);

    /* SSPCON1: SSPM3:0 (mode) + CKP + SSPEN. (WCOL/SSPOV are cleared by
     * writing the register; reset value 0x00.) */
    uint8_t con = (uint8_t)(h->Mode & PIC_SSPCON1_SSPM_MASK);
    if (h->ClockPolarity == SSP_SPI_CKP_IDLE_HIGH) con |= PIC_SSPCON1_CKP;
    con |= PIC_SSPCON1_SSPEN;
    pic8_sfr_write8(PIC_REG_SSPCON1, con);

    /* SSPCON2: idle (all clear). */
    pic8_sfr_write8(PIC_REG_SSPCON2, 0x00U);

    /* Interrupt enable. */
    HAL_IRQ_ClearFlag(PIC18_IRQ_SSP);
    if (h->TransferCallback) HAL_IRQ_Enable(PIC18_IRQ_SSP);
    else                     HAL_IRQ_DisableSrc(PIC18_IRQ_SSP);

    return HAL_OK;
}

HAL_StatusTypeDef HAL_SSP_DeInit(void)
{
    HAL_IRQ_DisableSrc(PIC18_IRQ_SSP);
    HAL_IRQ_ClearFlag(PIC18_IRQ_SSP);
    pic8_sfr_write8(PIC_REG_SSPCON1, 0x00U);
    pic8_sfr_write8(PIC_REG_SSPCON2, 0x00U);
    pic8_sfr_write8(PIC_REG_SSPSTAT, 0x00U);
    pic8_sfr_write8(PIC_REG_SSPADD, 0x00U);
    g_ssp = NULL;
    return HAL_OK;
}

uint16_t HAL_SSP_WriteByte(uint8_t data)
{
    uint8_t con = pic8_sfr_read8(PIC_REG_SSPCON1);
    if (con & PIC_SSPCON1_WCOL) return 0xFFFFU;     /* write collision pending */
    pic8_sfr_write8(PIC_REG_SSPBUF, data);
    /* On a real target the transfer starts and BF + SSPIF assert when it
     * completes; on the host sim, BF/SSPIF are set via the drive hook. */
    return 0U;
}

uint8_t HAL_SSP_ReadByte(void)
{
    /* Reading SSPBUF clears BF (DS39632E Register 19-1). */
    uint8_t v = pic8_sfr_read8(PIC_REG_SSPBUF);
    uint8_t stat = (uint8_t)(pic8_sfr_read8(PIC_REG_SSPSTAT) & (uint8_t)~PIC_SSPSTAT_BF);
    pic8_sfr_write8(PIC_REG_SSPSTAT, stat);
    return v;
}

uint8_t HAL_SSP_IsBufferFull(void)
{
    return (pic8_sfr_read8(PIC_REG_SSPSTAT) & PIC_SSPSTAT_BF) ? 1U : 0U;
}

uint8_t HAL_SSP_HasWriteCollision(void)
{
    return (pic8_sfr_read8(PIC_REG_SSPCON1) & PIC_SSPCON1_WCOL) ? 1U : 0U;
}

void HAL_SSP_ClearWriteCollision(void)
{
    uint8_t con = (uint8_t)(pic8_sfr_read8(PIC_REG_SSPCON1) & (uint8_t)~PIC_SSPCON1_WCOL);
    pic8_sfr_write8(PIC_REG_SSPCON1, con);
}

void HAL_SSP_Start(void)          { sspcon2_set(PIC_SSPCON2_SEN);  }
void HAL_SSP_RepeatedStart(void)  { sspcon2_set(PIC_SSPCON2_RSEN); }
void HAL_SSP_Stop(void)           { sspcon2_set(PIC_SSPCON2_PEN);  }
void HAL_SSP_ReceiveEnable(void)  { sspcon2_set(PIC_SSPCON2_RCEN); }
void HAL_SSP_AcknowledgeEnable(void) { sspcon2_set(PIC_SSPCON2_ACKEN); }

uint8_t HAL_SSP_AcknowledgeStatus(void)
{
    return (pic8_sfr_read8(PIC_REG_SSPCON2) & PIC_SSPCON2_ACKSTAT) ? 1U : 0U;
}

/* ───────────────────────── ISR ───────────────────────────────────── */

void SSP_IRQHandler(void)
{
    if (!HAL_IRQ_GetFlag(PIC18_IRQ_SSP)) return;
    HAL_IRQ_ClearFlag(PIC18_IRQ_SSP);
    if (g_ssp && g_ssp->TransferCallback) g_ssp->TransferCallback();
}
