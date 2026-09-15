/*
 * EUSART1 + EUSART2 async TX smoke: verify BRG math, program 9600 baud
 * async on both modules, and transmit a byte on each. Host sim verifies
 * register programming; the mdb uart gate captures the actual TX bytes
 * from SIM.
 */

#include "pic18f6520_hal.h"
#include "pic18f6520_sfr.h"
#include "peripherals/pic18f6520_usart.h"
#include "core/pic18_irq.h"
#include "core/epic_harness.h"

/** Simulated run length (host only). */
#define SIM_CYCLES  5000UL

/**
 * @brief  Program EUSART1/EUSART2 async TX + verify + transmit.
 *
 *          PASS if BRG math gives SPBRG=129 (20MHz/16/9600-1), TXSTA
 *          programs async/BRGH/TXEN, and SPBRG reads back 129 on both
 *          instances.
 */
int main(void)
{
    epic_harness_init(SIM_CYCLES);
    uint8_t ok = 1U;

    /* BRG math: 20MHz, async, BRGH=1, 9600 baud.
     * (20e6 / (16*9600)) - 1 = 130.2 - 1 = 129. */
    uint16_t sp = USART_ComputeSPBRG(20000000UL, 9600UL,
                                     USART_MODE_ASYNCHRONOUS,
                                     USART_BRGH_HIGH);
    if (sp != 129U) ok = 0U;

    /* EUSART1: async, 8-bit, BRGH=1, SPBRG=129, TX only. */
    USART_HandleTypeDef h = USART_HANDLE_DEFAULT;
    h.Instance  = USART_INSTANCE_1;
    h.Mode      = USART_MODE_ASYNCHRONOUS;
    h.BaudHigh  = USART_BRGH_HIGH;
    h.SPBRG     = 129U;
    if (EPIC_USART_Init(&h) != EPIC_OK) ok = 0U;

    /* EUSART2: same, instance 2. */
    USART_HandleTypeDef h2 = USART_HANDLE_DEFAULT;
    h2.Instance = USART_INSTANCE_2;
    h2.Mode     = USART_MODE_ASYNCHRONOUS;
    h2.BaudHigh = USART_BRGH_HIGH;
    h2.SPBRG    = 129U;
    if (EPIC_USART_Init(&h2) != EPIC_OK) ok = 0U;

    /* Verify both TXSTA: async (SYNC=0), BRGH=1, TXEN=1. */
    uint8_t txsta1 = epic_sfr_read8(PIC_REG_TXSTA1);
    uint8_t txsta2 = epic_sfr_read8(PIC_REG_TXSTA2);
    if (txsta1 & PIC_TXSTA_SYNC) ok = 0U;
    if (!(txsta1 & PIC_TXSTA_BRGH)) ok = 0U;
    if (!(txsta1 & PIC_TXSTA_TXEN)) ok = 0U;
    if (txsta2 & PIC_TXSTA_SYNC) ok = 0U;
    if (!(txsta2 & PIC_TXSTA_BRGH)) ok = 0U;
    if (!(txsta2 & PIC_TXSTA_TXEN)) ok = 0U;

    /* Verify SPBRG=129 on both. */
    if (epic_sfr_read8(PIC_REG_SPBRG1) != 129U) ok = 0U;
    if (epic_sfr_read8(PIC_REG_SPBRG2) != 129U) ok = 0U;

    /* Transmit 'U' (0x55, alternating bits, scope-friendly) on both. */
    EPIC_USART_Transmit(USART_INSTANCE_1, 0x55U);
    EPIC_USART_Transmit(USART_INSTANCE_2, 0xAAU);

    for (uint32_t i = 0; epic_harness_running(i); i++)
    {
        epic_harness_tick();
    }

    epic_harness_log("EUSART1/2 async 9600 TX programmed (SPBRG=129).\n");
    return epic_harness_report(ok);
}
