/* Shared PIC14 mid-range voltage-reference implementation (87XA CVRCON
 * parts and 88X/628A VRCON parts). Sources: DS39582B §13 (87XA),
 * DS40001291H §8.10 (88X). */

#include "peripherals/pic14_vref.h"

/**
 * @brief Initialize the voltage reference: program CVRCON from the
 *        handle.
 * @param h handle with Range, Value, OutputEnable, Enabled.
 * @return EPIC_OK on success, EPIC_INVALID if `h` is NULL.
 */
EPIC_StatusTypeDef EPIC_VREF_Init(const VREF_HandleTypeDef *h)
{
    if (!h) return EPIC_INVALID;

#if PIC14MIDRANGE_HAS_VRCON
    /* Build VRCON (Bank 1; 0x97 on 88X, 0x9F on 628A). VRR=1 selects
     * the low range on VRCON parts (per each datasheet's register
     * table), the opposite polarity of CVRCON. */
    uint8_t v = h->Value & PIC_VRCON_VR_MASK;
#if PIC14MIDRANGE_HAS_VRSS
    if (h->Source == VREF_SRC_VREFP_VREFN) v |= PIC_VRCON_VRSS;
#endif
    if (h->Range == VREF_RANGE_LOW)        v |= PIC_VRCON_VRR;
    if (h->OutputEnable)                   v |= PIC_VRCON_VROE;
    if (h->Enabled)                        v |= PIC_VRCON_VREN;
#ifdef EPIC_BANK1_WRITE8
    EPIC_BANK1_WRITE8(VRCON, v);
#else
    {
        uint8_t prev = (EPIC_REG8(PIC_REG_STATUS) >> 5) & 0x03U;
        pic_select_bank(1);
        EPIC_REG8(PIC_REG_VRCON) = v;
        pic_select_bank(prev);
    }
#endif
#else
    /* Build CVRCON (Bank 1, address 0x9D). */
    uint8_t v = h->Value & PIC_CVRCON_CVR_MASK;
    if (h->Range == VREF_RANGE_HIGH) v |= PIC_CVRCON_CVRR;
    if (h->OutputEnable)            v |= PIC_CVRCON_CVROE;
    if (h->Enabled)                 v |= PIC_CVRCON_CVREN;
#ifdef EPIC_BANK1_WRITE8
    /* See the family target platform header: a plain bank-switch write
     * here silently corrupts under XC8 v4.00. */
    EPIC_BANK1_WRITE8(CVRCON, v);
#else
    {
        uint8_t prev = (EPIC_REG8(PIC_REG_STATUS) >> 5) & 0x03U;
        pic_select_bank(1);
        EPIC_REG8(0x9DU) = v;
        pic_select_bank(prev);
    }
#endif
#endif
    return EPIC_OK;
}

/**
 * @brief De-initialize the voltage reference: clear CVRCON.
 * @return EPIC_OK on success.
 */
EPIC_StatusTypeDef EPIC_VREF_DeInit(void)
{
#if PIC14MIDRANGE_HAS_VRCON
    uint8_t prev = (EPIC_REG8(PIC_REG_STATUS) >> 5) & 0x03U;
    pic_select_bank(1);
    EPIC_REG8(PIC_REG_VRCON) = 0x00U;
    pic_select_bank(prev);
#else
    uint8_t prev = (EPIC_REG8(PIC_REG_STATUS) >> 5) & 0x03U;
    pic_select_bank(1);
    EPIC_REG8(0x9DU) = 0x00U;
    pic_select_bank(prev);
#endif
}

/**
 * @brief Compute the nominal output voltage in millivolts.
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
        /* CVREF = (VR<3:0>/24) × CVRSRC */
        return (vdd_mv * value) / 24U;
    } else {
        /* CVREF = 1/4 × CVRSRC + (VR<3:0>/32) × CVRSRC */
        return (vdd_mv / 4U) + (vdd_mv * value) / 32U;
    }
}
