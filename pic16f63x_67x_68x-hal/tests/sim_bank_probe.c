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
    for (i = 0U; i <= idx; i++)
    {
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
     * back through the literal-token macro. PORTB shapes (RB4..RB7,
     * 0xF0) probe TRISB; every other shape probes TRISA (RA0..RA5,
     * 0x3F). */
#if PIC16F63X_67X_68X_FAMILY_HAS_PORTB
    pic_select_bank(1);
    EPIC_REG8(PIC_REG_TRISB) = 0x00U;
    pic_select_bank(0);
    RD1(TRISB, v);
    /* 0x00: TRISB bank-1 write landed (RB4..RB7, 0xF0). */
    CHECK((v & 0xF0U) == 0U, 0x00U);

    /* Bank 0, PORT: the latch write lands and reads back. */
    EPIC_REG8(PIC_REG_PORTB) = 0xF0U;
    v = EPIC_REG8(PIC_REG_PORTB);
    CHECK((v & 0xF0U) == 0xF0U, 0x01U);
    EPIC_REG8(PIC_REG_PORTB) = 0x00U;
#else
    pic_select_bank(1);
    EPIC_REG8(PIC_REG_TRISA) = 0x00U;
    pic_select_bank(0);
    RD1(TRISA, v);
    /* 0x00: TRISA bank-1 write landed. The input-only RA3/MCLR
     * direction bit reads back set on some dice (639) and clear on
     * others (684), so it is masked; a misdirected write still
     * fails, it leaves the POR 1s on RA0..RA2/RA4/RA5. */
    CHECK((v & 0x37U) == 0U, 0x00U);
    /* Bank 0, PORT: the latch write lands and reads back. ANSEL
     * powers up all-analog (0xFF) on these parts, and an
     * analog-selected output reads 0, so the pins go digital through
     * the driver first; the measured path stays the raw latch. */
    EPIC_GPIO_Init(GPIOA, (uint16_t)(GPIO_PIN_0 | GPIO_PIN_1 | GPIO_PIN_2 | GPIO_PIN_4 | GPIO_PIN_5),
                   GPIO_MODE_OUTPUT);
    EPIC_REG8(PIC_REG_PORTA) = 0x3FU;
    v = EPIC_REG8(PIC_REG_PORTA);
    /* RA3/MCLR pin level reads back set on some dice (639/684) and
     * clear on others (688), so it is masked like the direction bit
     * above; a misdirected write still fails on the POR-0 latch. */
    CHECK((v & 0x37U) == 0x37U, 0x01U);
    EPIC_REG8(PIC_REG_PORTA) = 0x00U;
#endif
    /* Bank 1, OPTION via the GPIO pull-up path: RABPU = 0 enables the
     * pull-ups (active low); INTEDG reads back at its POR value. */
    EPIC_GPIO_SetPullups(GPIO_PULLUP);
    RD1(OPTION_REG, v);
    CHECK((v & 0x40U) == 0x40U && (v & 0x80U) == 0U, 0x02U);
    EPIC_GPIO_SetPullups(GPIO_NOPULL);

    /* Bank 1, OSCCON POR image reads back through the banked path.
     * LTS/HTS/OSTS are read-only hardware status (adding-a-device §4
     * step 8), so only SCS/IRCF are compared. The 1K dice (630/676)
     * have no OSCCON. */
    RD1(OSCCON, v);
    CHECK((v & 0x71U) == 0x60U, 0x03U);

    /* WDTCON software enable through the shared driver, read back
     * through the matching bank path: Bank-1 WDTCON on the 4-bank
     * shapes, Bank-0 WDTCON (plain access at bank 0) on the 2-bank
     * WDT shapes. Every probe-building part has WDTCON. */
    EPIC_WDT_SetSoftwareEnable(1U);
#if PIC16F63X_67X_68X_FAMILY_WDTCON_BANK0
    v = EPIC_REG8(PIC_REG_WDTCON_BANK0);
#else
    RD1(WDTCON, v);
#endif
    CHECK((v & PIC_WDTCON_SWDTEN) != 0U, 0x04U);
    EPIC_WDT_SetSoftwareEnable(0U);
#if PIC16F63X_67X_68X_FAMILY_WDTCON_BANK0
    v = EPIC_REG8(PIC_REG_WDTCON_BANK0);
#else
    RD1(WDTCON, v);
#endif
    CHECK((v & PIC_WDTCON_SWDTEN) == 0U, 0x05U);

#if PIC16F63X_67X_68X_FAMILY_HAS_PCON_SBOREN
    /* Bank 1, PCON: SBOREN reads back set through the banked path
     * (nBOR/nPOR are reset-cause dependent and stay unasserted).
     * The 630/676/688 PCON has no SBOREN bit; their Bank-1 read
     * path is already proven by the OPTION/OSCCON/PIE checks. */
    RD1(PCON, v);
    CHECK((v & PIC_PCON_SBOREN) != 0U, 0x11U);
#endif

#if PIC16F63X_67X_68X_FAMILY_HAS_ANSEL
    /* ANSEL via GPIO analog mode: RA1 to analog sets ANS1. RA1, not
     * RA0: RA0 is the PASS/FAIL marker pin (output, armed by the
     * harness), and the marker reads the pin level, so the probe
     * must never return RA0 to input. Parts without ANSEL (630/639,
     * no ADC) skip. The 14-pin ADC parts read it through the Bank-1
     * macro with the real SFR name. */
    EPIC_GPIO_Init(GPIOA, GPIO_PIN_1, GPIO_MODE_ANALOG);
#if PIC16F63X_67X_68X_FAMILY_HAS_ANSEL_BANK1
    EPIC_BANK1_READ8(ANSEL, v);
#else
    RD2(ANSEL, v);
#endif
    CHECK((v & PIC_ANSEL_ANS1) != 0U, 0x06U);
    EPIC_GPIO_Init(GPIOA, GPIO_PIN_1, GPIO_MODE_INPUT);
#if PIC16F63X_67X_68X_FAMILY_HAS_ANSEL_BANK1
    EPIC_BANK1_READ8(ANSEL, v);
#else
    RD2(ANSEL, v);
#endif
    CHECK((v & PIC_ANSEL_ANS1) == 0U, 0x07U);
#endif

#if PIC16F63X_67X_68X_FAMILY_HAS_COMP_DUAL
    /* Bank 2, CM1CON0 via the comparator driver: C1 on, channel IN0,
     * pin input, non-inverted, internal output. Dual-comparator
     * shapes only; the legacy-CMCON/CMCON0 parts (630/676/684/688)
     * and the comparator-less 639 have no driver for their
     * comparator (honest classification, see MANUAL.md). */
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
#endif
#if PIC16F63X_67X_68X_FAMILY_HAS_PORTB
    /* Bank 2, WPUB/IOCB via the GPIO pin helpers. */
    EPIC_GPIO_SetPinPullup(4U, 1U);
    RD2(WPUB, v);
    CHECK((v & 0x10U) != 0U, 0x09U);
    EPIC_GPIO_SetPinPullup(4U, 0U);
    EPIC_GPIO_SetPinIOC(5U, 1U);
    RD2(IOCB, v);
    CHECK((v & 0x20U) != 0U, 0x0AU);
    EPIC_GPIO_SetPinIOC(5U, 0U);
#elif PIC16F63X_67X_68X_FAMILY_HAS_WPUA
    /* No PORTB: WPUA/IOCA live in Bank 1 (0x95/0x96), so they go
     * through the Bank-1 macros with the real SFR names. */
    EPIC_BANK1_WRITE8(WPUA, 0x10U);
    RD1(WPUA, v);
    CHECK((v & 0x10U) != 0U, 0x09U);
    EPIC_BANK1_WRITE8(WPUA, 0x00U);
    EPIC_BANK1_WRITE8(IOCA, 0x20U);
    RD1(IOCA, v);
    CHECK((v & 0x20U) != 0U, 0x0AU);
    EPIC_BANK1_WRITE8(IOCA, 0x00U);
#else
    /* 16F639: no pull-ups on the die, IOCA only (Bank 1). */
    EPIC_BANK1_WRITE8(IOCA, 0x20U);
    RD1(IOCA, v);
    CHECK((v & 0x20U) != 0U, 0x0AU);
    EPIC_BANK1_WRITE8(IOCA, 0x00U);
#endif
#if PIC16F63X_67X_68X_FAMILY_HAS_ANSELH
    /* ANSELH parts: RB4 to output clears ANS10 (RB4 boots analog). */
    EPIC_GPIO_Init(GPIOB, GPIO_PIN_4, GPIO_MODE_OUTPUT);
    RD2(ANSELH, v);
    CHECK((v & PIC_ANSELH_ANS10) == 0U, 0x10U);
    EPIC_GPIO_Init(GPIOB, GPIO_PIN_4, GPIO_MODE_INPUT);
#endif

    /* EEPROM access classes: the data pair (EEDATA/EEADR) through the
     * Bank-2 macros and the control pair (EECON1) through the Bank-3
     * macros on the 4-bank shapes; all three through the Bank-1
     * macros on the 2-bank shapes (0x9A-0x9C), mirroring the shared
     * driver's own dispatch. (A variable-address fallback existed on
     * the 628A and silently misdirected banked writes; see that
     * family's MANUAL.md.) */
#if !PIC16F63X_67X_68X_FAMILY_EEPROM_BANK1
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
#else
    EPIC_BANK1_WRITE8(EEDATA, 0xC3U);
    EPIC_BANK1_WRITE8(EEADR, 0x10U);
    {
        uint8_t eedata = 0U;
        uint8_t eeadr = 0U;
        EPIC_BANK1_READ8(EEDATA, eedata);
        EPIC_BANK1_READ8(EEADR, eeadr);
        CHECK(eedata == 0xC3U, 0x0BU);
        CHECK(eeadr == 0x10U, 0x0CU);
    }
    /* WR sets in EECON1 (0x9C, Bank 1) via the Bank-1 macro and reads
     * back through the same literal-token path. */
    EPIC_BANK1_WRITE8(EECON1, (uint8_t)(PIC_EECON1_WREN | PIC_EECON1_WR));
    RD1(EECON1, v);
    CHECK((v & PIC_EECON1_WR) != 0U, 0x0DU);
    EPIC_BANK1_WRITE8(EECON1, 0x00U);
#endif

    /* Interrupt gates: TMR1IE lands in PIE1 (Bank 1) on every shape.
     * On the 4-bank shapes EEIE/C1IE live in PIE2 (Bank 1, 0x8D);
     * the EEPROM/CMP rows prove the split table (flag in PIR2,
     * enable in PIE2). On the 2-bank shapes EEPROM completion lives
     * in PIR1<7>/PIE1<7> and the legacy comparator interrupt is
     * undispatched, so only the PIE1 EEIE enable is proven. */
    EPIC_IRQ_Enable(PIC16_IRQ_TMR1);
    RD1(PIE1, v);
    CHECK((v & PIC_PIE1_TMR1IE) != 0U, 0x0EU);
    EPIC_IRQ_DisableSrc(PIC16_IRQ_TMR1);
#if PIC16F63X_67X_68X_FAMILY_HAS_PIR2
    EPIC_IRQ_Enable(PIC16_IRQ_C1);
    EPIC_IRQ_Enable(PIC16_IRQ_EEPROM);
    RD1(PIE2, v);
    CHECK((v & (uint8_t)(PIC_PIE2_C1IE | PIC_PIE2_EEIE)) ==
          (uint8_t)(PIC_PIE2_C1IE | PIC_PIE2_EEIE), 0x0FU);
    EPIC_IRQ_DisableSrc(PIC16_IRQ_C1);
    EPIC_IRQ_DisableSrc(PIC16_IRQ_EEPROM);
#else
    EPIC_IRQ_Enable(PIC16_IRQ_EEPROM);
    RD1(PIE1, v);
    CHECK((v & PIC_PIE1_EEIE) != 0U, 0x0FU);
    EPIC_IRQ_DisableSrc(PIC16_IRQ_EEPROM);
#endif
    /* Report once, then halt: XC8 restarts main() on return, and
     * epic_harness_init() would drive RA0 low again, flickering the
     * marker across the mdb `print PORTA` readback window. One
     * verdict. */
    (void)epic_harness_report(g_fail == 0U);
    pic16f63x_67x_68x_harness_halt();
    return 0;
}
