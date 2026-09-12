/* HARNESS=sim probe for the PIC16F83/84/84A banked-SFR audit: the
 * family's whole register file is two banks, and the probes cover each
 * access class the shared drivers use: Bank-0 plain writes (PORTA/B),
 * Bank-1 literal-token writes and reads (TRISA via pic_select_bank,
 * OPTION_REG via the GPIO pull-up path, EECON1 direct), the Bank-0
 * data pair (EEDATA/EEADR through EPIC_BANK0_*), and the INTCON
 * interrupt gates (TMR0IE, EEIE). Reports PASS/FAIL on RA0 (MODE=gpio
 * in scripts/sim-mdb-run.sh; no USART on this family). A bank
 * misdirection fails the gate instead of corrupting silently.
 *
 * The EEPROM and IRQ accessor calls are excluded for flash: at 1K
 * words the probe cannot also afford those driver call graphs, so the
 * bank access classes are driven directly through the same
 * literal-token macros the drivers use (the size trim the 628A probe
 * also records in its MANUAL). */

#include "core/epic_harness.h"
#include "peripherals/pic14_gpio.h"
#include "core/pic16_irq.h"
#include "target/pic16f83_84_platform.h"

#include <stdint.h>

/**
 * @brief Freeze-at-verdict hook, defined by the mdb harness (this probe
 *        only builds as the HARNESS=sim target, so the mdb harness is
 *        linked).
 */
extern void pic16f83_84_harness_halt(void);

#ifndef FOSC_HZ
#define FOSC_HZ 4000000UL
#endif

static uint16_t g_fail = 0u;
/**
 * @brief Bump the failure counter and log a marker line.
 * @param idx the check index (0x00..0x0F), logged as F characters.
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
 * @brief Run the banked-SFR access probes and report pass/fail on RA0.
 */
int main(void)
{
    uint8_t v = 0U;

    epic_harness_init(1000U);

    /* RA0 is the marker pin (armed by the harness); RA1..RA4 join the
     * output test. TRISA is Bank 1: write all-inputs then read back
     * through the literal-token macro. */
    pic_select_bank(1);
    EPIC_REG8(PIC_REG_TRISA) = 0x00U;
    pic_select_bank(0);
    RD1(TRISA, v);
    /* 0x07: TRISA bank-1 write landed (5 implemented pins, 0x1F). */
    CHECK((v & 0x1FU) == 0U, 0x07U);
    EPIC_GPIO_WritePort(GPIOA, 0xFFU);
    v = EPIC_REG8(PIC_REG_PORTA);
    /* RA4/T0CKI is open-drain: driving it high without a pull-up
     * reads 0, so the pin readback masks it out (RA0..RA3, same as
     * the 628A probe). */
    CHECK((v & 0x0FU) == 0x0FU, 0x06U);
    EPIC_GPIO_WritePort(GPIOA, 0x00U);
    v = EPIC_REG8(PIC_REG_PORTA);
    CHECK((v & 0x0FU) == 0x00U, 0x08U);

    /* OPTION_REG Bank-1 path via the GPIO driver's pull-up write (the
     * shared driver routes it through EPIC_BANK1_WRITE8): RBPU = 0
     * enables the pull-ups (nRBPU, active low); INTEDG reads back at
     * its POR value. */
    EPIC_GPIO_SetPullups(GPIO_PULLUP);
    RD1(OPTION_REG, v);
    CHECK((v & 0x40U) == 0x40U && (v & 0x80U) == 0U, 0x02U);

    /* EEPROM access classes: the data pair goes through EPIC_BANK0_*
     * (a variable-address fallback existed on the 628A and silently
     * misdirected banked writes; see that family's MANUAL.md), the
     * control pair through EPIC_BANK1_*. */
    EPIC_BANK0_WRITE8(EEDATA, 0xC3U);
    EPIC_BANK0_WRITE8(EEADR, 0x10U);
    /* 0x03/0x04: data-pair images land via the Bank-0 macros. */
    {
        uint8_t eedata = 0U;
        uint8_t eeadr = 0U;
        EPIC_BANK0_READ8(EEDATA, eedata);
        EPIC_BANK0_READ8(EEADR, eeadr);
        CHECK(eedata == 0xC3U, 0x03U);
        CHECK(eeadr == 0x10U, 0x04U);
    }
    /* 0x05: WR sets in EECON1 via the Bank-1 macro and reads back
     * through the same literal-token path (RD1). */
    EPIC_BANK1_WRITE8(EECON1, PIC_EECON1_WREN | PIC_EECON1_WR);
    RD1(EECON1, v);
    CHECK((v & PIC_EECON1_WR) != 0U, 0x05U);
    EPIC_BANK1_WRITE8(EECON1, 0x00U);
    /* 0x09: the write-complete flag reads through EECON1<4> itself
     * (the shared driver's PIR-less flag path reads the same register;
     * its accessor calls are trimmed here for flash). */
    RD1(EECON1, v);
    CHECK((v & PIC_EECON1_EEIF) == 0U, 0x09U);

    /* 0x0B/0x0C: interrupt gates land in INTCON (TMR0IE bit 5, EEIE
     * bit 6); the EEPROM row proves the split-residency table: its
     * gate is INTCON-resident while its flag lives in EECON1. */
    EPIC_IRQ_Enable(PIC16_IRQ_TMR0);
    v = EPIC_REG8(PIC_REG_INTCON);
    CHECK((v & 0x20U) != 0U, 0x0BU);
    EPIC_IRQ_Enable(PIC16_IRQ_EEPROM);
    v = EPIC_REG8(PIC_REG_INTCON);
    CHECK((v & 0x40U) != 0U, 0x0CU);
    /* Report once, then halt: XC8 restarts main() on return, and
     * epic_harness_init() would drive RA0 low again, flickering the
     * marker across the mdb `print PORTA` readback window. One
     * verdict. */
    (void)epic_harness_report(g_fail == 0U);
    pic16f83_84_harness_halt();
    return 0;
}
