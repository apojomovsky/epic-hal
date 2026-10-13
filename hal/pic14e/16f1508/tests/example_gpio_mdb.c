/**
 * GPIO, IOC and CLC output-behaviour probe for the mdb gate. Target-safe: uses
 * only the public EPIC_GPIO_*, EPIC_IRQ_* and EPIC_CLC_* contracts, then reads
 * back the SFRs it configured. Edges need an external source, so the IOC check
 * covers the enable path only; the edge path is in example_gpio.c on the host
 * sim. CLC gates run empty (GLS 0), so the outputs below never depend on a
 * signal level, only on gate-polarity constants; sequential modes stay
 * host-only in example_clc_sim.c.
 */
#include <stddef.h>
 #include "pic16f1508.h"
#include "peripherals/pic16f1508_gpio.h"
#include "peripherals/pic16f1508_clc.h"
#include "peripherals/pic16f1508_clc_src.h"
#include "core/pic16f1508_irq.h"
#include "core/epic_harness.h"

/** @brief Halt the target after the probe result is written (mdb gate). */
extern void pic16f1508_harness_halt(void);

/* AND-OR truth vectors: (G1&G2)|(G3&G4) over gate-polarity constants. */
static const uint8_t k_mode0_lines[] = {0x00U, 0x03U, 0x0CU, 0x05U, 0x0AU, 0x0FU, 0x07U, 0x0EU};
static const uint8_t k_mode0_want[] = {0U, 1U, 1U, 0U, 0U, 1U, 1U, 1U};
/* 4-input AND truth vectors: G1&G2&G3&G4. */
static const uint8_t k_mode2_lines[] = {0x00U, 0x0FU, 0x07U, 0x0EU, 0x03U};
static const uint8_t k_mode2_want[] = {0U, 1U, 0U, 0U, 0U};
/* OR-XOR vectors where the named reading and the drawn AND gates agree. */
static const uint8_t k_mode1_lines[] = {0x00U, 0x03U, 0x0CU, 0x0FU};
static const uint8_t k_mode1_want[] = {0U, 1U, 1U, 0U};

/**
 * @brief Configure CLC1 for the gate.
 * @param mode LCxMODE value 0..2.
 * @return 1 on EPIC_OK, 0 otherwise.
 */
static int clc1_gate_config(uint8_t mode)
{
    CLC_HandleTypeDef h;
    h.Instance = CLC1;
    h.Mode = mode;
    h.Source[0] = EPIC_CLC_SourceCode(1U, CLC_IN_CLCIN0);
    h.Source[1] = EPIC_CLC_SourceCode(2U, CLC_IN_FOSC);
    h.Source[2] = EPIC_CLC_SourceCode(3U, CLC1_IN_HFINTOSC);
    h.Source[3] = EPIC_CLC_SourceCode(4U, CLC_IN_CLCIN1);
    h.Gls[0] = 0U;
    h.Gls[1] = 0U;
    h.Gls[2] = 0U;
    h.Gls[3] = 0U;
    h.GatePolarity = 0U;
    h.OutputPolarity = 0U;
    h.OutputEnable = 0U;
    h.InterruptRising = 0U;
    h.InterruptFalling = 0U;
    h.Callback = NULL;
    if (EPIC_CLC_Init(&h) != EPIC_OK)
    {
        return 0;
    }
    epic_harness_tick();
    return 1;
}

/**
 * @brief Drive constant gate levels into CLC1.
 * @param lines G1..G4 levels in bits 3:0; empty gates read the polarity bit.
 */
static void clc1_lines(uint8_t lines)
{
    uint8_t pol = EPIC_REG8(PIC_REG_CLC1POL);
    EPIC_REG8(PIC_REG_CLC1POL) = (uint8_t)((pol & PIC_CLCPOL_POL) | (lines & 0x0FU));
    epic_harness_tick();
}

