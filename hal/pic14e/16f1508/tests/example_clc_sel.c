/**
 * CLC SEL-decode host-sim probe. Host-only: configures cells through
 * EPIC_CLC_Init and checks that undriven Dx levels decode their SEL codes
 * (Table 24-1): LCx_OUT taps post-POL settle combinationally in one step,
 * timer sources are the TMR1 wrap and pre-postscaler TMR2 match pulses,
 * CLCxIN0/1 follow their fixed pins, and unmodelled sources read 0 unless
 * the Dx hook overrides them.
 */
#include <stddef.h>
#include "pic16f1508.h"
#include "peripherals/pic16f1508_clc.h"
#include "peripherals/pic16f1508_clc_src.h"
#include "peripherals/pic16f1508_clc_gls.h"
#include "core/epic_harness.h"
#include "pic16f1508_sim.h"

/**
 * @brief Configure one cell as a single-gate passthrough of one Dx.
 * @param cell Cell to configure (CLC1..CLC4).
 * @param mux Data input 1..4 the gate reads.
 * @param code Select code stored in Source[mux - 1].
 */
static void setup_tap(CLC_InstanceTypeDef cell, uint8_t mux, uint8_t code)
{
    CLC_HandleTypeDef h;
    unsigned int i;

    h.Instance = cell;
    h.Mode = 1U;
    h.Source[0] = 0U;
    h.Source[1] = 0U;
    h.Source[2] = 0U;
    h.Source[3] = 0U;
    h.Source[mux - 1U] = code;
    for (i = 0U; i < 4U; i++)
    {
        h.Gls[i] = 0U;
    }
    h.Gls[mux - 1U] = EPIC_CLC_GLSInput(mux, 0U, 1U);
    h.GatePolarity = 0U;
    h.OutputPolarity = 0U;
    h.OutputEnable = 0U;
    h.InterruptRising = 0U;
    h.InterruptFalling = 0U;
    h.Callback = NULL;
    EPIC_CLC_Init(&h);
}

/**
 * @brief Configure one cell as a D flop clocked by the T2 match pulse.
 * @param cell Cell to configure (CLC1..CLC4).
 * @param code Select code for the D input on D2.
 */
static void setup_flop(CLC_InstanceTypeDef cell, uint8_t code)
{
    CLC_HandleTypeDef h;

    h.Instance = cell;
    h.Mode = 5U;
    h.Source[0] = EPIC_CLC_SourceCode(1U, CLC_IN_T2_MATCH);
    h.Source[1] = code;
    h.Source[2] = EPIC_CLC_SourceCode(3U, CLC_IN_LC1_OUT);
    h.Source[3] = EPIC_CLC_SourceCode(4U, CLC_IN_CLCIN0);
    h.Gls[0] = EPIC_CLC_GLSInput(1U, 0U, 1U);
    h.Gls[1] = EPIC_CLC_GLSInput(2U, 0U, 1U);
    h.Gls[2] = 0U;
    h.Gls[3] = 0U;
    h.GatePolarity = 0U;
    h.OutputPolarity = 0U;
    h.OutputEnable = 0U;
    h.InterruptRising = 0U;
    h.InterruptFalling = 0U;
    h.Callback = NULL;
    EPIC_CLC_Init(&h);
}

/**
 * @brief Read back a cell output through the driver contract.
 * @param cell Cell to read.
 * @return 0 or 1.
 */
static uint8_t read_cell(CLC_InstanceTypeDef cell)
{
    uint8_t level = 0U;

    EPIC_CLC_ReadOutput(cell, &level);
    return (level != 0U) ? 1U : 0U;
}

/**
 * @brief CLC SEL-decode checks on the host simulator.
 * @return Harness result: 0 on PASS, 1 on FAIL.
 */
