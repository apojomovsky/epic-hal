/**
 * PIC16F193X LCD segment driver implementation. Segment-to-register
 * mapping derived from the installed DFP header (pic16f1937.h)
 * _LCDDATAn_SEGxCOMy_POSN macros: data_reg_index = com*3 + seg/8,
 * bit_position = seg%8.
 */
#include "peripherals/pic16f193x_lcd.h"

/* Branch-before-touch for the 12 LCDDATA registers. Each access is a
 * literal PIC_REG_* token, satisfying the Global Constraint. */
#define LCD_SET_BIT(reg_idx, bit_mask)                                   \
    do {                                                                 \
        if      ((reg_idx) == 0)  EPIC_BIT_SET(EPIC_REG8(PIC_REG_LCDDATA0), (bit_mask));  \
        else if ((reg_idx) == 1)  EPIC_BIT_SET(EPIC_REG8(PIC_REG_LCDDATA1), (bit_mask));  \
        else if ((reg_idx) == 2)  EPIC_BIT_SET(EPIC_REG8(PIC_REG_LCDDATA2), (bit_mask));  \
        else if ((reg_idx) == 3)  EPIC_BIT_SET(EPIC_REG8(PIC_REG_LCDDATA3), (bit_mask));  \
        else if ((reg_idx) == 4)  EPIC_BIT_SET(EPIC_REG8(PIC_REG_LCDDATA4), (bit_mask));  \
        else if ((reg_idx) == 5)  EPIC_BIT_SET(EPIC_REG8(PIC_REG_LCDDATA5), (bit_mask));  \
        else if ((reg_idx) == 6)  EPIC_BIT_SET(EPIC_REG8(PIC_REG_LCDDATA6), (bit_mask));  \
        else if ((reg_idx) == 7)  EPIC_BIT_SET(EPIC_REG8(PIC_REG_LCDDATA7), (bit_mask));  \
        else if ((reg_idx) == 8)  EPIC_BIT_SET(EPIC_REG8(PIC_REG_LCDDATA8), (bit_mask));  \
        else if ((reg_idx) == 9)  EPIC_BIT_SET(EPIC_REG8(PIC_REG_LCDDATA9), (bit_mask));  \
        else if ((reg_idx) == 10) EPIC_BIT_SET(EPIC_REG8(PIC_REG_LCDDATA10), (bit_mask)); \
        else                       EPIC_BIT_SET(EPIC_REG8(PIC_REG_LCDDATA11), (bit_mask)); \
    } while (0)

#define LCD_CLR_BIT(reg_idx, bit_mask)                                   \
    do {                                                                 \
        if      ((reg_idx) == 0)  EPIC_BIT_CLR(EPIC_REG8(PIC_REG_LCDDATA0), (bit_mask));  \
        else if ((reg_idx) == 1)  EPIC_BIT_CLR(EPIC_REG8(PIC_REG_LCDDATA1), (bit_mask));  \
        else if ((reg_idx) == 2)  EPIC_BIT_CLR(EPIC_REG8(PIC_REG_LCDDATA2), (bit_mask));  \
        else if ((reg_idx) == 3)  EPIC_BIT_CLR(EPIC_REG8(PIC_REG_LCDDATA3), (bit_mask));  \
        else if ((reg_idx) == 4)  EPIC_BIT_CLR(EPIC_REG8(PIC_REG_LCDDATA4), (bit_mask));  \
        else if ((reg_idx) == 5)  EPIC_BIT_CLR(EPIC_REG8(PIC_REG_LCDDATA5), (bit_mask));  \
        else if ((reg_idx) == 6)  EPIC_BIT_CLR(EPIC_REG8(PIC_REG_LCDDATA6), (bit_mask));  \
        else if ((reg_idx) == 7)  EPIC_BIT_CLR(EPIC_REG8(PIC_REG_LCDDATA7), (bit_mask));  \
        else if ((reg_idx) == 8)  EPIC_BIT_CLR(EPIC_REG8(PIC_REG_LCDDATA8), (bit_mask));  \
        else if ((reg_idx) == 9)  EPIC_BIT_CLR(EPIC_REG8(PIC_REG_LCDDATA9), (bit_mask));  \
        else if ((reg_idx) == 10) EPIC_BIT_CLR(EPIC_REG8(PIC_REG_LCDDATA10), (bit_mask)); \
        else                       EPIC_BIT_CLR(EPIC_REG8(PIC_REG_LCDDATA11), (bit_mask)); \
    } while (0)

