/**
 * @file    peripherals/pic16f193x_lcd.h
 * @brief   PIC16F193X LCD segment driver (DS41364B LCD chapter).
 * @details Segment-to-register mapping derived from the DFP header's
 *          _LCDDATAn_SEGxCOMy_POSN macros: data_reg_index = com*3 +
 *          seg/8, bit = seg%8. Full reference: MANUAL.md.
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

EPIC_StatusTypeDef EPIC_LCD_Init(const LCD_HandleTypeDef *h);
EPIC_StatusTypeDef EPIC_LCD_DeInit(void);
EPIC_StatusTypeDef EPIC_LCD_SetSegment(uint8_t seg, uint8_t com, uint8_t on);
EPIC_StatusTypeDef EPIC_LCD_Clear(void);
uint8_t EPIC_LCD_IsActive(void);

void LCD_IRQHandler(void) EPIC_WEAK;

#endif /* PIC16F193X_LCD_H */
