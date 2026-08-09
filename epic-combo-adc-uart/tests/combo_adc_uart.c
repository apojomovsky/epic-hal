/**
 * @file    combo_adc_uart.c
 * @brief   C3 of the combination matrix
 *          (docs/superpowers/plans/2026-08-09-combination-matrix.md):
 *          ADC conversion loop + TIMER1 + USART TX interleaved, all
 *          real code, one firmware, one bank state.
 *
 * @details
 *   The bug class this gate hunts: a TIMER1 overflow interrupt taken
 *   while the preempted main line is inside a bank window. The ADC
 *   path is the widest bank user here (EPIC_ADC_Read opens TWO
 *   EPIC_BANK1_READ8 windows per conversion: ADRESL and ADCON1, plus
 *   the Bank-0 ADRESH read between them; EPIC_ADC_Init writes ADCON1
 *   through EPIC_BANK1_WRITE8), and the polled USART TX keeps the
 *   Bank-1 TXSTA/TRMT reads flowing, so the loop is a standing bank
 *   interleave exercise under a ~51 us TIMER1 ISR. The cross-checks
 *   verify the timer stayed healthy (TMR1IE intact, callback count
 *   advancing, GIE alive), the ADC register state and readback stayed
 *   consistent, and the USART TX path never stalled.
 *
 *   MPLAB SIM behaviors this gate pins down (probed 2026-08-09, see
 *   the "ADCM" diagnostic line and the status-byte stream in the
 *   captured UART):
 *   - MPLAB SIM DOES model the ADC on 16F877A: after EPIC_ADC_Start()
 *     sets ADCON0<GO/DONE>, the conversion completes - GO/DONE clears
 *     and PIR1<ADIF> sets - and ADRESH:ADRESL read back 0x0000 (no
 *     analog stimulus), stably and deterministically. The ordering
 *     quirk is visible in the status stream: the first byte carries
 *     adif=1 but go=0 (ADIF is observed one spin before GO/DONE
 *     clears on the first conversion); every later byte carries both.
 *     The gate therefore asserts the full conversion contract:
 *     completion (GO clears), ADIF, in-range result, stable readback.
 *   - Enabling GIE while a timer interrupt is already pending wedges
 *     the sim's ISR path (the Finding 10.1 class, same as C1): the
 *     gate stops TIMER1 and clears TMR1IF before GIE-on, per pass,
 *     and starts the timer after.
 *
 *   Bounded and self-reporting (the harness contract). No RX
 *   involvement, so the MPLAB SIM RX wall does not apply.
 */

#include "core/epic_harness.h"
#include "core/pic16_irq.h"
#include "peripherals/pic16f87xa_adc.h"
#include "peripherals/pic16f87xa_timer1.h"
#include "peripherals/pic16f87xa_usart.h"
#include "target/pic16f87xa_platform.h"

#include <stdint.h>

#ifndef FOSC_HZ
#define FOSC_HZ 20000000UL
#endif

#define SIM_ITERATIONS 2000UL

/* ~51 us period at 20 MHz: instruction clock 5 MHz, TMR1 prescaler
 * 1:1, reload 0xFF00 -> 256 counts. The ISR re-arms the counter, so
 * the overflow rate is one every 256 counts, not one per 65536. */
#define T1_RELOAD 0xFF00u

/* Bounded conversion wait: 8 spins is ~112 instruction cycles, far
 * more than a 12-Tad conversion (24 cycles at Fosc/8 / 20 MHz), so a
 * real part completes within the bound; the sim's ADC model completes
 * within a spin or two, so the bound is only the guard against a
 * wedged conversion. */
#define ADC_WAIT_SPINS 8u

/* A status byte goes out over the polled USART every 64 iterations. */
#define STATUS_PERIOD 64u

static uint16_t g_t1_count = 0u;
static uint16_t g_fail = 0u;

static void t1_overflow_cb(void)
{
    g_t1_count++;
    /* Re-arm so the next overflow comes 256 counts later, keeping the
     * ISR cadence high for the bank-interleave exercise. Inline SFR
     * writes (high byte first, DS39582B §6.8) instead of a
     * EPIC_TIMER1_WriteCounter call: the ISR call graph must stay on
     * one flash page (the dispatch's page discipline), and a leaf
     * callback keeps that constraint trivial. */
    EPIC_REG8(PIC_REG_TMR1H) = (uint8_t)(T1_RELOAD >> 8);
    EPIC_REG8(PIC_REG_TMR1L) = (uint8_t)(T1_RELOAD & 0xFFu);
}

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

#define CHECK(cond, idx) do {         \
    if (!(cond)) fail(idx);            \
} while (0)

static void s_tx_noop(void)
{
}

/* Polled status-byte transmit with a bounded TRMT wait: a wedged or
 * corrupted TX path (e.g. the Bank-1 TXSTA read misdirecting to the
 * Bank-0 alias RCSTA, which has no TRMT bit) makes the wait time out
 * and increments *stall instead of hanging the gate. */
