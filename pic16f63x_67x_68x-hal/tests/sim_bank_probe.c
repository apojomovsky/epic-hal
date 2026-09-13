/* HARNESS=sim probe for the PIC16F63x/67x/68x banked-SFR audit: runs
 * every banked access site the exemplar tier uses with known values
 * and reads them back, so XC8 v4.00 bank misdirection (the PIC16
 * Finding 9 mechanism, see pic16f87xa-hal/README.md) fails the gate
 * instead of corrupting silently. SFRs spread across all four banks
 * here (Bank 1: OPTION/TRIS/PIE/PCON/OSCCON/WPUA/IOCA/WDTCON; Bank 2:
 * EEDATA/EEADR/WPUB/IOCB/VRCON/CMx/ANSEL; Bank 3: EECON1/EECON2/
 * SRCON), so the probe covers each bank through the same literal-token
 * macros the drivers use. Reports PASS/FAIL on RA0 (MODE=gpio in
 * scripts/sim-mdb-run.sh; no USART on this family). */

#include "core/epic_harness.h"
#include "core/pic16_irq.h"
#include "core/pic14_wdt_sleep.h"
#include "peripherals/pic16f63x_67x_68x_gpio.h"
#include "peripherals/pic16f63x_67x_68x_comp.h"
#include "peripherals/pic14_timer0.h"
#include "peripherals/pic14_timer1.h"
#include "peripherals/pic14_eeprom.h"
#include "target/pic16f63x_67x_68x_platform.h"

#include <stdint.h>

/**
 * @brief Freeze-at-verdict hook, defined by the mdb harness (this probe
 *        only builds as the HARNESS=sim target, so the mdb harness is
 *        linked).
 */
extern void pic16f63x_67x_68x_harness_halt(void);

#ifndef FOSC_HZ
#define FOSC_HZ 4000000UL
#endif

static uint16_t g_fail = 0u;
/**
 * @brief Bump the failure counter and log a marker line.
 * @param idx the check index (0x00..0x14), logged as F characters.
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

/* Banked readback helpers. */
#define RD1(sfr, out) EPIC_BANK1_READ8(sfr, (out))
#define RD2(sfr, out) EPIC_BANK2_READ8(sfr, (out))
#define RD3(sfr, out) EPIC_BANK3_READ8(sfr, (out))

/**
 * @brief Run the banked-SFR access probes and report pass/fail on RA0.
 */
