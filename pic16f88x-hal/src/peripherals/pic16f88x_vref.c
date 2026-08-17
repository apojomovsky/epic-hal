/* Voltage Reference driver implementation (DS40001291H §8.10). */

#include "peripherals/pic16f88x_vref.h"

/**
 * @brief Initialize the voltage reference: program VRCON from the
 *        handle.
 * @param h handle with Range, Source, Value, OutputEnable, Enabled.
 * @return EPIC_OK on success, EPIC_INVALID if `h` is NULL.
 */
EPIC_StatusTypeDef EPIC_VREF_Init(const VREF_HandleTypeDef *h)
{
    if (!h) return EPIC_INVALID;

    /* Build VRCON (Bank 1, address 0x97, DS40001291H Register 8-5). */
    uint8_t v = h->Value & PIC_VRCON_VR_MASK;
    if (h->Source == VREF_SRC_VREFP_VREFN) v |= PIC_VRCON_VRSS;
    if (h->Range == VREF_RANGE_LOW)        v |= PIC_VRCON_VRR;
    if (h->OutputEnable)                   v |= PIC_VRCON_VROE;
    if (h->Enabled)                        v |= PIC_VRCON_VREN;
#ifdef EPIC_BANK1_WRITE8
    /* See target/pic16f88x_platform.h: a plain bank-switch write here
     * silently corrupts under XC8 v4.00. */
    EPIC_BANK1_WRITE8(VRCON, v);
#else
    {
        uint8_t prev = (EPIC_REG8(PIC_REG_STATUS) >> 5) & 0x03U;
        pic_select_bank(1);
        EPIC_REG8(PIC_REG_VRCON) = v;
        pic_select_bank(prev);
    }
#endif
    return EPIC_OK;
}

/**
 * @brief De-initialize the voltage reference: clear VRCON.
 * @return EPIC_OK on success.
 */
EPIC_StatusTypeDef EPIC_VREF_DeInit(void)
{
    uint8_t prev = (EPIC_REG8(PIC_REG_STATUS) >> 5) & 0x03U;
    pic_select_bank(1);
    EPIC_REG8(PIC_REG_VRCON) = 0x00U;
    pic_select_bank(prev);
    return EPIC_OK;
}

/**
 * @brief Compute the nominal output voltage in millivolts
 *        (DS40001291H Register 8-5 formulas).
 * @param vdd_mv the supply voltage in millivolts.
 * @param range VREF_RANGE_LOW or VREF_RANGE_HIGH.
 * @param value the ladder tap, 0..15.
 * @return the nominal output voltage in millivolts.
 */
uint32_t EPIC_VREF_MilliVolts(uint32_t vdd_mv,
                             VREF_RangeTypeDef range,
                             uint8_t value)
{
    value &= 0x0FU;
    if (range == VREF_RANGE_LOW) {
        /* VRR=1: CVREF = (VR<3:0>/24) × CVRSRC */
        return (vdd_mv * value) / 24U;
    } else {
        /* VRR=0: CVREF = 1/4 × CVRSRC + (VR<3:0>/32) × CVRSRC */
        return (vdd_mv / 4U) + (vdd_mv * value) / 32U;
    }
}
