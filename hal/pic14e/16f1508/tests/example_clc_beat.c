/**
 * CLC beat-frequency auto-dimmer probe, TB3273 remainder. Host-only: the S-R
 * latch wiring matches the MCC clc1.c beat pair (S on D1+D2, R on D3+D4),
 * and the two pulse trains keep the MCC period ratio (TMR2 4.096 ms against
 * TMR4 4.08 ms, scaled to 256 vs 255 steps). Dx levels are driven through
 * pic16f1508_sim_drive_clc_input, standing in for the timer outputs until
 * TMR2/PWM drivers land. S+R coincidence stays S-wins by host assumption.
 */
#include <stddef.h>
#include "pic16f1508.h"
#include "peripherals/pic16f1508_clc.h"
#include "peripherals/pic16f1508_clc_src.h"
#include "peripherals/pic16f1508_clc_gls.h"
#include "core/epic_harness.h"
#include "pic16f1508_sim.h"

/* MCC beat ratio: 4.096 ms vs 4.08 ms scales to 256 vs 255 single-step pulses. */
#define BEAT_S_PERIOD 256U
#define BEAT_R_PERIOD 255U

/**
 * @brief Drive one beat step: S pulse every 256 steps, R every 255.
 * @param t Step index from the beat start.
 */
static void beat_step(unsigned int t)
{
    uint8_t d = 0U;

    if ((t % BEAT_S_PERIOD) == 0U)
    {
        d |= 0x03U;
    }
    if ((t % BEAT_R_PERIOD) == 0U)
    {
        d |= 0x0CU;
    }
    {
        uint8_t i;

        for (i = 0U; i < 4U; i++)
        {
            pic16f1508_sim_drive_clc_input(0U, (uint8_t)(i + 1U), (uint8_t)((d >> i) & 1U));
        }
    }
    epic_harness_tick();
}

/**
 * @brief Snapshot the CLC1 wiring images the run must leave untouched.
 * @param img Receives CON, POL, SEL0, SEL1, GLS0..GLS3.
 */
static void snap_wiring(uint8_t img[8])
{
    img[0] = EPIC_REG8(PIC_REG_CLC1CON);
    img[1] = EPIC_REG8(PIC_REG_CLC1POL);
    img[2] = EPIC_REG8(PIC_REG_CLC1SEL0);
    img[3] = EPIC_REG8(PIC_REG_CLC1SEL1);
    img[4] = EPIC_REG8(PIC_REG_CLC1GLS0);
    img[5] = EPIC_REG8(PIC_REG_CLC1GLS1);
    img[6] = EPIC_REG8(PIC_REG_CLC1GLS2);
    img[7] = EPIC_REG8(PIC_REG_CLC1GLS3);
}

/**
 * @brief Beat-frequency dimmer checks: swept duty at fixed output rate.
 * @return Harness result: 0 on PASS, 1 on FAIL.
 */
int main(void)
{
    int ok = 1;
    CLC_HandleTypeDef h;
    uint8_t before[8];
    unsigned int t;
    unsigned int win_high = 0U;
    unsigned int win_min = BEAT_S_PERIOD + 1U;
    unsigned int win_max = 0U;
    unsigned long total_high = 0UL;
    unsigned int sets = 0U;
    uint8_t prev = 0U;
    uint8_t level = 0U;

    epic_harness_init(70000UL);

    h.Instance = CLC1;
    h.Mode = 3U;
    h.Source[0] = EPIC_CLC_SourceCode(1U, CLC_IN_T2_MATCH);
    h.Source[1] = EPIC_CLC_SourceCode(2U, CLC_IN_T2_MATCH);
    h.Source[2] = EPIC_CLC_SourceCode(3U, CLC1_IN_PWM3_OUT);
    h.Source[3] = EPIC_CLC_SourceCode(4U, CLC1_IN_PWM3_OUT);
    h.Gls[0] = EPIC_CLC_GLSInput(1U, 0U, 1U);
    h.Gls[1] = EPIC_CLC_GLSInput(2U, 0U, 1U);
    h.Gls[2] = EPIC_CLC_GLSInput(3U, 0U, 1U);
    h.Gls[3] = EPIC_CLC_GLSInput(4U, 0U, 1U);
    h.GatePolarity = 0U;
    h.OutputPolarity = 0U;
    h.OutputEnable = 0U;
    h.InterruptRising = 0U;
    h.InterruptFalling = 0U;
    h.Callback = NULL;
    ok &= (EPIC_CLC_Init(&h) == EPIC_OK);
    snap_wiring(before);

    /* One full beat: 255 S windows of 255-drifting R phase. */
    for (t = 0U; t < (BEAT_S_PERIOD * BEAT_R_PERIOD); t++)
    {
        beat_step(t);
        ok &= (EPIC_CLC_ReadOutput(CLC1, &level) == EPIC_OK);
        if ((prev == 0U) && (level != 0U))
        {
            sets++;
        }
        prev = level;
        if (level != 0U)
        {
            win_high++;
            total_high++;
        }
        if ((t % BEAT_S_PERIOD) == (BEAT_S_PERIOD - 1U))
        {
            if (win_high < win_min)
            {
                win_min = win_high;
            }
            if (win_high > win_max)
            {
                win_max = win_high;
            }
            win_high = 0U;
        }
    }

    /* Duty sweeps near-empty to near-full across the beat, averaging half. */
    ok &= (win_min <= 16U);
    ok &= (win_max >= 240U);
    ok &= (total_high > (105UL * 255UL));
    ok &= (total_high < (150UL * 255UL));

    /* One set per S window: the output rate stays fixed while duty sweeps. */
    ok &= (sets == 255U);

    /* No CPU involvement: the wiring images are untouched and no flag raised. */
    {
        uint8_t after[8];
        uint8_t i;

        snap_wiring(after);
        for (i = 0U; i < 8U; i++)
        {
            ok &= (after[i] == before[i]);
        }
    }
    ok &= ((EPIC_REG8(PIC_REG_PIR3) & PIC_PIR3_CLC1IF) == 0U);

    ok &= (EPIC_CLC_DeInit(CLC1) == EPIC_OK);
    return epic_harness_report(ok);
}
