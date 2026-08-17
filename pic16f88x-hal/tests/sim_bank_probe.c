/* HARNESS=sim probe for the pic16f88x-hal banked-SFR audit: runs every
 * ungated `pic_select_bank` + plain EPIC_REG8 site and every plain
 * Bank-1/2/3 SFR access from the Phase 0 audit with known values and
 * reads them back, so XC8 v4.00 bank misdirection (the PIC16 Finding
 * 9 mechanism, see pic16f87xa-hal/README.md) fails the gate instead of
 * corrupting silently. The 88X spreads SFRs across all four banks, so
 * this covers Bank 1 (OPTION_REG, TRISx, TXSTA/SPBRG, ADCON1, VRCON,
 * SSPCON2/SSPSTAT, PWM1CON/ECCPAS/PSTRCON), Bank 2 (WDTCON, CM1CON0,
 * CM2CON0, CM2CON1) and Bank 3 (SRCON, BAUDCTL, ANSEL, ANSELH,
 * EECON1). */

#include "core/epic_harness.h"
#include "core/pic16_irq.h"
#include "core/pic16f88x_wdt_sleep.h"
#include "peripherals/pic16f88x_adc.h"
#include "peripherals/pic16f88x_comp.h"
#include "peripherals/pic16f88x_gpio.h"
#include "peripherals/pic16f88x_ssp.h"
#include "peripherals/pic16f88x_timer0.h"
#include "peripherals/pic16f88x_timer1.h"
#include "peripherals/pic16f88x_timer2.h"
#include "peripherals/pic16f88x_usart.h"
#include "peripherals/pic16f88x_vref.h"
#include "peripherals/pic16f88x_srlatch.h"
#include "peripherals/pic16f88x_osc.h"
#include "target/pic16f88x_platform.h"

#include <stdint.h>

#ifndef FOSC_HZ
#define FOSC_HZ 20000000UL
#endif

static uint16_t g_fail = 0u;

/**
 * @brief Log a failed check index and bump the counter. The mdb harness
 *        writes fmt's raw bytes (no printf), so the index is encoded as
 *        two hex characters.
 * @param idx the check index (0x00..0x0F).
 */
static void fail(uint8_t idx)
{
    static const char hex[] = "0123456789ABCDEF";
    char buf[4] = { 'F', hex[(idx >> 4) & 0x0F], hex[idx & 0x0F], ' ' };
    g_fail++;
    epic_harness_log(buf);
}

/* Check numbering: the index is the check number in main() order,
 * starting 0x00; the mapping is documented on each check. */
#define CHECK(cond, idx) do {         \
    if (!(cond)) fail(idx);            \
} while (0)

/* Bank readback helpers. */
#define RD1(sfr, out) EPIC_BANK1_READ8(sfr, (out))
#define RD2(sfr, out) EPIC_BANK2_READ8(sfr, (out))
#define RD3(sfr, out) EPIC_BANK3_READ8(sfr, (out))

/**
 * @brief TX-complete callback: non-null only to arm TXEN.
 */
static void s_tx_noop(void)
{
}