/**
 * @brief Check the CLC1 live output bit.
 * @details Reads CON.OUT, the datasheet live output (Register 24-1, page 263).
 * MPLAB-SIM never sets the CLCDATA mirror copy, so the gate cannot observe it;
 * the mirror stays covered by the host probes.
 * @param want Expected output, 0 or 1.
 * @return 1 when CON.OUT matches want, 0 otherwise.
 */
static int clc1_check(uint8_t want)
{
    uint8_t co = ((EPIC_REG8(PIC_REG_CLC1CON) & PIC_CLCCON_OUT) != 0U) ? 1U : 0U;
    want = (want != 0U) ? 1U : 0U;
    return co == want;
}

/**
 * @brief GPIO, IOC and CLC output-behaviour probe for the mdb gate.
 * @return Harness result: 0 on PASS, 1 on FAIL.
 */
int main(void)
{
    int ok = 1;
    unsigned int i;

    epic_harness_init(0UL);

    /* RB4 output: TRIS cleared, analog cleared, latch follows WritePin and TogglePin. */
    EPIC_GPIO_Init(GPIOB, GPIO_PIN_4, GPIO_MODE_OUTPUT);
    ok &= ((EPIC_REG8(PIC_REG_TRISB) & GPIO_PIN_4) == 0U);
    ok &= ((EPIC_REG8(PIC_REG_ANSELB) & GPIO_PIN_4) == 0U);
    EPIC_GPIO_WritePin(GPIOB, GPIO_PIN_4, GPIO_PIN_SET);
    ok &= ((EPIC_REG8(PIC_REG_LATB) & GPIO_PIN_4) != 0U);
    EPIC_GPIO_TogglePin(GPIOB, GPIO_PIN_4);
    ok &= ((EPIC_REG8(PIC_REG_LATB) & GPIO_PIN_4) == 0U);

    /* RB5 input: TRIS set, analog cleared. RB0..RB3 are not implemented on this part. */
    EPIC_GPIO_Init(GPIOB, GPIO_PIN_5, GPIO_MODE_INPUT);
    ok &= ((EPIC_REG8(PIC_REG_TRISB) & GPIO_PIN_5) != 0U);
    ok &= ((EPIC_REG8(PIC_REG_ANSELB) & GPIO_PIN_5) == 0U);

    /* LATA3 is unimplemented (RA3 is input-only): target drops the bit, host sim keeps it.
     * Compare only the implemented latch bits, which both builds must match. */
    EPIC_GPIO_Init(GPIOA, GPIO_PIN_All, GPIO_MODE_OUTPUT);
    /* RA0 stays low: the harness drives RA0 high only on PASS, so a stopped probe must not look passed. */
    const uint8_t lata_pattern = (uint8_t)(PIC16F1508_FAMILY_LATA_MASK & (uint8_t)~GPIO_PIN_0);
    EPIC_GPIO_WritePort(GPIOA, lata_pattern);
    ok &= ((EPIC_REG8(PIC_REG_LATA) & PIC16F1508_FAMILY_LATA_MASK) == lata_pattern);

    /* Clear all pull-ups first: target reset is WPUA=0x3F and WPUB=0xF0, host sim is 0,
     * so the expectations below must not depend on the reset state. */
    EPIC_GPIO_SetPullups(GPIOA, GPIO_PIN_All, GPIO_PIN_RESET);
    EPIC_GPIO_SetPullups(GPIOB, GPIO_PIN_All, GPIO_PIN_RESET);
    ok &= ((EPIC_REG8(PIC_REG_OPTION) & PIC_OPTION_nWPUEN) != 0U);

    /* RB4 and RB5 pull-ups: WPUB bits set, nWPUEN active low (cleared when enabled). */
    EPIC_GPIO_SetPullups(GPIOB, GPIO_PIN_4 | GPIO_PIN_5, GPIO_PIN_SET);
    ok &= ((EPIC_REG8(PIC_REG_WPUB) & (GPIO_PIN_4 | GPIO_PIN_5)) == (GPIO_PIN_4 | GPIO_PIN_5));
    ok &= ((EPIC_REG8(PIC_REG_OPTION) & PIC_OPTION_nWPUEN) == 0U);
    /* RB5 cleared while RB4 is still pulled up: nWPUEN stays enabled. */
    EPIC_GPIO_SetPullups(GPIOB, GPIO_PIN_5, GPIO_PIN_RESET);
    ok &= ((EPIC_REG8(PIC_REG_WPUB) & GPIO_PIN_5) == 0U);
    ok &= ((EPIC_REG8(PIC_REG_OPTION) & PIC_OPTION_nWPUEN) == 0U);
    /* No pull-ups left: nWPUEN disabled again. */
    EPIC_GPIO_SetPullups(GPIOB, GPIO_PIN_4, GPIO_PIN_RESET);
    ok &= ((EPIC_REG8(PIC_REG_OPTION) & PIC_OPTION_nWPUEN) != 0U);

    /* IOC: only RB4..RB7 are implemented; the enable path sets IOCIE. */
    EPIC_GPIO_EnableChangeDetect(GPIO_PIN_All, 0U);
    ok &= (EPIC_REG8(PIC_REG_IOCBP) == PIC16F1508_FAMILY_IOCB_MASK);
    EPIC_IRQ_Enable(PIC16F1508_IRQ_IOC);
    ok &= ((EPIC_REG8(PIC_REG_INTCON) & PIC_INTCON_IOCIE) != 0U);
    EPIC_IRQ_DisableSrc(PIC16F1508_IRQ_IOC);
    ok &= ((EPIC_REG8(PIC_REG_INTCON) & PIC_INTCON_IOCIE) == 0U);

    /* CLC output behaviour on CLC1: empty gates driven by gate-polarity
     * constants through modes 0..2, with CON.OUT and CLCDATA checked at every
     * step. Sources are sim-safe Table 24-1 codes (SEL0 0x00, SEL1 0x55);
     * CLC1 D3S codes other than 0 and 5 throw in MPLAB-SIM (#383). */
    ok &= clc1_gate_config(0U);
    ok &= (EPIC_REG8(PIC_REG_CLC1SEL0) == 0x00U);
    ok &= (EPIC_REG8(PIC_REG_CLC1SEL1) == 0x55U);
    for (i = 0U; i < (unsigned int)sizeof(k_mode0_lines); i++)
    {
        clc1_lines(k_mode0_lines[i]);
        ok &= clc1_check(k_mode0_want[i]);
    }
    ok &= clc1_gate_config(2U);
    for (i = 0U; i < (unsigned int)sizeof(k_mode2_lines); i++)
    {
        clc1_lines(k_mode2_lines[i]);
        ok &= clc1_check(k_mode2_want[i]);
    }
    ok &= clc1_gate_config(1U);
    for (i = 0U; i < (unsigned int)sizeof(k_mode1_lines); i++)
    {
        clc1_lines(k_mode1_lines[i]);
        ok &= clc1_check(k_mode1_want[i]);
    }
    /* Output polarity inverts the cell; clearing EN forces a zero output. */
    EPIC_REG8(PIC_REG_CLC1POL) |= PIC_CLCPOL_POL;
    epic_harness_tick();
    clc1_lines(0x03U);
    ok &= clc1_check(0U);
    clc1_lines(0x00U);
    ok &= clc1_check(1U);
    EPIC_REG8(PIC_REG_CLC1POL) &= (uint8_t)~PIC_CLCPOL_POL;
    epic_harness_tick();
    EPIC_REG8(PIC_REG_CLC1CON) &= (uint8_t)~PIC_CLCCON_EN;
    epic_harness_tick();
    clc1_lines(0x0FU);
    ok &= clc1_check(0U);
    ok &= (EPIC_CLC_DeInit(CLC1) == EPIC_OK);
    int rc = epic_harness_report(ok);
    pic16f1508_harness_halt();
    return rc;
}
