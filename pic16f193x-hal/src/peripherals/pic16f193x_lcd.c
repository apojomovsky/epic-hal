/**
 * @file    pic16f193x_lcd.c
 * @brief   PIC16F193X LCD segment driver implementation.
 * @details Segment-to-register mapping derived from the installed
 *          DFP header (pic16f1937.h) _LCDDATAn_SEGxCOMy_POSN macros:
 *            data_reg_index = com * 3 + (seg / 8)
 *            bit_position   = seg % 8
 *          Verified against the first 30 entries of the DFP's mapping
 *          table. NOT the plan's placeholder arithmetic; this is the
 *          real DFP-derived formula.
 */
#include "peripherals/pic16f193x_lcd.h"

/* Branch-before-touch for the 12 LCDDATA registers. Each access is a
 * literal PIC_REG_* token, satisfying the Global Constraint. */
#define LCD_SET_BIT(reg_idx, bit_mask)                                   \
    do {                                                                 \
        if      ((reg_idx) == 0)  PIC8_BIT_SET(PIC8_REG8(PIC_REG_LCDDATA0), (bit_mask));  \
        else if ((reg_idx) == 1)  PIC8_BIT_SET(PIC8_REG8(PIC_REG_LCDDATA1), (bit_mask));  \
        else if ((reg_idx) == 2)  PIC8_BIT_SET(PIC8_REG8(PIC_REG_LCDDATA2), (bit_mask));  \
        else if ((reg_idx) == 3)  PIC8_BIT_SET(PIC8_REG8(PIC_REG_LCDDATA3), (bit_mask));  \
        else if ((reg_idx) == 4)  PIC8_BIT_SET(PIC8_REG8(PIC_REG_LCDDATA4), (bit_mask));  \
        else if ((reg_idx) == 5)  PIC8_BIT_SET(PIC8_REG8(PIC_REG_LCDDATA5), (bit_mask));  \
        else if ((reg_idx) == 6)  PIC8_BIT_SET(PIC8_REG8(PIC_REG_LCDDATA6), (bit_mask));  \
        else if ((reg_idx) == 7)  PIC8_BIT_SET(PIC8_REG8(PIC_REG_LCDDATA7), (bit_mask));  \
        else if ((reg_idx) == 8)  PIC8_BIT_SET(PIC8_REG8(PIC_REG_LCDDATA8), (bit_mask));  \
        else if ((reg_idx) == 9)  PIC8_BIT_SET(PIC8_REG8(PIC_REG_LCDDATA9), (bit_mask));  \
        else if ((reg_idx) == 10) PIC8_BIT_SET(PIC8_REG8(PIC_REG_LCDDATA10), (bit_mask)); \
        else                       PIC8_BIT_SET(PIC8_REG8(PIC_REG_LCDDATA11), (bit_mask)); \
    } while (0)

#define LCD_CLR_BIT(reg_idx, bit_mask)                                   \
    do {                                                                 \
        if      ((reg_idx) == 0)  PIC8_BIT_CLR(PIC8_REG8(PIC_REG_LCDDATA0), (bit_mask));  \
        else if ((reg_idx) == 1)  PIC8_BIT_CLR(PIC8_REG8(PIC_REG_LCDDATA1), (bit_mask));  \
        else if ((reg_idx) == 2)  PIC8_BIT_CLR(PIC8_REG8(PIC_REG_LCDDATA2), (bit_mask));  \
        else if ((reg_idx) == 3)  PIC8_BIT_CLR(PIC8_REG8(PIC_REG_LCDDATA3), (bit_mask));  \
        else if ((reg_idx) == 4)  PIC8_BIT_CLR(PIC8_REG8(PIC_REG_LCDDATA4), (bit_mask));  \
        else if ((reg_idx) == 5)  PIC8_BIT_CLR(PIC8_REG8(PIC_REG_LCDDATA5), (bit_mask));  \
        else if ((reg_idx) == 6)  PIC8_BIT_CLR(PIC8_REG8(PIC_REG_LCDDATA6), (bit_mask));  \
        else if ((reg_idx) == 7)  PIC8_BIT_CLR(PIC8_REG8(PIC_REG_LCDDATA7), (bit_mask));  \
        else if ((reg_idx) == 8)  PIC8_BIT_CLR(PIC8_REG8(PIC_REG_LCDDATA8), (bit_mask));  \
        else if ((reg_idx) == 9)  PIC8_BIT_CLR(PIC8_REG8(PIC_REG_LCDDATA9), (bit_mask));  \
        else if ((reg_idx) == 10) PIC8_BIT_CLR(PIC8_REG8(PIC_REG_LCDDATA10), (bit_mask)); \
        else                       PIC8_BIT_CLR(PIC8_REG8(PIC_REG_LCDDATA11), (bit_mask)); \
    } while (0)

