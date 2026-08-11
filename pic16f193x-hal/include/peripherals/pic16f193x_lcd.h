/**
 * PIC16F193X LCD segment driver (DS41364B LCD chapter). Segment-to-register
 * mapping derived from the DFP header's _LCDDATAn_SEGxCOMy_POSN macros:
 * data_reg_index = com*3 + seg/8, bit = seg%8. Full reference: MANUAL.md.
 */
#ifndef PIC16F193X_LCD_H
#define PIC16F193X_LCD_H
#include "pic16f193x.h"
#include "pic16f193x_sfr.h"

#define LCD_COMMONS 4U

typedef struct {
    uint8_t Contrast;
    uint8_t MuxMode;
} LCD_HandleTypeDef;

#define LCD_HANDLE_DEFAULT { .Contrast = 0U, .MuxMode = 3U }

/**
 * @brief  Configure the LCD module from the handle: contrast (LCDCST),
 *         mux mode and LCDEN (LCDCON), then clear all segment data and
 *         enable every segment the device supports (LCDSE0-2).
 *
 * @param  h  handle with Contrast (0..7) and MuxMode (0..3)
 * @return EPIC_OK on success, EPIC_INVALID if `h` is NULL or the
 *         contrast exceeds 7
 */
EPIC_StatusTypeDef EPIC_LCD_Init(const LCD_HandleTypeDef *h);

/**
 * @brief  Disable the LCD module and clear all 12 LCDDATA registers.
 *
 * @return EPIC_OK
 */
EPIC_StatusTypeDef EPIC_LCD_DeInit(void);

/**
 * @brief  Set or clear one LCD segment. Segment-to-register mapping:
 *         data_reg_index = com*3 + seg/8, bit = seg%8.
 *
 * @param  seg  segment number, 0..PIC16F193X_FAMILY_LCD_SEGMENTS-1
 * @param  com  common line, 0..LCD_COMMONS-1
 * @param  on   non-zero turns the segment on, zero turns it off
 * @return EPIC_OK on success, EPIC_INVALID if `seg` or `com` is out of
 *         range
 */
EPIC_StatusTypeDef EPIC_LCD_SetSegment(uint8_t seg, uint8_t com, uint8_t on);

/**
 * @brief  Clear all 12 LCDDATA registers (all segments off).
 *
 * @return EPIC_OK
 */
EPIC_StatusTypeDef EPIC_LCD_Clear(void);

/**
 * @brief  Report whether the LCD module is running (LCDPS<LCDA>).
 *
 * @return 1 if the LCD is active, 0 otherwise
 */
uint8_t EPIC_LCD_IsActive(void);

/** @brief Weak LCD interrupt handler; override in user code. */
void LCD_IRQHandler(void) EPIC_WEAK;

#endif /* PIC16F193X_LCD_H */
