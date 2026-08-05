/**
 * @file    example_eusart.c
 * @brief   EUSART smoke test: init at a known baud, transmit one byte,
 *          confirm control-register state. The §4 gate payload.
 *
 * @details
 *   FOSC=32MHz, BRGH=1 (divisor 16), target 9600 baud:
 *     SPBRG = (32000000 / (16*9600)) - 1 = 207 (0xCF).
 *
 *   Expected register image (after init + one Transmit):
 *     TXSTA   = 0x26   (TXEN=1, BRGH=1, TRMT=1)
 *     RCSTA   = 0x90   (SPEN=1, CREN=1)
 *     SPBRGL  = 0xCF
 *     BAUDCON = 0x40   (RCIDL=1, read-only; driver writes 0 but RCIDL stays)
 */

#include "pic16f193x.h"
#include "pic16f193x_sfr.h"
#include "peripherals/pic16f193x_usart.h"
#include "core/pic8_harness.h"

extern void pic16f193x_harness_halt(void);

#ifndef FOSC_HZ
#define FOSC_HZ 32000000UL
#endif

int main(void)
{
    pic8_harness_init(1UL);

    USART_HandleTypeDef usart = USART_HANDLE_DEFAULT;
    usart.SPBRG = (uint8_t)USART_ComputeSPBRG(FOSC_HZ, 9600U, USART_BRGH_HIGH);
    EPIC_USART_Init(&usart);
    EPIC_USART_Transmit(0x55U);

    /* Let the sim model TRMT=1 (shift register empty after TXEN). */
    pic8_harness_tick();

    uint8_t txsta = PIC8_REG8(PIC_REG_TXSTA);
    uint8_t rcsta = PIC8_REG8(PIC_REG_RCSTA);
    uint8_t spbrgl = PIC8_REG8(PIC_REG_SPBRGL);
    uint8_t baudcon = PIC8_REG8(PIC_REG_BAUDCON);

    pic8_harness_log("TXSTA=0x%02X RCSTA=0x%02X SPBRGL=0x%02X BAUDCON=0x%02X\n",
                      txsta, rcsta, spbrgl, baudcon);
    int pass = (txsta == 0x26U) && (rcsta == 0x90U) &&
               (spbrgl == 0xCFU) && (baudcon == 0x40U);
    int rc = pic8_harness_report(pass);
    pic16f193x_harness_halt();
    return rc;
}