#define LCD_WRITE_DATA(reg_idx, value)                                   \
    do {                                                                 \
        if      ((reg_idx) == 0)  PIC8_REG8(PIC_REG_LCDDATA0) = (uint8_t)(value);  \
        else if ((reg_idx) == 1)  PIC8_REG8(PIC_REG_LCDDATA1) = (uint8_t)(value);  \
        else if ((reg_idx) == 2)  PIC8_REG8(PIC_REG_LCDDATA2) = (uint8_t)(value);  \
        else if ((reg_idx) == 3)  PIC8_REG8(PIC_REG_LCDDATA3) = (uint8_t)(value);  \
        else if ((reg_idx) == 4)  PIC8_REG8(PIC_REG_LCDDATA4) = (uint8_t)(value);  \
        else if ((reg_idx) == 5)  PIC8_REG8(PIC_REG_LCDDATA5) = (uint8_t)(value);  \
        else if ((reg_idx) == 6)  PIC8_REG8(PIC_REG_LCDDATA6) = (uint8_t)(value);  \
        else if ((reg_idx) == 7)  PIC8_REG8(PIC_REG_LCDDATA7) = (uint8_t)(value);  \
        else if ((reg_idx) == 8)  PIC8_REG8(PIC_REG_LCDDATA8) = (uint8_t)(value);  \
        else if ((reg_idx) == 9)  PIC8_REG8(PIC_REG_LCDDATA9) = (uint8_t)(value);  \
        else if ((reg_idx) == 10) PIC8_REG8(PIC_REG_LCDDATA10) = (uint8_t)(value); \
        else                       PIC8_REG8(PIC_REG_LCDDATA11) = (uint8_t)(value); \
    } while (0)

HAL_StatusTypeDef HAL_LCD_Init(const LCD_HandleTypeDef *h)
{
    if (!h) return HAL_INVALID;
    if (h->Contrast > 0x07U) return HAL_INVALID;

    PIC8_REG8(PIC_REG_LCDCST) = (uint8_t)(h->Contrast & PIC_LCDCST_LCDCST_MASK);
    PIC8_REG8(PIC_REG_LCDREF) = 0x00U;
    PIC8_REG8(PIC_REG_LCDRL) = 0x00U;
    PIC8_REG8(PIC_REG_LCDPS) = 0x00U;
    PIC8_REG8(PIC_REG_LCDCON) = (uint8_t)(PIC_LCDCON_LCDEN | (h->MuxMode & PIC_LCDCON_LMUX_MASK));

    for (uint8_t i = 0U; i < 12U; i++) LCD_WRITE_DATA(i, 0x00U);

    PIC8_REG8(PIC_REG_LCDSE0) = 0xFFU;
    PIC8_REG8(PIC_REG_LCDSE1) = (PIC16F193X_FAMILY_LCD_SEGMENTS >= 16U) ? 0xFFU : 0x00U;
    PIC8_REG8(PIC_REG_LCDSE2) = (PIC16F193X_FAMILY_LCD_SEGMENTS >= 24U) ? 0xFFU : 0x00U;

    return HAL_OK;
}

HAL_StatusTypeDef HAL_LCD_DeInit(void)
{
    PIC8_REG8(PIC_REG_LCDCON) = 0x00U;
    for (uint8_t i = 0U; i < 12U; i++) LCD_WRITE_DATA(i, 0x00U);
    return HAL_OK;
}

HAL_StatusTypeDef HAL_LCD_SetSegment(uint8_t seg, uint8_t com, uint8_t on)
{
    if (seg >= PIC16F193X_FAMILY_LCD_SEGMENTS || com >= LCD_COMMONS) return HAL_INVALID;
    /* DFP-derived mapping: data_reg_index = com*3 + seg/8, bit = seg%8 */
    uint8_t reg_idx = (uint8_t)(com * 3U + (seg / 8U));
    uint8_t bit_mask = PIC8_BIT(seg % 8U);
    if (on) LCD_SET_BIT(reg_idx, bit_mask);
    else    LCD_CLR_BIT(reg_idx, bit_mask);
    return HAL_OK;
}

HAL_StatusTypeDef HAL_LCD_Clear(void)
{
    for (uint8_t i = 0U; i < 12U; i++) LCD_WRITE_DATA(i, 0x00U);
    return HAL_OK;
}

uint8_t HAL_LCD_IsActive(void)
{
    return (PIC8_REG8(PIC_REG_LCDPS) & PIC_LCDPS_LCDA) ? 1U : 0U;
}

void LCD_IRQHandler(void) {}