#define LCD_WRITE_DATA(reg_idx, value)                                   \
    do {                                                                 \
        if      ((reg_idx) == 0)  EPIC_REG8(PIC_REG_LCDDATA0) = (uint8_t)(value);  \
        else if ((reg_idx) == 1)  EPIC_REG8(PIC_REG_LCDDATA1) = (uint8_t)(value);  \
        else if ((reg_idx) == 2)  EPIC_REG8(PIC_REG_LCDDATA2) = (uint8_t)(value);  \
        else if ((reg_idx) == 3)  EPIC_REG8(PIC_REG_LCDDATA3) = (uint8_t)(value);  \
        else if ((reg_idx) == 4)  EPIC_REG8(PIC_REG_LCDDATA4) = (uint8_t)(value);  \
        else if ((reg_idx) == 5)  EPIC_REG8(PIC_REG_LCDDATA5) = (uint8_t)(value);  \
        else if ((reg_idx) == 6)  EPIC_REG8(PIC_REG_LCDDATA6) = (uint8_t)(value);  \
        else if ((reg_idx) == 7)  EPIC_REG8(PIC_REG_LCDDATA7) = (uint8_t)(value);  \
        else if ((reg_idx) == 8)  EPIC_REG8(PIC_REG_LCDDATA8) = (uint8_t)(value);  \
        else if ((reg_idx) == 9)  EPIC_REG8(PIC_REG_LCDDATA9) = (uint8_t)(value);  \
        else if ((reg_idx) == 10) EPIC_REG8(PIC_REG_LCDDATA10) = (uint8_t)(value); \
        else                       EPIC_REG8(PIC_REG_LCDDATA11) = (uint8_t)(value); \
    } while (0)

/**
 * @brief Configure the LCD controller from the handle and enable it.
 * @param h handle with contrast and mux mode
 * @return EPIC_OK on success, EPIC_INVALID if `h` is NULL or the
 *         contrast is out of range
 */
EPIC_StatusTypeDef EPIC_LCD_Init(const LCD_HandleTypeDef *h)
{
    if (!h) return EPIC_INVALID;
    if (h->Contrast > 0x07U) return EPIC_INVALID;

    EPIC_REG8(PIC_REG_LCDCST) = (uint8_t)(h->Contrast & PIC_LCDCST_LCDCST_MASK);
    EPIC_REG8(PIC_REG_LCDREF) = 0x00U;
    EPIC_REG8(PIC_REG_LCDRL) = 0x00U;
    EPIC_REG8(PIC_REG_LCDPS) = 0x00U;
    EPIC_REG8(PIC_REG_LCDCON) = (uint8_t)(PIC_LCDCON_LCDEN | (h->MuxMode & PIC_LCDCON_LMUX_MASK));

    for (uint8_t i = 0U; i < 12U; i++) LCD_WRITE_DATA(i, 0x00U);

    EPIC_REG8(PIC_REG_LCDSE0) = 0xFFU;
    EPIC_REG8(PIC_REG_LCDSE1) = (PIC16F193X_FAMILY_LCD_SEGMENTS >= 16U) ? 0xFFU : 0x00U;
    EPIC_REG8(PIC_REG_LCDSE2) = (PIC16F193X_FAMILY_LCD_SEGMENTS >= 24U) ? 0xFFU : 0x00U;

    return EPIC_OK;
}

/**
 * @brief Disable the LCD controller and clear all segment data.
 * @return EPIC_OK on success
 */
EPIC_StatusTypeDef EPIC_LCD_DeInit(void)
{
    EPIC_REG8(PIC_REG_LCDCON) = 0x00U;
    for (uint8_t i = 0U; i < 12U; i++) LCD_WRITE_DATA(i, 0x00U);
    return EPIC_OK;
}

/**
 * @brief Set or clear one LCD segment.
 * @param seg segment number (0..PIC16F193X_FAMILY_LCD_SEGMENTS-1)
 * @param com common line (0..LCD_COMMONS-1)
 * @param on 1 to turn the segment on, 0 to turn it off
 * @return EPIC_OK on success, EPIC_INVALID if seg or com is out of range
 */
EPIC_StatusTypeDef EPIC_LCD_SetSegment(uint8_t seg, uint8_t com, uint8_t on)
{
    if (seg >= PIC16F193X_FAMILY_LCD_SEGMENTS || com >= LCD_COMMONS) return EPIC_INVALID;
    /* DFP-derived mapping: data_reg_index = com*3 + seg/8, bit = seg%8 */
    uint8_t reg_idx = (uint8_t)(com * 3U + (seg / 8U));
    uint8_t bit_mask = EPIC_BIT(seg % 8U);
    if (on) LCD_SET_BIT(reg_idx, bit_mask);
    else    LCD_CLR_BIT(reg_idx, bit_mask);
    return EPIC_OK;
}

/**
 * @brief Clear all LCD segment data.
 * @return EPIC_OK on success
 */
EPIC_StatusTypeDef EPIC_LCD_Clear(void)
{
    for (uint8_t i = 0U; i < 12U; i++) LCD_WRITE_DATA(i, 0x00U);
    return EPIC_OK;
}

/**
 * @brief Report whether the LCD module is active.
 * @return 1 if the LCD is active (LCDA set), 0 otherwise
 */
uint8_t EPIC_LCD_IsActive(void)
{
    return (EPIC_REG8(PIC_REG_LCDPS) & PIC_LCDPS_LCDA) ? 1U : 0U;
}

/**
 * @brief LCD interrupt handler (weak, override in user code).
 */
void LCD_IRQHandler(void) {}
