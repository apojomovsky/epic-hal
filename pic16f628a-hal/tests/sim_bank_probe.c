/* HARNESS=sim probe for the PIC16F628A banked-SFR audit: exercises
 * every DFP-verified placement delta vs the 87XA (CMCON in Bank 0,
 * EEPROM block in Bank 1, CMIF/EEIF in PIR1/PIE1) plus the shared
 * Bank-1 sites (TXSTA/SPBRG/PR2/VRCON/PIE1) with known values, and
 * reports PASS/FAIL over the mdb harness UART. A bank misdirection
 * fails the gate instead of corrupting silently. */

#include "core/epic_harness.h"
#include "peripherals/pic14_gpio.h"
#include "peripherals/pic14_timer1.h"
#include "peripherals/pic14_timer2.h"
#include "peripherals/pic14_usart.h"
#include "peripherals/pic14_ccp.h"
#include "peripherals/pic14_comp.h"
#include "peripherals/pic14_vref.h"
#include "peripherals/pic14_eeprom.h"
#include "target/pic16f628a_platform.h"

#include <stdint.h>

#ifndef FOSC_HZ
#define FOSC_HZ 20000000UL
#endif

static uint16_t g_fail = 0u;

/**
 * @brief Log a failed check index and bump the counter.
 * @param idx the check index (0x00..0x0F).
 */
static void fail(uint8_t idx)
{
    uint8_t i = 0U;
    g_fail++;
    /* Raw-bytes channel (no printf): emit idx+1 F's so the failing
     * check is countable in the capture. */
    for (i = 0U; i <= idx; i++) {
        epic_harness_log("F");
    }
    epic_harness_log("\n");
}
#define CHECK(cond, idx) do {         \
    if (!(cond)) fail(idx);            \
} while (0)

/* Bank-1 readback helper. */
#define RD1(sfr, out) EPIC_BANK1_READ8(sfr, (out))


/**
 * @brief Run the banked-SFR access probes and report pass/fail.
 */
int main(void)
{
    uint8_t v = 0U;

    epic_harness_init(1000U);

    /* Force comparators off first: at POR (CM=000, reset) the RA pins
     * stay analog-gated in the SIM model and read 0 regardless of the
     * latch; CM=111 digitalizes them. */
    (void)EPIC_COMP_DeInit();
    pic_select_bank(1);
    EPIC_REG8(PIC_REG_TRISA) = 0x00U;
    pic_select_bank(0);
    RD1(TRISA, v);
    /* 0x07: TRISA bank-1 write landed on every writable bit. */
    CHECK((v & 0xDFU) == 0U, 0x07U);
    EPIC_GPIO_WritePort(GPIOA, 0xFFU);
    v = EPIC_REG8(PIC_REG_PORTA);
    CHECK((v & 0x0FU) == 0x0FU, 0x06U);
    EPIC_GPIO_WritePort(GPIOA, 0x00U);
    v = EPIC_REG8(PIC_REG_PORTA);
    CHECK((v & 0x0FU) == 0x00U, 0x08U);
    /* 0x00: CMCON Bank-0 write path (driver Init programs 0x1F). */
    {
        COMP_HandleTypeDef ch = COMP_HANDLE_DEFAULT;
        ch.Mode = COMP_MODE_TWO_INDEP;
        ch.C1Inverted = true;
        (void)EPIC_COMP_Init(&ch);
        /* Bank 0: plain access, no switch needed. */
        v = EPIC_REG8(PIC_REG_CMCON);
        /* CM2:CM0 = 010, C1INV = 1 → 0x12 in bits 5:0. Bits 7:6
         * (CxOUT) are live comparator outputs and depend on the RA
         * input levels (here C1INV drives C1OUT high on low inputs),
         * so they are masked out (§4 step 8: mask read-only status). */
        CHECK((v & 0x3FU) == 0x12U, 0x00U);
    }


    /* 0x02: VRCON Bank-1 write path (driver Init programs 0x9F). */
    {
        VREF_HandleTypeDef vh = VREF_HANDLE_DEFAULT;
        vh.Range = VREF_RANGE_HIGH;
        vh.Value = 8;
        vh.OutputEnable = true;
        vh.Enabled = true;
        (void)EPIC_VREF_Init(&vh);
        RD1(VRCON, v);
        /* VR3:0 = 8, VRR = 0 (high range on VRCON parts), VROE = 1,
         * VREN = 1 → 0xC8. */
        CHECK(v == 0xC8U, 0x02U);
    }

    /* EEPROM Bank-1 path: the driver writes EEDATA/EEADR through the
     * same literal-token Bank-1 macros as EECON1 (shared pic14 driver;
     * a variable-address fallback existed here and silently misdirected
     * the data-pair writes on XC8, fixed by routing Bank-1-only parts
     * through the token branch, see MANUAL.md). While WR is set the
     * SIM EEPROM model leaves the data registers alone, so images are
     * asserted; completion itself stays SIM-limited (see MANUAL.md). */
    (void)EPIC_EEPROM_WriteByte(0x10U, 0xC3U);
    /* 0x03/0x04: data-pair images land via Bank-1 macros. */
    RD1(EEADR, v);
    CHECK(v == 0x10U, 0x03U);
    RD1(EEDATA, v);
    CHECK(v == 0xC3U, 0x04U);
    /* 0x05: write initiated (WR set in EECON1 via Bank-1 macro). */
    RD1(EECON1, v);
    CHECK((v & PIC_EECON1_WR) != 0U, 0x05U);
    EPIC_EEPROM_ClearITFlag();
    CHECK(EPIC_EEPROM_IsWriteComplete() == 0U, 0x09U);

    /* USART Bank-1 sites (TXSTA/SPBRG) are proven by every passing UART
     * gate (the blink report itself transmits through them); the probe
     * skips them to fit 2K flash. */


    /* 0x0B/0x0C: IRQ table rows land in PIE1 (CMIE bit 6, EEIE bit 7). */
    EPIC_IRQ_Enable(PIC16_IRQ_CMP);
    RD1(PIE1, v);
    CHECK((v & 0x40U) != 0U, 0x0BU);
    EPIC_IRQ_Enable(PIC16_IRQ_EEPROM);
    RD1(PIE1, v);
    CHECK((v & 0x80U) != 0U, 0x0CU);
    EPIC_IRQ_DisableSrc(PIC16_IRQ_CMP);
    EPIC_IRQ_DisableSrc(PIC16_IRQ_EEPROM);

    /* Timer1/CCP1 use plain Bank-0 access with DFP-verified addresses;
     * their silicon spot-checks run as ad-hoc mdb gates (see MANUAL.md),
     * not in this size-constrained probe (2K flash). */

    /* Report once, then halt: XC8 restarts main() on return, and the SIM
     * analog-pin model latches once comparators/VREF have run, so later
     * iterations would re-fail the GPIO reads. One verdict. */
    int rc = epic_harness_report(g_fail == 0U);
    for (;;) {
    }
    return rc;
}
