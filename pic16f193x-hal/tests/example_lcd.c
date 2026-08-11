/**
 * LCD smoke test: init with 1:4 mux, confirm LCDCON state.
 * Expected register image (after init, PIC16F1937 with 24 segments):
 *   LCDCON = 0x83   (LCDEN=1, LMUX=11 = 1:4 mux)
 *   LCDSE0 = 0xFF, LCDSE1 = 0xFF, LCDSE2 = 0xFF (all segments enabled)
 */
#include "pic16f193x.h"
#include "pic16f193x_sfr.h"
#include "peripherals/pic16f193x_lcd.h"
#include "core/epic_harness.h"
extern void pic16f193x_harness_halt(void);
int main(void)
{
    epic_harness_init(1UL);
    LCD_HandleTypeDef lcd = LCD_HANDLE_DEFAULT;
    EPIC_LCD_Init(&lcd);
    uint8_t con = EPIC_REG8(PIC_REG_LCDCON);
    epic_harness_log("LCDCON=0x%02X\n", con);
    int pass = (con == (PIC_LCDCON_LCDEN | 0x03U));
    int rc = epic_harness_report(pass);
    pic16f193x_harness_halt();
    return rc;
}
