/**
 * CLC input-source table host-sim probe. Host-only: reads the flat simulator
 * SFR array through EPIC_REG8, so it is not in the XC8 build. Checks
 * EPIC_CLC_SourceCode and the CLC_IN_* names against the literal columns of
 * DS40001609E Table 24-1 (page 258), and the SEL images Init writes from them.
 */
#include <stddef.h>
#include "pic16f1508.h"
#include "peripherals/pic16f1508_clc.h"
#include "peripherals/pic16f1508_clc_src.h"
#include "core/epic_harness.h"

#define NA EPIC_CLC_SRC_INVALID

/* Table 24-1 columns lcxd1..lcxd4 (D1S..D4S) per LCx_in[0..15]; NA is a dash. */
static const uint8_t k_code[4][16] = {
    {0U, 1U, 2U, 3U, 4U, 5U, 6U, 7U, NA, NA, NA, NA, NA, NA, NA, NA},
    {NA, NA, NA, NA, 0U, 1U, 2U, 3U, 4U, 5U, 6U, 7U, NA, NA, NA, NA},
    {NA, NA, NA, NA, NA, NA, NA, NA, 0U, 1U, 2U, 3U, 4U, 5U, 6U, 7U},
    {4U, 5U, 6U, 7U, NA, NA, NA, NA, NA, NA, NA, NA, 0U, 1U, 2U, 3U}
};

/* The CLC 1..CLC 4 columns of Table 24-1, signal names in row order. */
static const uint8_t k_names[4][16] = {
    {CLC_IN_CLCIN0, CLC_IN_CLCIN1, CLC_IN_C1OUT, CLC_IN_C2OUT,
     CLC_IN_FOSC, CLC_IN_T0_OVERFLOW, CLC_IN_T1_OVERFLOW, CLC_IN_T2_MATCH,
     CLC_IN_LC1_OUT, CLC_IN_LC2_OUT, CLC_IN_LC3_OUT, CLC_IN_LC4_OUT,
     CLC1_IN_NCO1_OUT, CLC1_IN_HFINTOSC, CLC1_IN_PWM3_OUT, CLC1_IN_PWM4_OUT},
    {CLC_IN_CLCIN0, CLC_IN_CLCIN1, CLC_IN_C1OUT, CLC_IN_C2OUT,
     CLC_IN_FOSC, CLC_IN_T0_OVERFLOW, CLC_IN_T1_OVERFLOW, CLC_IN_T2_MATCH,
     CLC_IN_LC1_OUT, CLC_IN_LC2_OUT, CLC_IN_LC3_OUT, CLC_IN_LC4_OUT,
     CLC2_IN_LFINTOSC, CLC2_IN_FRC, CLC2_IN_PWM1_OUT, CLC2_IN_PWM2_OUT},
    {CLC_IN_CLCIN0, CLC_IN_CLCIN1, CLC_IN_C1OUT, CLC_IN_C2OUT,
     CLC_IN_FOSC, CLC_IN_T0_OVERFLOW, CLC_IN_T1_OVERFLOW, CLC_IN_T2_MATCH,
     CLC_IN_LC1_OUT, CLC_IN_LC2_OUT, CLC_IN_LC3_OUT, CLC_IN_LC4_OUT,
     CLC3_IN_TX_OUT, CLC3_IN_LFINTOSC, CLC3_IN_PWM2_OUT, CLC3_IN_PWM3_OUT},
    {CLC_IN_CLCIN0, CLC_IN_CLCIN1, CLC_IN_C1OUT, CLC_IN_C2OUT,
     CLC_IN_FOSC, CLC_IN_T0_OVERFLOW, CLC_IN_T1_OVERFLOW, CLC_IN_T2_MATCH,
     CLC_IN_LC1_OUT, CLC_IN_LC2_OUT, CLC_IN_LC3_OUT, CLC_IN_LC4_OUT,
     CLC4_IN_SCK_OUT, CLC4_IN_SDO_OUT, CLC4_IN_PWM1_OUT, CLC4_IN_PWM4_OUT}
};

/**
 * @brief Run the CLC input-source table checks on the host simulator.
 * @return Harness result: 0 on PASS, 1 on FAIL.
 */
int main(void)
{
    int ok = 1;
    CLC_HandleTypeDef h = {0};
    unsigned int mux;
    unsigned int in;
    unsigned int cell;
    uint8_t before[2];

    epic_harness_init(10000UL);

    for (mux = 0U; mux < 4U; mux++)
    {
        for (in = 0U; in < 16U; in++)
        {
            ok &= (EPIC_CLC_SourceCode((uint8_t)(mux + 1U), (uint8_t)in) == k_code[mux][in]);
        }
    }
    for (cell = 0U; cell < 4U; cell++)
    {
        for (in = 0U; in < 16U; in++)
        {
            ok &= (k_names[cell][in] == in);
        }
    }

    /* Out-of-range arguments are invalid, never a wrapped code. */
    ok &= (EPIC_CLC_SourceCode(0U, 0U) == NA);
    ok &= (EPIC_CLC_SourceCode(5U, 0U) == NA);
    ok &= (EPIC_CLC_SourceCode(1U, 16U) == NA);
    ok &= (EPIC_CLC_SourceCode(4U, 255U) == NA);

    /* CLC3: D1=FOSC, D2=LC1_out, D3=TX_out, D4=C2OUT_sync lands as SEL0 0x44, SEL1 0x74. */
    h.Instance = CLC3;
    h.Source[0] = EPIC_CLC_SourceCode(1U, CLC_IN_FOSC);
    h.Source[1] = EPIC_CLC_SourceCode(2U, CLC_IN_LC1_OUT);
    h.Source[2] = EPIC_CLC_SourceCode(3U, CLC3_IN_TX_OUT);
    h.Source[3] = EPIC_CLC_SourceCode(4U, CLC_IN_C2OUT);
    ok &= (EPIC_CLC_Init(&h) == EPIC_OK);
    ok &= (EPIC_REG8(PIC_REG_CLC3SEL0) == 0x44U);
    ok &= (EPIC_REG8(PIC_REG_CLC3SEL1) == 0x74U);

    /* A multiplexer that cannot see the input yields an invalid code that Init
     * rejects without touching the registers. */
    before[0] = EPIC_REG8(PIC_REG_CLC3SEL0);
    before[1] = EPIC_REG8(PIC_REG_CLC3SEL1);
    h.Source[0] = EPIC_CLC_SourceCode(1U, CLC3_IN_TX_OUT);
    h.Source[1] = EPIC_CLC_SourceCode(2U, CLC_IN_CLCIN0);
    ok &= (EPIC_CLC_Init(&h) == EPIC_INVALID);
    ok &= (EPIC_REG8(PIC_REG_CLC3SEL0) == before[0]);
    ok &= (EPIC_REG8(PIC_REG_CLC3SEL1) == before[1]);

    epic_harness_log("clc/src: %s\n", ok ? "PASS" : "FAIL");
    return epic_harness_report(ok);
}
