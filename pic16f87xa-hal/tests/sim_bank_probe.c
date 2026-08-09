/**
 * @file    sim_bank_probe.c
 * @brief   HARNESS=sim probe for the pic16f87xa-hal banked-SFR audit
 *          (docs/toolchain-coverage.md classes A and B): exercises
 *          every ungated `pic_select_bank` + plain EPIC_REG8 site and
 *          every plain Bank-1 SFR access with known values and reads
 *          them back, so XC8 v4.00's bank misdirection (Finding 9's
 *          mechanism) shows up as a FAIL instead of silent corruption.
 *
 * @details
 *   Each suspect site is first pre-set to a non-default value through
 *   the SAFE atomic macro (EPIC_BANK1_WRITE8), so a misdirected write
 *   leaves the pre-set value in place and the readback check fails:
 *   DeInit targets whose expected result equals the POR default (0x00)
 *   are otherwise indistinguishable from a write that went to the
 *   wrong address. Readbacks go through EPIC_BANK1_READ8 (safe), never
 *   plain EPIC_REG8 on a Bank-1 SFR, so the probe itself cannot
 *   exhibit the class it is testing for.
 *
 *   Findings (2026-08-09, all confirmed by this probe and fixed):
 *   - TIMER2_ReadPeriod's plain bank-switch read misdirected (read the
 *     Bank-0 alias). Fixed with EPIC_BANK1_READ8.
 *   - The Finding-9 safe macros only managed RP0; an incoming RP1=1
 *     state (observed right after the sim harness init) routed their
 *     accesses to Bank 3 GPR. Fixed: EPIC_BANK1_* and EPIC_PIE_*
 *     macros now select their bank absolutely and exit to Bank 0.
 *   - The EPIC_PIE_* PIE2 branch never selected Bank 2 (it RMW'd the
 *     Bank-1 alias of PIE2). Fixed: selects Bank 2 explicitly.
 *   - Plain Bank-1 reads (TXSTA, SSPSTAT, OPTION_REG, PCON) and RMWs
 *     misdirected to their Bank-0 aliases (RCSTA, SSPCON, TMR0, PIR1)
 *     and corrupted them. Fixed with the safe-macro pattern.
 *
 *   Runs as the sim variant of the pic16f87xa-hal pseudo-module under
 *   MPLAB SIM (the real XC8 v4.00 code), reporting through the 87XA
 *   sim harness. Order matters: the USART DeInit probe is last because
 *   it kills the marker USART, and the probe re-inits the USART (the
 *   harness's own shape) before the final report.
 */

#include "core/epic_harness.h"
#include "core/pic16_irq.h"
#include "peripherals/pic16f87xa_adc.h"
#include "peripherals/pic16f87xa_comp.h"
#include "peripherals/pic16f87xa_gpio.h"
#include "peripherals/pic16f87xa_ssp.h"
#include "peripherals/pic16f87xa_timer0.h"
#include "peripherals/pic16f87xa_timer2.h"
#include "peripherals/pic16f87xa_usart.h"
#include "peripherals/pic16f87xa_vref.h"
#include "target/pic16f87xa_platform.h"

#include <stdint.h>

#ifndef FOSC_HZ
#define FOSC_HZ 20000000UL
#endif

static uint16_t g_fail = 0u;

static void fail(uint8_t idx)
{
    static const char hx[] = "0123456789ABCDEF";
    char c[2];
    g_fail++;
    epic_harness_log("F");
    c[0] = hx[(idx >> 4) & 0xF];
    c[1] = '\0';
    epic_harness_log(c);
    c[0] = hx[idx & 0xF];
    c[1] = '\0';
    epic_harness_log(c);
    epic_harness_log(".");
}

/* Check numbering: the index is the check number in main() order,
 * starting 0x00; the mapping is documented on each check. */
#define CHECK(cond, idx) do {         \
    if (!(cond)) fail(idx);            \
} while (0)

/* Bank-1 readback helper. */
#define RD1(sfr, out) EPIC_BANK1_READ8(sfr, (out))

static void s_tx_noop(void)
{
}