int main(void)
{
    uint8_t v = 0U;

    epic_harness_init(1000U);

    /* Bank 1, TRIS: write all-outputs through pic_select_bank, read
     * back through the literal-token macro. */
    pic_select_bank(1);
    EPIC_REG8(PIC_REG_TRISB) = 0x00U;
    pic_select_bank(0);
    RD1(TRISB, v);
    /* 0x00: TRISB bank-1 write landed (RB4..RB7, 0xF0). */
    CHECK((v & 0xF0U) == 0U, 0x00U);

    /* Bank 0, PORT: the latch write lands and reads back. */
    EPIC_GPIO_WritePort(GPIOB, 0xFFU);
    v = EPIC_REG8(PIC_REG_PORTB);
    CHECK((v & 0xF0U) == 0xF0U, 0x01U);
    EPIC_GPIO_WritePort(GPIOB, 0x00U);

    /* Bank 1, OPTION via the GPIO pull-up path: RABPU = 0 enables the
     * pull-ups (active low); INTEDG reads back at its POR value. */
    EPIC_GPIO_SetPullups(GPIO_PULLUP);
    RD1(OPTION_REG, v);
    CHECK((v & 0x40U) == 0x40U && (v & 0x80U) == 0U, 0x02U);
    EPIC_GPIO_SetPullups(GPIO_NOPULL);

    /* Bank 1, OSCCON POR image reads back through the banked path.
     * LTS/HTS/OSTS are read-only hardware status (adding-a-device §4
     * step 8), so only SCS/IRCF are compared. */
    RD1(OSCCON, v);
    CHECK((v & 0x71U) == 0x60U, 0x03U);

    /* Bank 1, WDTCON software enable: the shared driver's Bank-1 path
     * (PIC14MIDRANGE_HAS_WDTCON_BANK1) sets SWDTEN and reads back. */
    EPIC_WDT_SetSoftwareEnable(1U);
    RD1(WDTCON, v);
    CHECK((v & PIC_WDTCON_SWDTEN) != 0U, 0x04U);
    EPIC_WDT_SetSoftwareEnable(0U);
    RD1(WDTCON, v);
    CHECK((v & PIC_WDTCON_SWDTEN) == 0U, 0x05U);

    /* Bank 1, PCON: SBOREN reads back set through the banked path
     * (nBOR/nPOR are reset-cause dependent and stay unasserted). */
    RD1(PCON, v);
    CHECK((v & PIC_PCON_SBOREN) != 0U, 0x11U);

    /* Bank 2, ANSEL via GPIO analog mode: RA1 to analog sets ANS1.
     * RA1, not RA0: RA0 is the PASS/FAIL marker pin (output, armed by
     * the harness), and the marker reads the pin level, so the probe
     * must never return RA0 to input. */
    EPIC_GPIO_Init(GPIOA, GPIO_PIN_1, GPIO_MODE_ANALOG);
    RD2(ANSEL, v);
    CHECK((v & PIC_ANSEL_ANS1) != 0U, 0x06U);
    EPIC_GPIO_Init(GPIOA, GPIO_PIN_1, GPIO_MODE_INPUT);
    RD2(ANSEL, v);
    CHECK((v & PIC_ANSEL_ANS1) == 0U, 0x07U);

    /* Bank 2, CM1CON0 via the comparator driver: C1 on, channel IN0,
     * pin input, non-inverted, internal output. */
    {
        COMP_HandleTypeDef hc = COMP_HANDLE_DEFAULT;
        hc.ChangeCallback = 0;
        (void)EPIC_COMP1_Init(&hc);
    }
    RD2(CM1CON0, v);
    /* 0x08: C1ON set, CxCH/CxR/CxPOL/CxOE clear. C1OUT is a read-only
     * live output (adding-a-device §4 step 8) and stays masked out. */
    CHECK((v & (uint8_t)~PIC_CMx_CxOUT) == PIC_CMx_CxON, 0x08U);

    /* Bank 2, CM2CON0/VRCON via the C2 driver: C2 on, channel IN1,
     * pin input, CVREF reference. CM2CON0 = C2ON|C2R|CH1 = 0x85;
     * VRCON = C2VREN = 0x40. */
    {
        COMP_HandleTypeDef hc2 = COMP_HANDLE_DEFAULT;
        hc2.Channel     = COMP_CHANNEL_IN1;
        hc2.InputSource = COMP_INPUT_REF;
        hc2.RefSource   = COMP_REF_CVREF;
        hc2.ChangeCallback = 0;
        (void)EPIC_COMP2_Init(&hc2);
    }
    RD2(CM2CON0, v);
    CHECK((v & (uint8_t)~PIC_CMx_CxOUT) == 0x85U, 0x12U);
    RD2(VRCON, v);
    CHECK(v == PIC_VRCON_C2VREN, 0x13U);
    /* CM2CON1 via the C2-sync helper: POR T1GSS=1 must survive the
     * RMW that sets C2SYNC. (SetT1GateSource shares the same RMW
     * shape; the T1GSS bit itself is POR-set.) */
    EPIC_COMP_SetC2Sync(1U);
    RD2(CM2CON1, v);
    /* MC1OUT/MC2OUT are read-only live outputs, masked out. */
    CHECK((v & 0x03U) == (uint8_t)(PIC_CM2CON1_T1GSS | PIC_CM2CON1_C2SYNC), 0x14U);

    /* Bank 2, WPUB/IOCB via the GPIO pin helpers. */
    EPIC_GPIO_SetPinPullup(4U, 1U);
    RD2(WPUB, v);
    CHECK((v & 0x10U) != 0U, 0x09U);
    EPIC_GPIO_SetPinPullup(4U, 0U);
    EPIC_GPIO_SetPinIOC(5U, 1U);
    RD2(IOCB, v);
    CHECK((v & 0x20U) != 0U, 0x0AU);
    EPIC_GPIO_SetPinIOC(5U, 0U);
#if PIC16F63X_67X_68X_FAMILY_HAS_ANSELH
    /* ANSELH parts: RB4 to output clears ANS10 (RB4 boots analog). */
    EPIC_GPIO_Init(GPIOB, GPIO_PIN_4, GPIO_MODE_OUTPUT);
    RD2(ANSELH, v);
    CHECK((v & PIC_ANSELH_ANS10) == 0U, 0x10U);
    EPIC_GPIO_Init(GPIOB, GPIO_PIN_4, GPIO_MODE_INPUT);
#endif

    /* Bank 2/3, EEPROM access classes: the data pair (EEDATA/EEADR)
     * through EPIC_BANK2_*, the control pair (EECON1) through
     * EPIC_BANK3_*. (A variable-address fallback existed on the 628A
     * and silently misdirected banked writes; see that family's
     * MANUAL.md.) */
    EPIC_BANK2_WRITE8(EEDATA, 0xC3U);
    EPIC_BANK2_WRITE8(EEADR, 0x10U);
    {
        uint8_t eedata = 0U;
        uint8_t eeadr = 0U;
        EPIC_BANK2_READ8(EEDATA, eedata);
        EPIC_BANK2_READ8(EEADR, eeadr);
        CHECK(eedata == 0xC3U, 0x0BU);
        CHECK(eeadr == 0x10U, 0x0CU);
    }
    /* WR sets in EECON1 via the Bank-3 macro and reads back through
     * the same literal-token path (no unlock sequence issued, so no
     * write starts). */
    EPIC_BANK3_WRITE8(EECON1, (uint8_t)(PIC_EECON1_WREN | PIC_EECON1_WR));
    RD3(EECON1, v);
    CHECK((v & PIC_EECON1_WR) != 0U, 0x0DU);
    EPIC_BANK3_WRITE8(EECON1, 0x00U);

    /* Interrupt gates: TMR1IE lands in PIE1 (Bank 1), EEIE/C1IE in
     * PIE2 (Bank 1, 0x8D); the EEPROM/CMP rows prove the split table
     * (flag in PIR2, enable in PIE2). */
    EPIC_IRQ_Enable(PIC16_IRQ_TMR1);
    RD1(PIE1, v);
    CHECK((v & PIC_PIE1_TMR1IE) != 0U, 0x0EU);
    EPIC_IRQ_DisableSrc(PIC16_IRQ_TMR1);
    EPIC_IRQ_Enable(PIC16_IRQ_C1);
    EPIC_IRQ_Enable(PIC16_IRQ_EEPROM);
    RD1(PIE2, v);
    CHECK((v & (uint8_t)(PIC_PIE2_C1IE | PIC_PIE2_EEIE)) ==
          (uint8_t)(PIC_PIE2_C1IE | PIC_PIE2_EEIE), 0x0FU);
    EPIC_IRQ_DisableSrc(PIC16_IRQ_C1);
    EPIC_IRQ_DisableSrc(PIC16_IRQ_EEPROM);
    /* Report once, then halt: XC8 restarts main() on return, and
     * epic_harness_init() would drive RA0 low again, flickering the
     * marker across the mdb `print PORTA` readback window. One
     * verdict. */
    (void)epic_harness_report(g_fail == 0U);
    pic16f63x_67x_68x_harness_halt();
    return 0;
}