int main(void)
{
    int ok = 1;

    epic_harness_init(10000UL);
    /* CLC1 follows CLC1IN0 (RA3); CLC2 follows LC1_OUT; CLC3 follows
     * TMR1IF; CLC4 follows TMR2IF. */
    setup_tap(CLC1, 1U, EPIC_CLC_SourceCode(1U, CLC_IN_CLCIN0));
    setup_tap(CLC2, 2U, EPIC_CLC_SourceCode(2U, CLC_IN_LC1_OUT));
    setup_tap(CLC3, 1U, EPIC_CLC_SourceCode(1U, CLC_IN_T1_OVERFLOW));
    setup_tap(CLC4, 1U, EPIC_CLC_SourceCode(1U, CLC_IN_T2_MATCH));
    epic_harness_tick();
    ok &= (read_cell(CLC1) == 0U);
    ok &= (read_cell(CLC2) == 0U);
    ok &= (read_cell(CLC3) == 0U);
    ok &= (read_cell(CLC4) == 0U);

    /* Pin decode with a settled cascade: CLC1 answers at once and the
     * LC1_OUT tap lands in the same step. */
    pic16f1508_sim_drive_input('A', 3U, 1U);
    epic_harness_tick();
    ok &= (read_cell(CLC1) == 1U);
    ok &= (read_cell(CLC2) == 1U);

    /* Post-POL tap: inverting CLC1 inverts what CLC2 sees, same step. */
    EPIC_REG8(PIC_REG_CLC1POL) |= PIC_CLCPOL_POL;
    epic_harness_tick();
    ok &= (read_cell(CLC1) == 0U);
    ok &= (read_cell(CLC2) == 0U);
    EPIC_REG8(PIC_REG_CLC1POL) &= (uint8_t)~PIC_CLCPOL_POL;
    epic_harness_tick();
    ok &= (read_cell(CLC1) == 1U);
    ok &= (read_cell(CLC2) == 1U);

    /* Four-deep cascade against the wiring order: CLC4 follows its pin while
     * each lower cell taps the one above, so the lowest cell settles last.
     * One step still lands all four outputs. */
    setup_tap(CLC4, 1U, EPIC_CLC_SourceCode(1U, CLC_IN_CLCIN0));
    setup_tap(CLC3, 2U, EPIC_CLC_SourceCode(2U, CLC_IN_LC4_OUT));
    setup_tap(CLC2, 2U, EPIC_CLC_SourceCode(2U, CLC_IN_LC3_OUT));
    setup_tap(CLC1, 2U, EPIC_CLC_SourceCode(2U, CLC_IN_LC2_OUT));
    pic16f1508_sim_drive_input('B', 5U, 1U);
    epic_harness_tick();
    ok &= (read_cell(CLC4) == 1U);
    ok &= (read_cell(CLC3) == 1U);
    ok &= (read_cell(CLC2) == 1U);
    ok &= (read_cell(CLC1) == 1U);
    pic16f1508_sim_drive_input('B', 5U, 0U);
    epic_harness_tick();
    ok &= (read_cell(CLC4) == 0U);
    ok &= (read_cell(CLC3) == 0U);
    ok &= (read_cell(CLC2) == 0U);
    ok &= (read_cell(CLC1) == 0U);
    setup_tap(CLC3, 1U, EPIC_CLC_SourceCode(1U, CLC_IN_T1_OVERFLOW));
    setup_tap(CLC4, 1U, EPIC_CLC_SourceCode(1U, CLC_IN_T2_MATCH));

    /* T1_overflow is the wrap pulse, not the TMR1IF level: the flag stays
     * latched while the tap drops back to 0 on the next step. */
    EPIC_REG8(PIC_REG_TMR1H) = 0xFFU;
    EPIC_REG8(PIC_REG_TMR1L) = 0xFFU;
    EPIC_REG8(PIC_REG_T1CON) = PIC_T1CON_TMR1ON;
    epic_harness_tick();
    ok &= (read_cell(CLC3) == 1U);
    epic_harness_tick();
    ok &= (read_cell(CLC3) == 0U);
    ok &= ((EPIC_REG8(PIC_REG_PIR1) & PIC_PIR1_TMR1IF) != 0U);
    EPIC_REG8(PIC_REG_T1CON) = 0U;

    /* T2_match is the PR2 match ahead of the 1:4 postscaler: it pulses on
     * every second step while TMR2IF waits for the fourth match. */
    EPIC_REG8(PIC_REG_PR2) = 1U;
    EPIC_REG8(PIC_REG_T2CON) = (uint8_t)(PIC_T2CON_TMR2ON | (3U << PIC_T2CON_T2OUTPS_SHIFT));
    epic_harness_tick();
    ok &= (read_cell(CLC4) == 0U);
    epic_harness_tick();
    ok &= (read_cell(CLC4) == 1U);
    epic_harness_tick();
    ok &= (read_cell(CLC4) == 0U);
    epic_harness_tick();
    ok &= (read_cell(CLC4) == 1U);
    ok &= ((EPIC_REG8(PIC_REG_PIR1) & PIC_PIR1_TMR2IF) == 0U);
    epic_harness_tick();
    epic_harness_tick();
    epic_harness_tick();
    epic_harness_tick();
    ok &= ((EPIC_REG8(PIC_REG_PIR1) & PIC_PIR1_TMR2IF) != 0U);
    EPIC_REG8(PIC_REG_T2CON) = 0U;

    /* Unmodelled FOSC reads 0; a hooked Dx wins over the decode. */
    setup_tap(CLC4, 1U, EPIC_CLC_SourceCode(1U, CLC_IN_FOSC));
    epic_harness_tick();
    ok &= (read_cell(CLC4) == 0U);
    pic16f1508_sim_drive_clc_input(3U, 1U, 1U);
    epic_harness_tick();
    ok &= (read_cell(CLC4) == 1U);

    /* CLCIN1 follows its own pin: CLC1 on D4 reads RC7, not RA3. */
    setup_tap(CLC1, 4U, EPIC_CLC_SourceCode(4U, CLC_IN_CLCIN1));
    pic16f1508_sim_drive_input('C', 7U, 1U);
    epic_harness_tick();
    ok &= (read_cell(CLC1) == 1U);
    pic16f1508_sim_drive_input('C', 7U, 0U);
    epic_harness_tick();
    ok &= (read_cell(CLC1) == 0U);

    /* Same-clock flop pair through the SEL tap: CLC2 samples the settled
     * CLC1 output, so the pair advances one stage per shared T2 clock. */
    setup_flop(CLC1, EPIC_CLC_SourceCode(2U, CLC_IN_FOSC));
    setup_flop(CLC2, EPIC_CLC_SourceCode(2U, CLC_IN_LC1_OUT));
    pic16f1508_sim_drive_clc_input(0U, 2U, 1U);
    EPIC_REG8(PIC_REG_TMR2) = 0U;
    EPIC_REG8(PIC_REG_PR2) = 1U;
    EPIC_REG8(PIC_REG_T2CON) = (uint8_t)(PIC_T2CON_TMR2ON | (3U << PIC_T2CON_T2OUTPS_SHIFT));
    epic_harness_tick();
    ok &= (read_cell(CLC1) == 0U);
    ok &= (read_cell(CLC2) == 0U);
    epic_harness_tick();
    ok &= (read_cell(CLC1) == 1U);
    ok &= (read_cell(CLC2) == 0U);
    epic_harness_tick();
    ok &= (read_cell(CLC1) == 1U);
    ok &= (read_cell(CLC2) == 0U);
    epic_harness_tick();
    ok &= (read_cell(CLC1) == 1U);
    ok &= (read_cell(CLC2) == 1U);

    /* Opposite-order pair: CLC2 samples CLC3, so the cell order cannot
     * smuggle a second stage into one shared clock either. CLC4 sits out:
     * its D1 is still hooked high from the FOSC check, which would hide
     * the T2 clock from its G1. */
    setup_flop(CLC3, EPIC_CLC_SourceCode(2U, CLC_IN_FOSC));
    setup_flop(CLC2, EPIC_CLC_SourceCode(2U, CLC_IN_LC3_OUT));
    pic16f1508_sim_drive_clc_input(2U, 2U, 1U);
    epic_harness_tick();
    ok &= (read_cell(CLC3) == 0U);
    ok &= (read_cell(CLC2) == 1U);
    epic_harness_tick();
    ok &= (read_cell(CLC3) == 1U);
    ok &= (read_cell(CLC2) == 0U);
    epic_harness_tick();
    ok &= (read_cell(CLC3) == 1U);
    ok &= (read_cell(CLC2) == 0U);
    epic_harness_tick();
    ok &= (read_cell(CLC3) == 1U);
    ok &= (read_cell(CLC2) == 1U);
    ok &= (read_cell(CLC1) == 1U);
    EPIC_REG8(PIC_REG_T2CON) = 0U;

    epic_harness_log("clc/sel: %s\n", ok ? "PASS" : "FAIL");
    return epic_harness_report(ok);
}
