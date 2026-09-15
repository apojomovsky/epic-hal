/*
 * EUSART async TX smoke: verify BRG math (Table 16-1), program 9600 baud
 * async, and transmit a byte. Host sim verifies register programming; the
 * mdb gate reads the programmed TXSTA/BAUDCTL/SPBRG registers back.
 * Target-compatible (no host-only sim hooks): one source builds for both.
 */

#include "pic18f1320.h"
#include "pic18f1320_sfr.h"
#include "peripherals/pic18f1320_usart.h"
#include "core/pic18_irq.h"
#include "core/epic_harness.h"

/** Simulated run length (host only). */
#define SIM_CYCLES  5000UL

/**
 * @brief  Program EUSART async TX + verify + transmit.
 *
 *          PASS if BRG math gives SPBRG=129 (20MHz/16/9600-1), TXSTA
 *          programs async/BRGH, and SPBRG reads back 129. (No TXEN here:
 *          no TX callback registered, so the driver leaves TXEN clear, the
 *          caller setting it to actually transmit.)
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

    /* Sync 20MHz, 1MHz baud: divisor 4 -> X = (20e6/4e6)-1 = 4. */
    sp = USART_ComputeSPBRG(20000000UL, 1000000UL,
                            USART_MODE_SYNCHRONOUS,
                            USART_BRGH_LOW, USART_BAUDGEN_8BIT);
    if (sp != 4U) ok = 0U;

    /* 16-bit BRG range: 16MHz, async, BRGH=0, BRG16=1, 1200 baud
     * divisor 16 -> X = (16e6/(16*1200))-1 = 832, only reachable in
     * 16-bit mode. */
    sp = USART_ComputeSPBRG(16000000UL, 1200UL,
                            USART_MODE_ASYNCHRONOUS,
                            USART_BRGH_HIGH, USART_BAUDGEN_8BIT);
    if (sp != 0xFFFFU) ok = 0U;   /* 832 > 255 -> overflow. */
    sp = USART_ComputeSPBRG(16000000UL, 1200UL,
                            USART_MODE_ASYNCHRONOUS,
                            USART_BRGH_LOW, USART_BAUDGEN_16BIT);
    if (sp != 832U) ok = 0U;

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

    /* Verify TXSTA: reset 0x02 (TRMT) | BRGH(bit2) = 0x06. No TXEN
     * (no callback). */
    uint8_t txsta = epic_sfr_read8(PIC_REG_TXSTA);
    if (!(txsta & PIC_TXSTA_BRGH)) ok = 0U;
    if (txsta & PIC_TXSTA_SYNC) ok = 0U;

    /* Verify RCSTA: SPEN(bit7) only, no CREN -> 0x80. */
    if (epic_sfr_read8(PIC_REG_RCSTA) != PIC_RCSTA_SPEN) ok = 0U;

    /* Verify BAUDCTL: BRG16=0, no ABDEN (the writable bits). RCIDL
     * (bit 6) is read-only and hardware-set whenever the receiver is idle
     * (DS39605F Register 16-3); mask it out of the comparison per the
     * section-4 read-only-bit rule. */
    if ((epic_sfr_read8(PIC_REG_BAUDCTL) & (PIC_BAUDCTL_BRG16 | PIC_BAUDCTL_ABDEN)) != 0x00U) ok = 0U;

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
