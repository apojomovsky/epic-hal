/* SR latch driver implementation (DS40001291H §8.9). SRCON is Bank 3
 * (0x185). */

#include "peripherals/pic16f88x_srlatch.h"

/**
 * @brief Initialize the SR latch: program SRCON from the handle.
 * @param h handle with Output, C1SetEnable, C2ResetEnable, FVREN.
 * @return EPIC_OK on success, EPIC_INVALID if `h` is NULL.
 */
EPIC_StatusTypeDef EPIC_SRLATCH_Init(const SRLATCH_HandleTypeDef *h)
{
    if (!h) return EPIC_INVALID;

    uint8_t v = (uint8_t)(((uint8_t)h->Output & 0x03U) << 6);  /* SR1:SR0, bits 7:6. */
    if (h->C1SetEnable)   v |= PIC_SRCON_C1SEN;
    if (h->C2ResetEnable) v |= PIC_SRCON_C2REN;
    if (h->FVREN)         v |= PIC_SRCON_FVREN;
#ifdef EPIC_BANK3_WRITE8
    EPIC_BANK3_WRITE8(SRCON, v);
#else
    EPIC_REG8(PIC_REG_SRCON) = v;
#endif
    return EPIC_OK;
}

/**
 * @brief De-initialize the SR latch: clear SRCON.
 * @return EPIC_OK on success.
 */
EPIC_StatusTypeDef EPIC_SRLATCH_DeInit(void)
{
#ifdef EPIC_BANK3_WRITE8
    EPIC_BANK3_WRITE8(SRCON, 0x00U);
#else
    EPIC_REG8(PIC_REG_SRCON) = 0x00U;
#endif
    return EPIC_OK;
}

/**
 * @brief Set the SR latch with a software pulse (PULSS).
 */
void EPIC_SRLATCH_Set(void)
{
#ifdef EPIC_BANK3_READ8
    uint8_t srcon = 0u;
    EPIC_BANK3_READ8(SRCON, srcon);
    srcon |= PIC_SRCON_PULSS;
    EPIC_BANK3_WRITE8(SRCON, srcon);
#else
    EPIC_REG8(PIC_REG_SRCON) |= PIC_SRCON_PULSS;
#endif
}

/**
 * @brief Reset the SR latch with a software pulse (PULSR).
 */
void EPIC_SRLATCH_Reset(void)
{
#ifdef EPIC_BANK3_READ8
    uint8_t srcon = 0u;
    EPIC_BANK3_READ8(SRCON, srcon);
    srcon |= PIC_SRCON_PULSR;
    EPIC_BANK3_WRITE8(SRCON, srcon);
#else
    EPIC_REG8(PIC_REG_SRCON) |= PIC_SRCON_PULSR;
#endif
}
