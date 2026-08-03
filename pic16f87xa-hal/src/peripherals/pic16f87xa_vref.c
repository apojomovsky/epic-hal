/**
 * @file    pic16f87xa_vref.c
 * @brief   Voltage Reference driver, implementation (DS39582B §13.0).
 */

#include "peripherals/pic16f87xa_vref.h"

HAL_StatusTypeDef HAL_VREF_Init(const VREF_HandleTypeDef *h)
{
    if (!h) return HAL_INVALID;

    /* Build CVRCON (Bank 1, address 0x9D). */
    uint8_t v = h->Value & PIC_CVRCON_CVR_MASK;
    if (h->Range == VREF_RANGE_HIGH) v |= PIC_CVRCON_CVRR;
    if (h->OutputEnable)            v |= PIC_CVRCON_CVROE;
    if (h->Enabled)                 v |= PIC_CVRCON_CVREN;
#ifdef PIC8_BANK1_WRITE8
    /* See target/pic16f87xa_platform.h: a plain bank-switch write here
     * silently corrupts under XC8 v4.00. */
    PIC8_BANK1_WRITE8(CVRCON, v);
#else
    {
        uint8_t prev = (PIC8_REG8(PIC_REG_STATUS) >> 5) & 0x03U;
        pic_select_bank(1);
        PIC8_REG8(0x9DU) = v;
        pic_select_bank(prev);
    }
#endif
    return HAL_OK;
}

HAL_StatusTypeDef HAL_VREF_DeInit(void)
{
    uint8_t prev = (PIC8_REG8(PIC_REG_STATUS) >> 5) & 0x03U;
    pic_select_bank(1);
    PIC8_REG8(0x9DU) = 0x00U;
    pic_select_bank(prev);
    return HAL_OK;
}

uint32_t HAL_VREF_MilliVolts(uint32_t vdd_mv,
                             VREF_RangeTypeDef range,
                             uint8_t value)
{
    value &= 0x0FU;
    if (range == VREF_RANGE_LOW) {
        /* CVREF = (VR<3:0>/24) × CVRSRC */
        return (vdd_mv * value) / 24U;
    } else {
        /* CVREF = 1/4 × CVRSRC + (VR<3:0>/32) × CVRSRC */
        return (vdd_mv / 4U) + (vdd_mv * value) / 32U;
    }
}
