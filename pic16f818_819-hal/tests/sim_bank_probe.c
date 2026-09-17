/* HARNESS=sim probe for the PIC16F818/819 banked-SFR and register-image
 * audit: this is the family's section-4 mdb gate, so it drives every
 * access class the shared drivers use and every peripheral image the
 * ladder configures, on the real target under MPLAB SIM. Reports
 * PASS/FAIL on RA0 (MODE=gpio in scripts/sim-mdb-run.sh; the family has
 * no USART). A bank misdirection or a miscompiled register write fails
 * the gate instead of corrupting silently.
 *
 * Hand-computed expected images (DS39598F, DFP-verified addresses):
 *   TRISA 0x85, TRISB 0x86 (Bank 1)   PORTA 0x05, PORTB 0x06 (Bank 0)
 *   OPTION_REG 0x81, PR2 0x92, SSPADD 0x93, SSPSTAT 0x94,
 *   ADRESL 0x9E, ADCON1 0x9F, PIE1 0x8C, PIE2 0x8D (Bank 1)
 *   EEDATA 0x10C, EEADR 0x10D (Bank 2)   EECON1 0x18C (Bank 3)
 *   ADCON0 0x1F (Bank 0)   CCPR1L 0x15, CCP1CON 0x17 (Bank 0)
 *   0x51 = ADCON0 for AN2 (CHS 010), Fosc/8 (ADCS 01), ADON;
 *   0x82 = ADCON1 for ADFM (bit 7) plus PCFG 0010 (AN0..AN4 analog
 *   against AVDD/AVSS, Register 11-2);
 *   0x2C = CCP1CON for PWM mode 1100 with duty LSBs 10, CCPR1L = 12 of
 *   PR2 = 99 for a 50% duty (Section 9.3.2).
 * TRISA5 and PORTA5 are MCLR-only pins whose latch/direction bit reads
 * fixed (Table 2-1 note 3), so the port checks mask them out; EECON2 is
 * not a physical register, so its write is exercised without a readback.
 * Flash budget is not a constraint here: only the 2 KW PIC16F819 is a
 * supported variant (the 16F818 is excluded in the manifest). */

#include "core/epic_harness.h"
#include "core/pic16_irq.h"
#include "peripherals/hal_adc.h"
#include "peripherals/pic14_gpio.h"
#include "pic16f818_819_sfr.h"
#include "target/pic16f818_819_platform.h"

#include <stdint.h>

/**
 * @brief Freeze-at-verdict hook, defined by the mdb harness (this probe
 *        only builds as the HARNESS=sim target, so the mdb harness is
 *        linked).
 */
extern void pic16f818_819_harness_halt(void);

#ifndef FOSC_HZ
#define FOSC_HZ 20000000UL
#endif

static uint16_t g_fail = 0u;

/**
 * @brief Bump the failure counter and log a marker line.
 * @param idx the check index, logged as idx+1 F characters so the
 *            failing check is countable in the capture.
 */
static void fail(uint8_t idx)
{
    uint8_t i = 0U;
    g_fail++;
    /* Raw-bytes channel (no printf): emit idx+1 F's. */
    for (i = 0U; i <= idx; i++)
    {
        epic_harness_log("F");
    }
    epic_harness_log("\n");
}
#define CHECK(cond, idx) do {         \
    if (!(cond)) fail(idx);            \
} while (0)

/* Bank-1 readback helper: the literal-token path the drivers use. */
#define RD1(sfr, out) EPIC_BANK1_READ8(sfr, (out))

/**
 * @brief Run the banked-SFR and register-image probes, then report
 *        pass/fail on RA0.
 */