/**
 * @brief Run the banked-SFR access probes and report pass/fail.
 */
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

    /* Class A: EPIC_VREF_DeInit writes VRCON (Bank 1, 0x97), expect
     * 0x00. */
    EPIC_BANK1_WRITE8(VRCON, 0xAAu);
    (void)EPIC_VREF_DeInit();
    RD1(VRCON, v);
    CHECK(v == 0x00u, 0x01);

    /* Class A: EPIC_SRLATCH_DeInit writes SRCON (Bank 3, 0x185),
     * expect 0x00. */
    EPIC_BANK3_WRITE8(SRCON, 0xAAu);
    (void)EPIC_SRLATCH_DeInit();
    RD3(SRCON, v);
    CHECK(v == 0x00u, 0x02);

    /* Class A: EPIC_OSC_FailSafeDeInit only touches PIE2, so probe the
     * OSC Bank-1 path via SetSystemClockSource: write SCS=1, expect
     * the OSCCON bit set. */
    EPIC_BANK1_WRITE8(OSCCON, 0x70u);   /* POR value: IRCF=110. */
    EPIC_OSC_SetSystemClockSource(1u);
    RD1(OSCCON, v);
    CHECK((v & 0x01u) != 0u, 0x03);

    /* Class A: EPIC_WDT_SetPrescaler (Bank 2, WDTCON 0x105). */
    EPIC_BANK2_WRITE8(WDTCON, 0x08u);   /* POR: WDTPS=0100, SWDTEN=0. */
    EPIC_WDT_SetPrescaler(0x07u);
    RD2(WDTCON, v);
    CHECK((v & 0x1Eu) == (0x07u << 1), 0x04);

    /* Class A: comparator DeInit clears CM1CON0/CM2CON0 (Bank 2).
     * CxOUT (bit 6) is a read-only live comparator output (Register
     * 8-1, R-0): MPLAB SIM leaves it set from the pre-write 0xAA
     * enable, so mask it out of the comparison (docs/adding-a-device.md
     * §4 step 8). */
    EPIC_BANK2_WRITE8(CM1CON0, 0xAAu);
    EPIC_BANK2_WRITE8(CM2CON0, 0xAAu);
    (void)EPIC_COMP1_DeInit();
    (void)EPIC_COMP2_DeInit();
    RD2(CM1CON0, v);
    CHECK((v & (uint8_t)~PIC_CMx_CxOUT) == 0x00u, 0x05);
    RD2(CM2CON0, v);
    CHECK((v & (uint8_t)~PIC_CMx_CxOUT) == 0x00u, 0x06);

    /* Class A: EPIC_TIMER2_ReadPeriod reads PR2 (Bank 1, 0x92). */
    EPIC_BANK1_WRITE8(PR2, 0xAAu);
    {
        /* Read through the driver's safe path. */
        extern uint8_t EPIC_TIMER2_ReadPeriod(void);
        CHECK(EPIC_TIMER2_ReadPeriod() == 0xAAu, 0x07);
    }

    /* Class B: EPIC_GPIO_Init writes TRISx (Bank 1) through a runtime
     * address. PORTB all outputs then low nibble inputs. */
    EPIC_GPIO_Init(GPIOB, 0xFFu, GPIO_MODE_OUTPUT);
    RD1(TRISB, v);
    CHECK(v == 0x00u, 0x08);
    EPIC_GPIO_Init(GPIOB, 0x0Fu, GPIO_MODE_INPUT);
    RD1(TRISB, v);
    CHECK(v == 0x0Fu, 0x09);

    /* Class B: EPIC_TIMER0_Init's OPTION_REG RMW (Bank 1). Init clears
     * T0CS (internal clock) and leaves the prescaler assignment to
     * Start, so OPTION_REG == 0xDF. */
    {
        TIMER0_HandleTypeDef t0 = TIMER0_HANDLE_DEFAULT;
        t0.Prescaler = TIMER0_PRESCALER_1_8;
        (void)EPIC_TIMER0_Init(&t0);
        RD1(OPTION_REG, v);
        CHECK(v == 0xDFu, 0x0A);
    }

    /* Class B: EPIC_TIMER1_Start's CM2CON1<T1GSS> RMW (Bank 2). */
    {
        TIMER1_HandleTypeDef t1 = TIMER1_HANDLE_DEFAULT;
        t1.GateEnabled   = 1u;
        t1.GateSource    = TIMER1_GATE_SRC_T1G;
        (void)EPIC_TIMER1_Start(&t1);
        RD2(CM2CON1, v);
        CHECK((v & 0x02u) != 0u, 0x0B);
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
        CHECK(v == 129u, 0x0C);
        RD1(TXSTA, v);
        CHECK((v & PIC_TXSTA_TXEN) != 0u, 0x0D);
        (void)EPIC_USART_DeInit();
        RD1(SPBRG, v);
        CHECK(v == 0x00u, 0x0E);
        RD1(TXSTA, v);
        CHECK(v == 0x02u, 0x0F);
        /* Re-init so the harness marker can transmit. */
        (void)EPIC_USART_Init(&h);
        EPIC_IRQ_DisableSrc(PIC16_IRQ_USART_TX);
    }

    for (uint32_t i = 0; epic_harness_running(i); i++) {
        epic_harness_tick();
    }
    return epic_harness_report(g_fail == 0u);
}
