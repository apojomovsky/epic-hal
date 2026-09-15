/*
 * EUSART async TX smoke: verify BRG math, program 9600 baud async, and
 * transmit a byte. Host sim verifies register programming; the mdb uart
 * gate captures the actual TX byte from SIM.
 */

#include "pic18f2520_hal.h"
#include "pic18f2520_sfr.h"
#include "peripherals/pic18f2520_usart.h"
#include "core/pic18_irq.h"
#include "core/epic_harness.h"

/** Simulated run length (host only). */
#define SIM_CYCLES  5000UL

/**
 * @brief  Program EUSART async TX + verify + transmit.
 *
 *          PASS if BRG math gives SPBRG=129 (20MHz/16/9600-1), TXSTA
 *          programs async/BRGH/TXEN, and SPBRG reads back 129.
 */
int main(void)
{
    epic_harness_init(SIM_CYCLES);
    uint8_t ok = 1U;

    /* BRG math: 20MHz, async, BRGH=1, 8-bit, 9600 baud.
     * (20e6 / (16*9600)) - 1 = 130.2 - 1 = 129. */
    uint16_t sp = USART_ComputeSPBRG(20000000UL, 9600UL,
                                     USART_MODE_ASYNCHRONOUS,
                                     USART_BRGH_HIGH, USART_BAUDGEN_8BIT);
    if (sp != 129U) ok = 0U;

    /* EUSART: async, 8-bit, BRGH=1, SPBRG=129, TX only. */
    USART_HandleTypeDef h;
    h.Mode           = USART_MODE_ASYNCHRONOUS;
    h.ClockSource    = USART_CLOCK_SLAVE;
    h.BaudHigh       = USART_BRGH_HIGH;
    h.BaudGen        = USART_BAUDGEN_8BIT;
    h.DataWidth      = USART_DATA_8BITS;
    h.SPBRG          = 129U;
    h.SPBRGH         = 0U;
    h.AddressDetect  = 0U;
    h.AutoBaud       = 0U;
    h.TxCpltCallback = NULL;
    h.RxCpltCallback = NULL;
    if (EPIC_USART_Init(&h) != EPIC_OK) ok = 0U;

    /* Verify TXSTA: async (SYNC=0), BRGH=1, TXEN=1. */
    uint8_t txsta = epic_sfr_read8(PIC_REG_TXSTA);
    if (txsta & PIC_TXSTA_SYNC) ok = 0U;
    if (!(txsta & PIC_TXSTA_BRGH)) ok = 0U;
    if (!(txsta & PIC_TXSTA_TXEN)) ok = 0U;

    /* Verify SPBRG=129. */
    if (epic_sfr_read8(PIC_REG_SPBRG) != 129U) ok = 0U;

    /* Transmit 'U' (0x55, alternating bits, scope-friendly). */
    EPIC_USART_Transmit(0x55U);

    for (uint32_t i = 0; epic_harness_running(i); i++)
    {
        epic_harness_tick();
    }

    epic_harness_log("EUSART async 9600 TX programmed (SPBRG=129).\n");
    return epic_harness_report(ok);
}
