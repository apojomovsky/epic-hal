/*
 * Low-Voltage Detect driver, implementation (DS39609B §22.0).
 */

#include "peripherals/pic18f6520_lvd.h"
#include "core/pic18_irq.h"

/* Only the callback is read back (by the ISR below), so it is the only
 * handle field kept. */
static void (*g_lvd_cb)(void) = NULL;

/**
 * @brief  Replace the LVDL/LVDEN fields of LVDCON, leaving the
 *         read-only IRVST bit as read.
 * @param lvdl LVDL<3:0> trip select (low nibble used).
 * @param enable 1 to set LVDEN, 0 to clear it.
 */
static void lvd_write(uint8_t lvdl, uint8_t enable)
{
    uint8_t v = epic_sfr_read8(PIC_REG_LVDCON);
    v = (uint8_t)(v & ~(PIC_LVDCON_LVDL_MASK | PIC_LVDCON_LVDEN));
    v = (uint8_t)(v | (lvdl & PIC_LVDCON_LVDL_MASK));
    if (enable) v = (uint8_t)(v | PIC_LVDCON_LVDEN);
    epic_sfr_write8(PIC_REG_LVDCON, v);
}

/**
 * @brief  Initialize the LVD from a handle: module off, trip level
 *         programmed, flag cleared, interrupt disabled, callback stored.
 * @param h Handle describing the LVD configuration.
 * @return EPIC_OK on success, EPIC_INVALID if `h` is NULL or the trip
 *         level exceeds 15.
 */
EPIC_StatusTypeDef EPIC_LVD_Init(const LVD_HandleTypeDef *h)
{
    if (!h || h->TripLevel > 0x0FU) return EPIC_INVALID;

    lvd_write(h->TripLevel, 0U);
    EPIC_IRQ_DisableSrc(PIC18_IRQ_LVD);
    EPIC_IRQ_ClearFlag(PIC18_IRQ_LVD);
    g_lvd_cb = h->TripCallback;
    return EPIC_OK;
}

/**
 * @brief  De-initialize the LVD: disable its interrupt, clear the flag,
 *         restore LVDCON to its power-on value and drop the callback.
 * @return EPIC_OK.
 */
EPIC_StatusTypeDef EPIC_LVD_DeInit(void)
{
    EPIC_IRQ_DisableSrc(PIC18_IRQ_LVD);
    EPIC_IRQ_ClearFlag(PIC18_IRQ_LVD);
    epic_sfr_write8(PIC_REG_LVDCON, PIC_LVDCON_POR_VALUE);
    g_lvd_cb = NULL;
    return EPIC_OK;
}

/**
 * @brief  Program the trip level and enable the detector (LVDEN).
 * @param h Handle whose trip level is applied.
 * @return EPIC_OK on success, EPIC_INVALID if `h` is NULL or the trip
 *         level exceeds 15.
 */
EPIC_StatusTypeDef EPIC_LVD_Start(const LVD_HandleTypeDef *h)
{
    if (!h || h->TripLevel > 0x0FU) return EPIC_INVALID;

    lvd_write(h->TripLevel, 1U);
    return EPIC_OK;
}

/**
 * @brief  Disable the detector (clear LVDEN), keeping the trip level.
 * @return EPIC_OK.
 */
EPIC_StatusTypeDef EPIC_LVD_Stop(void)
{
    uint8_t v = epic_sfr_read8(PIC_REG_LVDCON);
    lvd_write((uint8_t)(v & PIC_LVDCON_LVDL_MASK), 0U);
    return EPIC_OK;
}

/**
 * @brief  Read the internal-reference-stable flag (LVDCON<IRVST>).
 * @return 1 if the reference is stable, else 0.
 */
uint8_t EPIC_LVD_IsStable(void)
{
    return (epic_sfr_read8(PIC_REG_LVDCON) & PIC_LVDCON_IRVST) ? 1U : 0U;
}

/**
 * @brief  Clear LVDIF and enable the low-voltage interrupt, once the
 *         reference is stable (a flag raised while it settles is not a
 *         real trip).
 * @return EPIC_OK on success, EPIC_INVALID if no callback is stored,
 *         EPIC_BUSY if IRVST is still clear.
 */
EPIC_StatusTypeDef EPIC_LVD_EnableInterrupt(void)
{
    if (!g_lvd_cb) return EPIC_INVALID;
    if (!EPIC_LVD_IsStable()) return EPIC_BUSY;

    EPIC_IRQ_ClearFlag(PIC18_IRQ_LVD);
    EPIC_IRQ_Enable(PIC18_IRQ_LVD);
    return EPIC_OK;
}

/**
 * @brief  Weak LVD interrupt handler: clears LVDIF and invokes the
 *         callback stored by Init.
 */
void LVD_IRQHandler(void)
{
    if (!EPIC_IRQ_GetFlag(PIC18_IRQ_LVD)) return;
    EPIC_IRQ_ClearFlag(PIC18_IRQ_LVD);
    if (g_lvd_cb)
    {
        g_lvd_cb();
    }
}
