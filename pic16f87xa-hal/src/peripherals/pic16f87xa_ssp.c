/**
 * @file    pic16f87xa_ssp.c
 * @brief   MSSP driver, implementation (DS39582B §9.0).
 *
 *   This is a register-level driver: the actual I²C state machine
 *   (Start/Stop/ACK timing, slave address matching) is left to the
 *   user. The driver configures SSPCON / SSPCON2 / SSPSTAT / SSPADD
 *   and provides the byte-level transmit / receive primitives.
 */

#include "peripherals/pic16f87xa_ssp.h"
#include "core/pic16_irq.h"

/* ───────────────────────── handle storage ───────────────────────── */

static const SSP_HandleTypeDef *g_ssp = NULL;

/* SSPCON2/SSPSTAT/SSPADD are Bank 1 (SSPSTAT=0x94, SSPCON2=0x91,
 * SSPADD=0x93, DS39582B Figure 2-4). PIC8_BANK1_WRITE8/READ8 need a
 * literal SFR name at compile time (inline-asm operands), not a
 * runtime `addr`, so this dispatches on `addr` before any bank switch
 * begins, then invokes the named macro; every real call site passes a
 * compile-time constant, so XC8 folds this to one branch either way. */
#ifdef PIC8_BANK1_READ8
static uint8_t ssp_b1_read(uint8_t addr)
{
    uint8_t v = 0U;
    if (addr == 0x91U)      PIC8_BANK1_READ8(SSPCON2, v);
    else if (addr == 0x93U) PIC8_BANK1_READ8(SSPADD, v);
    else                    PIC8_BANK1_READ8(SSPSTAT, v);
    return v;
}

static void ssp_b1_write(uint8_t addr, uint8_t v)
{
    if (addr == 0x91U)      PIC8_BANK1_WRITE8(SSPCON2, v);
    else if (addr == 0x93U) PIC8_BANK1_WRITE8(SSPADD, v);
    else                    PIC8_BANK1_WRITE8(SSPSTAT, v);
}
#else
static uint8_t ssp_b1_read(uint8_t addr)
{
    uint8_t prev = (PIC8_REG8(PIC_REG_STATUS) >> 5) & 0x03U;
    pic_select_bank(1);
    uint8_t v = PIC8_REG8(addr);
    pic_select_bank(prev);
    return v;
}

static void ssp_b1_write(uint8_t addr, uint8_t v)
{
    uint8_t prev = (PIC8_REG8(PIC_REG_STATUS) >> 5) & 0x03U;
    pic_select_bank(1);
    PIC8_REG8(addr) = v;
    pic_select_bank(prev);
}
#endif

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
    g_ssp = h;

    /* Program SSPADD (Bank 1, address 0x93). */
    ssp_b1_write(0x93U, h->SSPADD);

    /* Build SSPSTAT (Bank 1, address 0x94). */
    uint8_t stat = 0U;
    if (h->ClockEdge   == SSP_SPI_CKE_IDLE_ACTIVE) stat |= PIC_SSPSTAT_CKE;
    if (h->SamplePhase == SSP_SPI_SMP_END)        stat |= PIC_SSPSTAT_SMP;
    ssp_b1_write(0x94U, stat);

    /* Build SSPCON (Bank 0, address 0x14):
     *   bit 0..3 SSPM3:SSPM0 (mode)
     *   bit 4   CKP
     *   bit 5   SSPEN
     *   bit 6   SSPOV (read-only, cleared by software)
     *   bit 7   WCOL  (read-only, cleared by software)
     * Reset value: 0x00. */
    uint8_t con = (uint8_t)(h->Mode & 0x0FU);
    if (h->ClockPolarity == SSP_SPI_CKP_IDLE_HIGH) con |= PIC_SSPCON_CKP;
    con |= PIC_SSPCON_SSPEN;
    PIC8_REG8(0x14U) = con;

    /* SSPCON2 (Bank 1, address 0x91), clear all bits (idle state). */
    ssp_b1_write(0x91U, 0x00U);

    /* Interrupt enable. */
    HAL_IRQ_ClearFlag(PIC16_IRQ_SSP);
    if (h->TransferCallback) HAL_IRQ_Enable(PIC16_IRQ_SSP);
    else                     HAL_IRQ_DisableSrc(PIC16_IRQ_SSP);

    return HAL_OK;
}

