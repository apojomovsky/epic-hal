/* 16F506 comparator implementation (DS41268D §7.0, §8.0). Literal
 * register tokens throughout (the §4.9 high-risk shape); no IRQ
 * hooks, this core has no vector. Only the 16F506 builds this file. */

#include "peripherals/pic16f5x_comp.h"

#if PIC16F5X_FAMILY_HAS_COMP_ADC

/**
 * @brief Initialize the comparators and reference from the handle.
 * @param h handle with enable, polarity and reference selection.
 * @return EPIC_OK on success, EPIC_INVALID if `h` is NULL.
 */
EPIC_StatusTypeDef EPIC_COMP_Init(const COMP_HandleTypeDef *h)
{
    uint8_t cm1;
    uint8_t cm2;
    uint8_t vr;

    if (h == NULL) return EPIC_INVALID;

    /* Enable + polarity, preserving the mux, Timer0-source, wake-up
     * and output-enable bits (Register 7-2/7-3). C1POL reads
     * inverted from its name (1 = not inverted), so an inverted
     * handle clears it. */
    cm1 = EPIC_REG8(PIC_REG_CM1CON0);
    if (h->C1Enabled) cm1 |= PIC_CM1CON0_C1ON;
    else              cm1 &= (uint8_t)~PIC_CM1CON0_C1ON;
    if (h->C1Inverted) cm1 &= (uint8_t)~PIC_CM1CON0_C1POL;
    else               cm1 |= PIC_CM1CON0_C1POL;
    EPIC_REG8(PIC_REG_CM1CON0) = cm1;

    cm2 = EPIC_REG8(PIC_REG_CM2CON0);
    if (h->C2Enabled) cm2 |= PIC_CM2CON0_C2ON;
    else              cm2 &= (uint8_t)~PIC_CM2CON0_C2ON;
    if (h->C2Inverted) cm2 &= (uint8_t)~PIC_CM2CON0_C2POL;
    else               cm2 |= PIC_CM2CON0_C2POL;
    EPIC_REG8(PIC_REG_CM2CON0) = cm2;

    /* The reference is fully owned: tap, range, pin output, enable. */
    vr = h->VrefTap & PIC_VRCON_VR_MASK;
    if (h->VrefRange)  vr |= PIC_VRCON_VRR;
    if (h->VrefOutput) vr |= PIC_VRCON_VROE;
    if (h->VrefEnabled) vr |= PIC_VRCON_VREN;
    EPIC_REG8(PIC_REG_VRCON) = vr;

    return EPIC_OK;
}

/**
 * @brief Disable both comparators and the reference.
 * @return EPIC_OK on success.
 */
EPIC_StatusTypeDef EPIC_COMP_DeInit(void)
{
    EPIC_REG8(PIC_REG_CM1CON0) &= (uint8_t)~PIC_CM1CON0_C1ON;
    EPIC_REG8(PIC_REG_CM2CON0) &= (uint8_t)~PIC_CM2CON0_C2ON;
    EPIC_REG8(PIC_REG_VRCON) = 0x00U;
    return EPIC_OK;
}

/**
 * @brief Read the C1 output bit.
 * @return 1 if the C1 output is high, 0 otherwise.
 */
uint8_t EPIC_COMP_C1Out(void)
{
    return (EPIC_REG8(PIC_REG_CM1CON0) & PIC_CM1CON0_C1OUT) ? 1U : 0U;
}

/**
 * @brief Read the C2 output bit.
 * @return 1 if the C2 output is high, 0 otherwise.
 */
uint8_t EPIC_COMP_C2Out(void)
{
    return (EPIC_REG8(PIC_REG_CM2CON0) & PIC_CM2CON0_C2OUT) ? 1U : 0U;
}

#endif /* PIC16F5X_FAMILY_HAS_COMP_ADC */
