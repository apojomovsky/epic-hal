/* HARNESS=sim probe for the pic16f7x-hal banked-SFR audit (16F77
 * exemplar): runs the plain Bank-1/Bank-2 SFR access sites with known
 * values under real XC8 v4.00, so bank misdirection shows up as a FAIL
 * instead of silent corruption. Each site is pre-set through the SAFE
 * macro (a misdirected write leaves the pre-set value, failing the
 * readback); readbacks go through EPIC_BANK1_READ8. The 16F77 has
 * no comparator/vref/EEPROM; the banked surface here is ADCON1,
 * TRISx, OPTION_REG, TXSTA, SPBRG, PR2 and SSP. */

#include "core/epic_harness.h"
#include "core/pic16_irq.h"
#include "peripherals/pic16f7x_adc.h"
#include "peripherals/pic16f7x_gpio.h"
#include "peripherals/pic16f7x_ssp.h"
#include "peripherals/pic16f7x_timer0.h"
#include "peripherals/pic16f7x_timer2.h"
#include "peripherals/pic16f7x_usart.h"
#include "target/pic16f7x_platform.h"

#include <stdint.h>

#ifndef FOSC_HZ
#define FOSC_HZ 20000000UL
#endif

static uint16_t g_fail = 0u;

/**
 * @brief Log a failed check index as a hex pair and bump the counter.
 * @param idx the check index (0x00..0x0F).
 */
static void fail(uint8_t idx)
{
    static const char hx[] = "0123456789ABCDEF";
    char c[2];
    g_fail++;
    epic_harness_log("F");
    c[0] = hx[(idx >> 4) & 0xF];
    c[1] = hx[idx & 0xF];
    epic_harness_log("0x");
    epic_harness_log(c);
    epic_harness_log(".");
}

/* Check numbering: the index is the check number in main() order. */
#define CHECK(cond, idx) do {         \
    if (!(cond)) fail(idx);            \
} while (0)

/* Bank-1 readback helper. */
#define RD1(sfr, out) EPIC_BANK1_READ8(sfr, (out))

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

    /* Class A read side: EPIC_TIMER2_ReadPeriod reads PR2 (0x92) after
     * an ungated bank switch. */
    EPIC_TIMER2_WritePeriod(0xAAu);
    CHECK(EPIC_TIMER2_ReadPeriod() == 0xAAu, 0x01);

    /* Class A read side: EPIC_USART_DeInit restores TXSTA (0x98) and
     * SPBRG (0x99). */
    EPIC_BANK1_WRITE8(SPBRG, 0xAAu);
    EPIC_BANK1_WRITE8(TXSTA, 0xAAu);
    (void)EPIC_USART_DeInit();
    RD1(SPBRG, v);
    CHECK(v == 0x00u, 0x02);
    RD1(TXSTA, v);
    CHECK(v == 0x02u, 0x03);

    /* Class B: EPIC_SSP_ReadByte's SSPBUF value round-trip (Bank 0).
     * The BF-clear RMW is the safe Bank-1 pattern; BF itself is a
     * hardware status bit, not reliable to poke under MPLAB SIM. */
    EPIC_REG8(PIC_REG_SSPBUF) = 0x5Au;
    CHECK(EPIC_SSP_ReadByte() == 0x5Au, 0x04);

    /* Class B: EPIC_GPIO_Init writes TRISx (Bank 1) through a runtime
     * address (FSR-indirect). PORTB all outputs then low nibble
     * inputs. */
    EPIC_GPIO_Init(GPIOB, 0xFFu, GPIO_MODE_OUTPUT);
    RD1(TRISB, v);
    CHECK(v == 0x00u, 0x05);
    EPIC_GPIO_Init(GPIOB, 0x0Fu, GPIO_MODE_INPUT);
    RD1(TRISB, v);
    CHECK(v == 0x0Fu, 0x06);

    /* Class B: EPIC_TIMER0_Init's OPTION_REG RMW. Init clears T0CS and
     * leaves the prescaler assignment to Start, so OPTION_REG == 0xDF. */
    {
        TIMER0_HandleTypeDef t0 = TIMER0_HANDLE_DEFAULT;
        t0.Prescaler = TIMER0_PRESCALER_1_8;
        (void)EPIC_TIMER0_Init(&t0);
        RD1(OPTION_REG, v);
        CHECK(v == 0xDFu, 0x07);
    }

    /* Class B (last: kills the marker USART): EPIC_USART_Init's TXSTA
     * and SPBRG writes are the safe pattern. Verify the Init state,
     * then DeInit, then re-init so the harness marker can transmit.
     * TXEN is expected from the non-null callback. */
    {
        USART_HandleTypeDef h = USART_HANDLE_DEFAULT;
        h.SPBRG = (uint8_t)USART_ComputeSPBRG(
            FOSC_HZ, 9600UL, USART_MODE_ASYNCHRONOUS, USART_BRGH_HIGH);
        h.TxCpltCallback = s_tx_noop;
        (void)EPIC_USART_Init(&h);
        RD1(SPBRG, v);
        CHECK(v == 129u, 0x08);
        RD1(TXSTA, v);
        CHECK((v & PIC_TXSTA_TXEN) != 0u, 0x09);
        (void)EPIC_USART_DeInit();
        RD1(SPBRG, v);
        CHECK(v == 0x00u, 0x0A);
        RD1(TXSTA, v);
        CHECK(v == 0x02u, 0x0B);
        /* Re-init so the harness marker can transmit. */
        (void)EPIC_USART_Init(&h);
        EPIC_IRQ_DisableSrc(PIC16_IRQ_USART_TX);
    }

    for (uint32_t i = 0; epic_harness_running(i); i++) {
        epic_harness_tick();
    }
    return epic_harness_report(g_fail == 0u);
}
