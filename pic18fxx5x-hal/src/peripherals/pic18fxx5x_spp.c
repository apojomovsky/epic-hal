/*
 * Streaming Parallel Port driver, implementation (DS39632E §18.0).
 * Register-level only: the USB streaming protocol itself (endpoint
 * management, CLK1/CLK2 toggling, ownership handoff) is left to the
 * user. The handle is copied into owned storage so a caller may
 * stack-allocate it; the sim backend drives status via
 * `pic18_sim_drive_spp()`.
 */

#include "peripherals/pic18fxx5x_spp.h"
#include "core/pic18_irq.h"

static SPP_HandleTypeDef        g_spp_storage;
static const SPP_HandleTypeDef *g_spp = NULL;

/**
 * @brief  Initialize the Streaming Parallel Port from a handle. Programs
 *         SPPCFG (clock config, chip-select, CLK1 enable, wait states),
 *         SPPEPS (endpoint address) and SPPCON (ownership + SPPEN), then
 *         arms the transfer interrupt if a callback is provided.
 * @param h Handle describing the SPP configuration.
 * @return EPIC_OK on success, EPIC_INVALID if `h` is NULL.
 */
EPIC_StatusTypeDef EPIC_SPP_Init(const SPP_HandleTypeDef *h)
{
    if (!h) return EPIC_INVALID;
    g_spp_storage = *h;
    g_spp = &g_spp_storage;

    /* SPPCFG (Register 18-2): CLKCFG1:0 | CSEN | CLK1EN | WS3:WS0. */
    uint8_t cfg = (uint8_t)((h->ClockConfig & 0x03U) << PIC_SPPCFG_CLKCFG_POS);
    if (h->CSEnable)   cfg |= PIC_SPPCFG_CSEN;
    if (h->CLK1Enable) cfg |= PIC_SPPCFG_CLK1EN;
    cfg |= (uint8_t)(h->WaitStates & PIC_SPPCFG_WS_MASK);
    epic_sfr_write8(PIC_REG_SPPCFG, cfg);

    /* SPPEPS (Register 18-3): endpoint address (status bits read-only). */
    epic_sfr_write8(PIC_REG_SPPEPS, (uint8_t)(h->Endpoint & PIC_SPPEPS_ADDR_MASK));

    /* SPPCON (Register 18-1): SPPOWN | SPPEN. */
    uint8_t con = PIC_SPPCON_SPPEN;
    if (h->Ownership == SPP_OWN_USB) con |= PIC_SPPCON_SPPOWN;
    epic_sfr_write8(PIC_REG_SPPCON, con);

    /* Interrupt enable. */
    EPIC_IRQ_ClearFlag(PIC18_IRQ_SPP);
    if (h->TransferCallback) EPIC_IRQ_Enable(PIC18_IRQ_SPP);
    else                     EPIC_IRQ_DisableSrc(PIC18_IRQ_SPP);

    return EPIC_OK;
}

/**
 * @brief  De-initialize the SPP: disable its interrupt, clear the flag,
 *         restore SPPCON/SPPCFG/SPPEPS to their power-on values and drop
 *         the stored handle.
 * @return EPIC_OK.
 */
EPIC_StatusTypeDef EPIC_SPP_DeInit(void)
{
    EPIC_IRQ_DisableSrc(PIC18_IRQ_SPP);
    EPIC_IRQ_ClearFlag(PIC18_IRQ_SPP);
    epic_sfr_write8(PIC_REG_SPPCON, PIC_SPPCON_POR_VALUE);
    epic_sfr_write8(PIC_REG_SPPCFG, PIC_SPPCFG_POR_VALUE);
    epic_sfr_write8(PIC_REG_SPPEPS, PIC_SPPEPS_POR_VALUE);
    g_spp = NULL;
    return EPIC_OK;
}

/**
 * @brief  Select an endpoint address and write a data byte to SPPDATA.
 * @param ep Endpoint address (masked to the SPPEPS address field).
 * @param data Byte to transmit.
 */
void EPIC_SPP_WriteByte(uint8_t ep, uint8_t data)
{
    /* Select the endpoint address, then load the data register. */
    epic_sfr_write8(PIC_REG_SPPEPS, (uint8_t)(ep & PIC_SPPEPS_ADDR_MASK));
    epic_sfr_write8(PIC_REG_SPPDATA, data);
}

/**
 * @brief  Select an endpoint address and read a data byte from SPPDATA.
 * @param ep Endpoint address (masked to the SPPEPS address field).
 * @return The byte received on `ep`.
 */
uint8_t EPIC_SPP_ReadByte(uint8_t ep)
{
    epic_sfr_write8(PIC_REG_SPPEPS, (uint8_t)(ep & PIC_SPPEPS_ADDR_MASK));
    return epic_sfr_read8(PIC_REG_SPPDATA);
}

/**
 * @brief  Return 1 if the SPP is busy (SPPEPS<SPPBUSY>).
 * @return 1 if busy, else 0.
 */
uint8_t EPIC_SPP_IsBusy(void)
{
    return (epic_sfr_read8(PIC_REG_SPPEPS) & PIC_SPPEPS_SPPBUSY) ? 1U : 0U;
}

/**
 * @brief  Return 1 if a write from the SPP has occurred (SPPEPS<WRSPP>).
 * @return 1 if a write occurred, else 0.
 */
uint8_t EPIC_SPP_HasWriteOccurred(void)
{
    return (epic_sfr_read8(PIC_REG_SPPEPS) & PIC_SPPEPS_WRSPP) ? 1U : 0U;
}

/**
 * @brief  Return 1 if a read by the SPP has occurred (SPPEPS<RDSPP>).
 * @return 1 if a read occurred, else 0.
 */
uint8_t EPIC_SPP_HasReadOccurred(void)
{
    return (epic_sfr_read8(PIC_REG_SPPEPS) & PIC_SPPEPS_RDSPP) ? 1U : 0U;
}

/**
 * @brief  Return 1 if the SPP interrupt flag (PIR1<SPPIF>) is set.
 * @return 1 if SPPIF is set, else 0.
 */
uint8_t EPIC_SPP_IsInterruptFlag(void)
{
    return (epic_sfr_read8(PIC_REG_PIR1) & PIC_PIR1_SPPIF) ? 1U : 0U;
}

/**
 * @brief  Clear the SPP interrupt flag (PIR1<SPPIF>).
 */
void EPIC_SPP_ClearITFlag(void)
{
    EPIC_IRQ_ClearFlag(PIC18_IRQ_SPP);
}

/**
 * @brief  Weak SPP interrupt handler: clears SPPIF and invokes the
 *         transfer callback registered via Init.
 */
void SPP_IRQHandler(void)
{
    if (!EPIC_IRQ_GetFlag(PIC18_IRQ_SPP)) return;
    EPIC_IRQ_ClearFlag(PIC18_IRQ_SPP);
    if (g_spp && g_spp->TransferCallback) g_spp->TransferCallback();
}