HAL_StatusTypeDef HAL_SSP_DeInit(void)
{
    HAL_IRQ_DisableSrc(PIC16_IRQ_SSP);
    HAL_IRQ_ClearFlag(PIC16_IRQ_SSP);
    PIC8_REG8(0x14U) = 0x00U;
    ssp_b1_write(0x91U, 0x00U);
    ssp_b1_write(0x94U, 0x00U);
    ssp_b1_write(0x93U, 0x00U);
    g_ssp = NULL;
    return HAL_OK;
}

uint16_t HAL_SSP_WriteByte(uint8_t data)
{
    uint8_t con = PIC8_REG8(0x14U);
    if (con & PIC_SSPCON_WCOL) return 0xFFFFU;     /* write collision pending. */
    PIC8_REG8(PIC_REG_SSPBUF) = data;
    /* The sim backend sets BF + SSPIF on the next sim_step
     * (see sim_step_ssp() in src/sim/pic16f87xa_sim.c). */
    return 0U;
}

uint8_t HAL_SSP_ReadByte(void)
{
    /* Reading SSPBUF clears BF (Register 9-1). */
    uint8_t v = PIC8_REG8(PIC_REG_SSPBUF);
    PIC8_REG8(0x94U) &= (uint8_t)~PIC_SSPSTAT_BF;
    return v;
}

uint8_t HAL_SSP_IsBufferFull(void)
{
    return (PIC8_REG8(0x94U) & PIC_SSPSTAT_BF) ? 1U : 0U;
}

uint8_t HAL_SSP_HasWriteCollision(void)
{
    return (PIC8_REG8(0x14U) & PIC_SSPCON_WCOL) ? 1U : 0U;
}

void HAL_SSP_ClearWriteCollision(void)
{
    PIC8_REG8(0x14U) &= (uint8_t)~PIC_SSPCON_WCOL;
}

void HAL_SSP_Start(void)
{
    /* Writing SSPCON2<SEN>=1 initiates a Start. The hardware clears
     * the bit when the Start completes. (§9.4.2) */
    ssp_b1_write(0x91U, ssp_b1_read(0x91U) | PIC_SSPCON2_SEN);
}

void HAL_SSP_RepeatedStart(void)
{
    ssp_b1_write(0x91U, ssp_b1_read(0x91U) | PIC_SSPCON2_RSEN);
}

void HAL_SSP_Stop(void)
{
    ssp_b1_write(0x91U, ssp_b1_read(0x91U) | PIC_SSPCON2_PEN);
}

void HAL_SSP_ReceiveEnable(void)
{
    ssp_b1_write(0x91U, ssp_b1_read(0x91U) | PIC_SSPCON2_RCEN);
}

void HAL_SSP_AcknowledgeEnable(void)
{
    /* Begin ACK sequence: SSPCON2<ACKEN>=1, ACKDT holds the bit value. */
    ssp_b1_write(0x91U, ssp_b1_read(0x91U) | PIC_SSPCON2_ACKEN);
}

uint8_t HAL_SSP_AcknowledgeStatus(void)
{
    return (ssp_b1_read(0x91U) & PIC_SSPCON2_ACKSTAT) ? 1U : 0U;
}

/* ───────────────────────── ISRs ─────────────────────────────────── */

void SSP_IRQHandler(void)
{
    if (!HAL_IRQ_GetFlag(PIC16_IRQ_SSP)) return;
    HAL_IRQ_ClearFlag(PIC16_IRQ_SSP);
    if (g_ssp && g_ssp->TransferCallback) g_ssp->TransferCallback();
}
