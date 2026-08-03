/**
 * @file    pic18fxx5x_spp.c
 * @brief   Streaming Parallel Port driver, implementation (DS39632E §18.0).
 *
 *   Register-level only: the USB streaming protocol itself (endpoint
 *   management, CLK1/CLK2 toggling, ownership handoff) is left to the
 *   user. The handle is copied into owned storage so a caller may
 *   stack-allocate it; the sim backend drives status via
 *   `pic18_sim_drive_spp()`.
 */

#include "peripherals/pic18fxx5x_spp.h"
#include "core/pic18_irq.h"

/* ───────────────────────── handle storage ───────────────────────── */

static SPP_HandleTypeDef        g_spp_storage;
static const SPP_HandleTypeDef *g_spp = NULL;

/* ───────────────────────── public API ───────────────────────────── */

HAL_StatusTypeDef HAL_SPP_Init(const SPP_HandleTypeDef *h)
{
    if (!h) return HAL_INVALID;
    g_spp_storage = *h;
    g_spp = &g_spp_storage;

    /* SPPCFG (Register 18-2): CLKCFG1:0 | CSEN | CLK1EN | WS3:WS0. */
    uint8_t cfg = (uint8_t)((h->ClockConfig & 0x03U) << PIC_SPPCFG_CLKCFG_POS);
    if (h->CSEnable)   cfg |= PIC_SPPCFG_CSEN;
    if (h->CLK1Enable) cfg |= PIC_SPPCFG_CLK1EN;
    cfg |= (uint8_t)(h->WaitStates & PIC_SPPCFG_WS_MASK);
    pic8_sfr_write8(PIC_REG_SPPCFG, cfg);

    /* SPPEPS (Register 18-3): endpoint address (status bits read-only). */
    pic8_sfr_write8(PIC_REG_SPPEPS, (uint8_t)(h->Endpoint & PIC_SPPEPS_ADDR_MASK));

    /* SPPCON (Register 18-1): SPPOWN | SPPEN. */
    uint8_t con = PIC_SPPCON_SPPEN;
    if (h->Ownership == SPP_OWN_USB) con |= PIC_SPPCON_SPPOWN;
    pic8_sfr_write8(PIC_REG_SPPCON, con);

    /* Interrupt enable. */
    HAL_IRQ_ClearFlag(PIC18_IRQ_SPP);
    if (h->TransferCallback) HAL_IRQ_Enable(PIC18_IRQ_SPP);
    else                     HAL_IRQ_DisableSrc(PIC18_IRQ_SPP);

    return HAL_OK;
}

HAL_StatusTypeDef HAL_SPP_DeInit(void)
{
    HAL_IRQ_DisableSrc(PIC18_IRQ_SPP);
    HAL_IRQ_ClearFlag(PIC18_IRQ_SPP);
    pic8_sfr_write8(PIC_REG_SPPCON, PIC_SPPCON_POR_VALUE);
    pic8_sfr_write8(PIC_REG_SPPCFG, PIC_SPPCFG_POR_VALUE);
    pic8_sfr_write8(PIC_REG_SPPEPS, PIC_SPPEPS_POR_VALUE);
    g_spp = NULL;
    return HAL_OK;
}

void HAL_SPP_WriteByte(uint8_t ep, uint8_t data)
{
    /* Select the endpoint address, then load the data register. */
    pic8_sfr_write8(PIC_REG_SPPEPS, (uint8_t)(ep & PIC_SPPEPS_ADDR_MASK));
    pic8_sfr_write8(PIC_REG_SPPDATA, data);
}

uint8_t HAL_SPP_ReadByte(uint8_t ep)
{
    pic8_sfr_write8(PIC_REG_SPPEPS, (uint8_t)(ep & PIC_SPPEPS_ADDR_MASK));
    return pic8_sfr_read8(PIC_REG_SPPDATA);
}

uint8_t HAL_SPP_IsBusy(void)
{
    return (pic8_sfr_read8(PIC_REG_SPPEPS) & PIC_SPPEPS_SPPBUSY) ? 1U : 0U;
}

uint8_t HAL_SPP_HasWriteOccurred(void)
{
    return (pic8_sfr_read8(PIC_REG_SPPEPS) & PIC_SPPEPS_WRSPP) ? 1U : 0U;
}

uint8_t HAL_SPP_HasReadOccurred(void)
{
    return (pic8_sfr_read8(PIC_REG_SPPEPS) & PIC_SPPEPS_RDSPP) ? 1U : 0U;
}

uint8_t HAL_SPP_IsInterruptFlag(void)
{
    return (pic8_sfr_read8(PIC_REG_PIR1) & PIC_PIR1_SPPIF) ? 1U : 0U;
}

void HAL_SPP_ClearITFlag(void)
{
    HAL_IRQ_ClearFlag(PIC18_IRQ_SPP);
}

/* ───────────────────────── ISR ───────────────────────────────────── */

void SPP_IRQHandler(void)
{
    if (!HAL_IRQ_GetFlag(PIC18_IRQ_SPP)) return;
    HAL_IRQ_ClearFlag(PIC18_IRQ_SPP);
    if (g_spp && g_spp->TransferCallback) g_spp->TransferCallback();
}