int main(void)
{
    uint8_t v = 0U;
    uint8_t v2 = 0U;

    epic_harness_init(1000U);

    /* Bank 1, literal-token direction registers. RA5 is MCLR-only, so
     * only TRISA<4:0> is meaningful. */
    EPIC_BANK1_WRITE8(TRISA, 0x1FU);
    RD1(TRISA, v);
    CHECK((v & 0x1FU) == 0x1FU, 0x00U);
    EPIC_BANK1_WRITE8(TRISA, 0x00U);
    RD1(TRISA, v);
    CHECK((v & 0x1FU) == 0x00U, 0x01U);

    /* Bank 1 through the plain bank-switch path, and the Bank-0 latch
     * it enables. */
    pic_select_bank(1);
    EPIC_REG8(PIC_REG_TRISB) = 0x00U;
    pic_select_bank(0);
    v = EPIC_REG8(PIC_REG_TRISB);
    CHECK(v == 0x00U, 0x02U);
    EPIC_GPIO_WritePort(GPIOB, 0xFFU);
    v = EPIC_REG8(PIC_REG_PORTB);
    CHECK(v == 0xFFU, 0x03U);
    EPIC_GPIO_WritePort(GPIOB, 0x00U);
    v = EPIC_REG8(PIC_REG_PORTB);
    CHECK(v == 0x00U, 0x04U);
    /* The analog-capable pins must be made digital before a latch
     * readback means anything: at reset ADCON1 = 0x00 selects PCFG 0000
     * (every AN pin analog), and an analog pin's digital input reads 0
     * (Section 11.3). PCFG 011x selects all-digital. RA4/AN4/T0CKI is a
     * Schmitt Trigger input with a full CMOS output driver on this die
     * (Section 5.0, Table 5-1), unlike the open-drain RA4 of the 83/84
     * sibling, so RA0..RA4 all read back their latch. */
    EPIC_BANK1_WRITE8(ADCON1, 0x06U);
    RD1(ADCON1, v);
    CHECK((v & PIC_ADCON1_PCFG_MASK) == 0x06U, 0x05U);
    EPIC_GPIO_WritePort(GPIOA, 0x1FU);
    v = EPIC_REG8(PIC_REG_PORTA);
    CHECK((v & 0x1FU) == 0x1FU, 0x26U);
    EPIC_GPIO_WritePort(GPIOA, 0x00U);
    v = EPIC_REG8(PIC_REG_PORTA);
    CHECK((v & 0x1FU) == 0x00U, 0x27U);

    /* OPTION_REG through the GPIO driver's pull-up write: RBPU = 0
     * enables the PORTB pull-ups (active-low). */
    EPIC_GPIO_SetPullups(GPIO_PULLUP);
    RD1(OPTION_REG, v);
    CHECK((v & 0x80U) == 0x00U, 0x06U);

    /* Bank 1 literal-token RMW on the remaining driver-owned registers:
     * write, read back through the same macro. */
    EPIC_BANK1_WRITE8(PR2, 99U);
    RD1(PR2, v);
    CHECK(v == 99U, 0x07U);
    EPIC_BANK1_WRITE8(SSPADD, 39U);
    RD1(SSPADD, v);
    CHECK(v == 39U, 0x08U);
    EPIC_BANK1_WRITE8(SSPSTAT, PIC_SSPSTAT_CKE);
    RD1(SSPSTAT, v);
    CHECK(v == PIC_SSPSTAT_CKE, 0x09U);
    EPIC_BANK1_WRITE8(ADRESL, 0xA3U);
    RD1(ADRESL, v);
    CHECK(v == 0xA3U, 0x0AU);
    EPIC_BANK1_WRITE8(ADCON1, PIC_ADCON1_ADFM | 0x02U);
    RD1(ADCON1, v);
    CHECK(v == 0x82U, 0x0BU);

    /* PIE1/PIE2 arming through the interrupt API (the iorwf/andwf asm
     * path on target), each bit read back through the banked reader. */
    EPIC_IRQ_Enable(PIC16_IRQ_TMR1);
    RD1(PIE1, v);
    CHECK((v & PIC_PIE1_TMR1IE) != 0U, 0x0CU);
    EPIC_IRQ_Enable(PIC16_IRQ_TMR2);
    RD1(PIE1, v);
    CHECK((v & PIC_PIE1_TMR2IE) != 0U, 0x0DU);
    EPIC_IRQ_Enable(PIC16_IRQ_CCP1);
    RD1(PIE1, v);
    CHECK((v & PIC_PIE1_CCP1IE) != 0U, 0x0EU);
    EPIC_IRQ_Enable(PIC16_IRQ_SSP);
    RD1(PIE1, v);
    CHECK((v & PIC_PIE1_SSPIE) != 0U, 0x0FU);
    EPIC_IRQ_Enable(PIC16_IRQ_ADC);
    RD1(PIE1, v);
    CHECK((v & PIC_PIE1_ADIE) != 0U, 0x10U);
    EPIC_IRQ_DisableSrc(PIC16_IRQ_ADC);
    RD1(PIE1, v);
    CHECK((v & PIC_PIE1_ADIE) == 0U, 0x11U);
    EPIC_IRQ_Enable(PIC16_IRQ_EEPROM);
    RD1(PIE2, v);
    CHECK((v & PIC_PIE2_EEIE) != 0U, 0x12U);

    /* INTCON residents, the Bank-0 gate class. */
    EPIC_IRQ_Enable(PIC16_IRQ_RB);
    v = EPIC_REG8(PIC_REG_INTCON);
    CHECK((v & PIC_INTCON_RBIE) != 0U, 0x13U);
    EPIC_IRQ_Enable(PIC16_IRQ_INT);
    v = EPIC_REG8(PIC_REG_INTCON);
    CHECK((v & PIC_INTCON_INTE) != 0U, 0x14U);
    EPIC_IRQ_Enable(PIC16_IRQ_TMR0);
    v = EPIC_REG8(PIC_REG_INTCON);
    CHECK((v & PIC_INTCON_TMR0IE) != 0U, 0x15U);

    /* Flag residency: ADIF is PIR1<6>, EEIF is PIR2<4>, so the EEPROM
     * row proves the pir_is_pir2 selection in the shared IRQ table. */
    EPIC_IRQ_ClearFlag(PIC16_IRQ_ADC);
    CHECK(EPIC_IRQ_GetFlag(PIC16_IRQ_ADC) == 0U, 0x16U);
    EPIC_REG8(PIC_REG_PIR1) = PIC_PIR1_ADIF;
    CHECK(EPIC_IRQ_GetFlag(PIC16_IRQ_ADC) == 1U, 0x17U);
    EPIC_IRQ_ClearFlag(PIC16_IRQ_ADC);
    CHECK(EPIC_IRQ_GetFlag(PIC16_IRQ_ADC) == 0U, 0x18U);
    EPIC_REG8(PIC_REG_PIR2) = PIC_PIR2_EEIF;
    CHECK(EPIC_IRQ_GetFlag(PIC16_IRQ_EEPROM) == 1U, 0x19U);
    EPIC_IRQ_ClearFlag(PIC16_IRQ_EEPROM);
    CHECK(EPIC_IRQ_GetFlag(PIC16_IRQ_EEPROM) == 0U, 0x1AU);

    /* EEPROM access classes: the data pair rides Bank 2 and the control
     * pair Bank 3 (the 87XA placement, not the 628A Bank-1 one). Only
     * the plain WREN bit is asserted: WR is a hardware-cleared control
     * bit, and the EECON2 unlock pair starts a self-timed write that
     * MPLAB SIM never completes, which would leave WR latched and make
     * the readback a simulator artifact rather than a bank check. */
    EPIC_BANK2_WRITE8(EEDATA, 0xC3U);
    EPIC_BANK2_WRITE8(EEADR, 0x10U);
    EPIC_BANK2_READ8(EEDATA, v);
    EPIC_BANK2_READ8(EEADR, v2);
    CHECK(v == 0xC3U, 0x1BU);
    CHECK(v2 == 0x10U, 0x1CU);
    EPIC_BANK3_WRITE8(EECON1, PIC_EECON1_WREN);
    EPIC_BANK3_READ8(EECON1, v);
    CHECK((v & PIC_EECON1_WREN) != 0U, 0x1DU);
    EPIC_BANK3_WRITE8(EECON1, 0x00U);
    EPIC_BANK3_READ8(EECON1, v);
    CHECK(v == 0x00U, 0x1EU);
    /* The write-unlock pair goes out through the same Bank-3 macro with
     * EECON1 clear, so the sequence is inert and the access is covered
     * without latching a write cycle (EECON2 is not a readable
     * register, so there is nothing to read back). */
    EPIC_BANK3_WRITE8(EECON2, 0x55U);
    EPIC_BANK3_WRITE8(EECON2, 0xAAU);

    /* ADC image: AN2, Fosc/8, right-justified, PCFG 0010. */
    {
        ADC_HandleTypeDef h = ADC_HANDLE_DEFAULT;
        h.Channel      = ADC_CHANNEL_AN2;
        h.ClockSource  = ADC_CLOCK_FOSC_8;
        h.ResultFormat = ADC_FORMAT_RIGHT;
        h.Reference    = ADC_REFERENCE_VDD_VSS_5CH;
        CHECK(EPIC_ADC_Init(&h) == EPIC_OK, 0x1FU);
        v = EPIC_REG8(PIC_REG_ADCON0);
        CHECK(v == 0x51U, 0x20U);
        RD1(ADCON1, v);
        CHECK(v == 0x82U, 0x21U);
    }

    /* CCP1 image: Timer2 period 99 and a 50% PWM duty. The image is
     * written through the same literal addresses and EPIC_REG8/BANK1
     * forms the CCP and Timer2 drivers use (their own call graphs are
     * host-verified; this gate's job is the target-side addressing). */
    EPIC_BANK1_WRITE8(PR2, 99U);
    EPIC_REG8(PIC_REG_T2CON) = 0x04U;
    EPIC_REG8(PIC_REG_CCP1RL) = 12U;
    EPIC_REG8(PIC_REG_CCP1CON) = PIC_CCP_CCPX_M3 | PIC_CCP_CCPX_M2 |
                                 PIC_CCP_CCPX_X;
    RD1(PR2, v);
    CHECK(v == 99U, 0x22U);
    v = EPIC_REG8(PIC_REG_T2CON);
    CHECK(v == 0x04U, 0x23U);
    v = EPIC_REG8(PIC_REG_CCP1RL);
    CHECK(v == 12U, 0x24U);
    v = EPIC_REG8(PIC_REG_CCP1CON);
    CHECK(v == 0x2CU, 0x25U);

    /* Report once, then halt: XC8 restarts main() on return, and
     * epic_harness_init() would drive RA0 low again, flickering the
     * marker across the mdb `print PORTA` readback window. One verdict.
     * ADCON1 goes back to all-digital first: the ADC image above left
     * PCFG 0010, and an analog-selected RA0/AN0 reads 0 through its
     * digital input path (Section 11.3), which would mask a PASS. */
    EPIC_BANK1_WRITE8(ADCON1, 0x06U);
    (void)epic_harness_report(g_fail == 0U);
    pic16f818_819_harness_halt();
    return 0;
}
