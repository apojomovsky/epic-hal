/**
 * CLC SEL-decode host-sim probe. Host-only: configures cells through
 * EPIC_CLC_Init and checks that undriven Dx levels decode their SEL codes
 * (Table 24-1): LCx_OUT taps post-POL with a one-step cascade delay, timer
 * sources follow the PIR1 flags, CLCxIN0/1 follow their fixed pins, and
 * unmodelled sources read 0 unless the Dx hook overrides them.
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

    /* Pin decode with a one-step cascade: CLC1 answers at once, the LC1_OUT
     * tap one step later. */
    pic16f1508_sim_drive_input('A', 3U, 1U);
    epic_harness_tick();
    ok &= (read_cell(CLC1) == 1U);
    ok &= (read_cell(CLC2) == 0U);
    epic_harness_tick();
    ok &= (read_cell(CLC2) == 1U);

    /* Post-POL tap: inverting CLC1 inverts what CLC2 sees. */
    EPIC_REG8(PIC_REG_CLC1POL) |= PIC_CLCPOL_POL;
    epic_harness_tick();
    epic_harness_tick();
    ok &= (read_cell(CLC1) == 0U);
    ok &= (read_cell(CLC2) == 0U);
    EPIC_REG8(PIC_REG_CLC1POL) &= (uint8_t)~PIC_CLCPOL_POL;
    epic_harness_tick();
    epic_harness_tick();
    ok &= (read_cell(CLC1) == 1U);
    ok &= (read_cell(CLC2) == 1U);

    /* Timer decode follows the latched flags until software clears them. */
    EPIC_REG8(PIC_REG_PIR1) |= PIC_PIR1_TMR1IF;
    epic_harness_tick();
    ok &= (read_cell(CLC3) == 1U);
    EPIC_REG8(PIC_REG_PIR1) &= (uint8_t)~PIC_PIR1_TMR1IF;
    epic_harness_tick();
    ok &= (read_cell(CLC3) == 0U);
    EPIC_REG8(PIC_REG_PIR1) |= PIC_PIR1_TMR2IF;
    epic_harness_tick();
    ok &= (read_cell(CLC4) == 1U);
    EPIC_REG8(PIC_REG_PIR1) &= (uint8_t)~PIC_PIR1_TMR2IF;
    epic_harness_tick();
    ok &= (read_cell(CLC4) == 0U);

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

    epic_harness_log("clc/sel: %s\n", ok ? "PASS" : "FAIL");
    return epic_harness_report(ok);
}