static void s_tx_status(uint8_t data, uint16_t *stall)
{
    uint16_t n = 0u;
    while (!EPIC_USART_IsTxShiftRegisterEmpty() && n < 1000u) {
        n++;
    }
    if (n == 1000u) (*stall)++;
    EPIC_USART_Transmit(data);
}

int main(void)
{
    epic_harness_init(SIM_ITERATIONS);

    /* USART: the harness already inited it; re-init with the no-op
     * callback handle (arms TXEN) and turn the TX interrupt source off
     * (transmission is polled). */
    {
        USART_HandleTypeDef h = USART_HANDLE_DEFAULT;
        h.SPBRG = (uint8_t)USART_ComputeSPBRG(
            FOSC_HZ, 9600UL, USART_MODE_ASYNCHRONOUS, USART_BRGH_HIGH);
        h.TxCpltCallback = s_tx_noop;
        (void)EPIC_USART_Init(&h);
        EPIC_IRQ_DisableSrc(PIC16_IRQ_USART_TX);
    }

    /* ADC: single channel AN0, Fosc/8, right-justified, VDD/VSS
     * references, polled. No conversion-complete callback, so ADIE
     * stays off and the ISR's ADC branch (which the dispatcher runs
     * whenever ADIF is set, gated on the flag alone) only clears the
     * latched ADIF from each completed conversion - it never calls
     * back into application code. */
    ADC_HandleTypeDef adc = ADC_HANDLE_DEFAULT;
    adc.Channel = ADC_CHANNEL_AN0;
    adc.ClockSource = ADC_CLOCK_FOSC_8;
    adc.ResultFormat = ADC_FORMAT_RIGHT;
    adc.Reference = ADC_REFERENCE_VDD_VSS_8CH;
    (void)EPIC_ADC_Init(&adc);

    /* Init-time register checks (the sim honors these writes). */
    CHECK((EPIC_REG8(PIC_REG_ADCON0) & PIC_ADCON0_ADON) != 0u, 0x06);
    CHECK((EPIC_REG8(PIC_REG_ADCON0) & PIC_ADCON0_CHS_MASK) == 0u, 0x06);
    CHECK((EPIC_REG8(PIC_REG_ADCON0) & PIC_ADCON0_GO_DONE) == 0u, 0x06);
    {
        uint8_t adcon1 = 0u;
        EPIC_BANK1_READ8(ADCON1, adcon1);
        CHECK((adcon1 & PIC_ADCON1_ADFM) != 0u, 0x06);       /* right-justified */
        CHECK((adcon1 & PIC_ADCON1_PCFG_MASK) == 0u, 0x06);  /* 8-ch VDD/VSS */
    }

    /* GIE on BEFORE the timer starts, so the first overflow fires the
     * ISR with GIE already set (no latch-then-enable edge). MPLAB SIM
     * wedges the ISR path when GIE is enabled while a timer interrupt
     * is already pending (same Finding 10.1 class C1 reproduces), so
     * the timer is stopped and its flag cleared first, per pass (main
     * re-runs after returning, `ljmp start`). */
    (void)EPIC_TIMER1_Stop();
    EPIC_IRQ_ClearFlag(PIC16_IRQ_TMR1);
    EPIC_IRQ_Restore(1);

    /* TIMER1 with the overflow callback: enables TMR1IE. */
    TIMER1_HandleTypeDef t1 = TIMER1_HANDLE_DEFAULT;
    t1.Prescaler = TIMER1_PRESCALER_1_1;
    t1.ReloadValue = T1_RELOAD;
    t1.OverflowCallback = t1_overflow_cb;
    (void)EPIC_TIMER1_Init(&t1);
    (void)EPIC_TIMER1_Start(&t1);

    /* Per-pass observations (locals: main re-runs, and the checks must
     * see a fresh pass, not state carried from the previous one). */
    uint16_t first_result = 0u;
    uint8_t have_result = 0u;
    uint8_t adc_stable = 1u;
    uint8_t go_clears = 0u;    /* GO/DONE observed clearing after a Start */
    uint8_t adif_seen = 0u;    /* PIR1<ADIF> observed set */
    uint8_t first_start_ok = 0u;
    uint8_t adc_started = 0u;
    uint16_t tx_count = 0u;
    uint16_t tx_stall = 0u;

    for (uint32_t i = 0; epic_harness_running(i); i++) {
        /* Start a conversion when the ADC is idle: the sim's model
         * completes every conversion (GO/DONE clears), so this fires
         * once per iteration; the guard keeps a wedged conversion
         * from stacking Starts. */
        if (!EPIC_ADC_IsConversionInProgress()) {
            uint16_t rv = EPIC_ADC_Start();
            if (!adc_started) {
                first_start_ok = (rv == 0u) ? 1u : 0u;
                adc_started = 1u;
            }
        }
        /* Bounded wait for completion. */
        for (uint16_t w = 0u; w < ADC_WAIT_SPINS; w++) {
            if (EPIC_ADC_IsConversionDone()) { adif_seen = 1u; break; }
            if (!EPIC_ADC_IsConversionInProgress()) { go_clears = 1u; break; }
        }
        /* Read the result registers (two Bank-1 windows + one Bank-0
         * read: the bank-interleave surface this gate exercises). */
        uint16_t r = EPIC_ADC_Read();
        if (!have_result) {
            first_result = r;
            have_result = 1u;
        } else if (r != first_result) {
            adc_stable = 0u;
        }

        /* Periodic status byte over the polled USART. The byte's low
         * nibble is the iteration index, its high nibble carries the
         * conversion observations so far, and every byte lands in the
         * mdb UART capture (the gate's "reported over UART" leg).
         * Base 0x40 keeps the stream printable: under the sim's ADC
         * model the first byte is 'D' (adif=1, go=0 - ADIF becomes
         * visible one spin before GO/DONE clears on the first
         * conversion) and every later byte is 'L'/'M'/'N'/'O' (both
         * flags set, iteration low nibble). */
        if ((i & (STATUS_PERIOD - 1u)) == 0u) {
            s_tx_status((uint8_t)(0x40u | (go_clears ? 0x08u : 0u) |
                                  (adif_seen ? 0x04u : 0u) |
                                  (uint8_t)(i & 0x07u)),
                        &tx_stall);
            tx_count++;
        }
    }

    /* GIE must still be alive at loop end: the sim wedges it on the
     * latch-then-enable edge, so a live GIE here proves the per-pass
     * stop/clear/restore discipline held. Capture before Disable. */
    uint8_t gie_alive = (EPIC_REG8(PIC_REG_INTCON) & PIC_INTCON_GIE) != 0u;
    EPIC_IRQ_Disable();

    /* Let the ADC finish the conversion it started last (a real part
     * needs the bounded wait; the sim completes within it). */
    for (uint16_t w = 0u; w < ADC_WAIT_SPINS; w++) {
        if (!EPIC_ADC_IsConversionInProgress()) break;
    }

    /* Cross-checks. */
    uint8_t pie1 = 0u;
    EPIC_BANK1_READ8(PIE1, pie1);
    CHECK((pie1 & PIC_PIE1_TMR1IE) != 0u, 0x00);   /* source still enabled */
    CHECK((pie1 & (uint8_t)~(PIC_PIE1_TMR1IE)) == 0u, 0x01); /* nothing else */
    CHECK(gie_alive != 0u, 0x02);                  /* GIE alive at loop end */
    CHECK(g_t1_count >= 100u, 0x03);               /* timer kept firing */
    CHECK(tx_stall == 0u, 0x04);                   /* TX never stalled */
    CHECK(tx_count == (uint16_t)((SIM_ITERATIONS + STATUS_PERIOD - 1u) /
                                 STATUS_PERIOD), 0x05); /* every status byte went out */
    CHECK(first_start_ok != 0u, 0x06);             /* first Start accepted */

    /* ADC conversion contract (what the sim's model honors, probed
     * 2026-08-09: completion with a stable 0x0000 result). The
     * conversion loop was bounded, so these assert the model actually
     * completed conversions and the readback path stayed clean. */
    CHECK(go_clears != 0u, 0x07);                  /* GO/DONE cleared */
    CHECK(adif_seen != 0u, 0x07);                  /* ADIF set */
    CHECK(have_result != 0u, 0x08);
    CHECK(first_result < 1024u, 0x08);             /* 10-bit result */
    CHECK(adc_stable != 0u, 0x08);                 /* readback stable */
    CHECK(EPIC_ADC_IsConversionInProgress() == 0u, 0x08); /* GO cleared */

    /* TX path still live after the interleave: one more polled byte,
     * shift register must report empty (the harness's own marker
     * transmit uses the same path, so this is the same contract the
     * runner's PASS grep depends on). */
    s_tx_status(0x5Au, &tx_stall);
    CHECK(EPIC_USART_IsTxShiftRegisterEmpty() != 0u, 0x09);

    /* Diagnostic line (raw bytes only, no args): documents the ADC
     * model observation for this run - "ADCM" + go_clears, adif_seen,
     * stable, first_result hi/lo as hex digits. */
    {
        static const char hx[] = "0123456789ABCDEF";
        char c[2];
        epic_harness_log("ADCM");
        c[0] = hx[go_clears & 0xF]; c[1] = '\0'; epic_harness_log(c);
        c[0] = hx[adif_seen & 0xF]; c[1] = '\0'; epic_harness_log(c);
        c[0] = hx[adc_stable & 0xF]; c[1] = '\0'; epic_harness_log(c);
        c[0] = hx[(first_result >> 4) & 0xF]; c[1] = '\0'; epic_harness_log(c);
        c[0] = hx[first_result & 0xF]; c[1] = '\0'; epic_harness_log(c);
        epic_harness_log("\n");
    }

    for (uint32_t i = 0; epic_harness_running(i); i++) {
        epic_harness_tick();
    }
    return epic_harness_report(g_fail == 0u);
}