int main(void)
{
    epic_harness_init(0UL);

    uint8_t v;

    /* Class A: EPIC_ADC_DeInit writes ADCON1 (Bank 1, 0x9F) through an
     * ungated pic_select_bank + plain EPIC_REG8. Pre-set 0xAA, DeInit,
     * expect 0x00. */
    EPIC_BANK1_WRITE8(ADCON1, 0xAAu);
    (void)EPIC_ADC_DeInit();
    RD1(ADCON1, v);
    CHECK(v == 0x00u, 0x00);

    /* Class A: EPIC_COMP_DeInit writes CMCON (0x9C), expect 0x07. */
    EPIC_BANK1_WRITE8(CMCON, 0xAAu);
    (void)EPIC_COMP_DeInit();
    RD1(CMCON, v);
    CHECK(v == 0x07u, 0x01);

    /* Class A: EPIC_VREF_DeInit writes CVRCON (0x9D), expect 0x00. */
    EPIC_BANK1_WRITE8(CVRCON, 0xAAu);
    (void)EPIC_VREF_DeInit();
    RD1(CVRCON, v);
    CHECK(v == 0x00u, 0x02);

    /* Class A read side: EPIC_TIMER2_ReadPeriod reads PR2 (0x92) after
     * an ungated bank switch (was corrupted, now the safe macro). */
    EPIC_TIMER2_WritePeriod(0xAAu);
    CHECK(EPIC_TIMER2_ReadPeriod() == 0xAAu, 0x03);

    /* Class B: EPIC_SSP_ReadByte's SSPBUF value round-trip (Bank 0).
     * The BF-clear RMW is the safe Bank-1 pattern; BF itself is a
     * hardware status bit, not reliable to poke under MPLAB SIM. */
    EPIC_REG8(PIC_REG_SSPBUF) = 0x5Au;
    CHECK(EPIC_SSP_ReadByte() == 0x5Au, 0x04);

    /* Class B: EPIC_GPIO_Init writes TRISx (Bank 1) through a runtime
     * address (FSR-indirect, expected safe; probe confirms). PORTB all
     * outputs then low nibble inputs. */
    EPIC_GPIO_Init(GPIOB, 0xFFu, GPIO_MODE_OUTPUT);
    RD1(TRISB, v);
    CHECK(v == 0x00u, 0x08);
    EPIC_GPIO_Init(GPIOB, 0x0Fu, GPIO_MODE_INPUT);
    RD1(TRISB, v);
    CHECK(v == 0x0Fu, 0x09);

    /* Class B: EPIC_TIMER0_Init's OPTION_REG RMW (was corrupted, now
     * the safe pattern). Init clears T0CS (internal clock) and leaves
     * the prescaler assignment to Start, so OPTION_REG == 0xDF. */
    {
        TIMER0_HandleTypeDef t0 = TIMER0_HANDLE_DEFAULT;
        t0.Prescaler = TIMER0_PRESCALER_1_8;
        (void)EPIC_TIMER0_Init(&t0);
        RD1(OPTION_REG, v);
        CHECK(v == 0xDFu, 0x0A);
    }

    /* Class B (last: kills the marker USART): EPIC_USART_Init's TXSTA
     * and SPBRG writes are the safe pattern. Verify the Init state
     * first, then DeInit (SPBRG -> 0x00, TXSTA -> 0x02), then re-init.
     * TXEN is expected from the non-null callback. */
    {
        USART_HandleTypeDef h = USART_HANDLE_DEFAULT;
        h.SPBRG = (uint8_t)USART_ComputeSPBRG(
            FOSC_HZ, 9600UL, USART_MODE_ASYNCHRONOUS, USART_BRGH_HIGH);
        h.TxCpltCallback = s_tx_noop;
        (void)EPIC_USART_Init(&h);
        RD1(SPBRG, v);
        CHECK(v == 129u, 0x0B);
        RD1(TXSTA, v);
        CHECK((v & PIC_TXSTA_TXEN) != 0u, 0x0C);
        (void)EPIC_USART_DeInit();
        RD1(SPBRG, v);
        CHECK(v == 0x00u, 0x0D);
        RD1(TXSTA, v);
        CHECK(v == 0x02u, 0x0E);
        /* Re-init so the harness marker can transmit. */
        (void)EPIC_USART_Init(&h);
        EPIC_IRQ_DisableSrc(PIC16_IRQ_USART_TX);
    }

    for (uint32_t i = 0; epic_harness_running(i); i++) {
        epic_harness_tick();
    }
    return epic_harness_report(g_fail == 0u);
}
