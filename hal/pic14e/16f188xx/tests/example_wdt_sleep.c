/**
 * WDT, Sleep, Idle and reset-status host-sim probe. Host-only: Refresh
 * and Sleep only count in the simulator, so it is not in the XC8 build
 * (the mdb gate probe is example_gpio_mdb.c). Covers the period
 * encoding, the SWDTEN switch, a period pinned by CONFIG3, validation,
 * the refresh and sleep counters, IDLEN handling and the active-low
 * BOR/POR flags.
 *
 * Expected register image: WDTCON0 = (WDTPS << 1) | SWDTEN, so
 * 1 ms = 0x01, 2 s = 0x17, 256 s = 0x25; POR reads 0x16 (2 s, off).
 * PCON0 after a Power-on Reset reads 0x3D (POR clear = occurred, BOR set =
 * not); re-arming POR gives 0x3F.
 */
#include <stddef.h>
#include "pic16f188xx.h"
#include "pic16f188xx_sim.h"
#include "core/epic_harness.h"
#include "core/pic16f188xx_wdt_sleep.h"

/**
 * @brief WDT/Sleep checks on the host simulator.
 * @return Harness result: 0 on PASS, 1 on FAIL.
 */
int main(void)
{
    int ok = 1;

    epic_harness_init(10000UL);

    /* POR image: 2 s period, software enable off. */
    ok &= (EPIC_REG8(PIC_REG_WDTCON0) == 0x16U);
    ok &= (EPIC_REG8(PIC_REG_CPUDOZE) == 0x00U);

    /* Period encoding and the enable bit. */
    ok &= (EPIC_WDT_Enable(WDT_PERIOD_1MS) == EPIC_OK);
    ok &= (EPIC_REG8(PIC_REG_WDTCON0) == 0x01U);
    ok &= (EPIC_WDT_Enable(WDT_PERIOD_2S) == EPIC_OK);
    ok &= (EPIC_REG8(PIC_REG_WDTCON0) == 0x17U);
    ok &= (EPIC_WDT_Enable(WDT_PERIOD_256S) == EPIC_OK);
    ok &= (EPIC_REG8(PIC_REG_WDTCON0) == 0x25U);
    ok &= (EPIC_WDT_Disable() == EPIC_OK);
    ok &= (EPIC_REG8(PIC_REG_WDTCON0) == 0x24U);

    /* An out-of-range period leaves WDTCON0 alone. */
    ok &= (EPIC_WDT_Enable((WDT_PeriodTypeDef)19) == EPIC_INVALID);
    ok &= (EPIC_REG8(PIC_REG_WDTCON0) == 0x24U);

    /* CONFIG3 pins the period at 1 s: a different request still sets
     * SWDTEN but reports EPIC_ERROR, the pinned one succeeds. */
    pic16f188xx_sim_wdt_set_fixed_period((int8_t)WDT_PERIOD_1S);
    ok &= (EPIC_REG8(PIC_REG_WDTCON0) == 0x14U);
    ok &= (EPIC_WDT_Enable(WDT_PERIOD_4S) == EPIC_ERROR);
    ok &= (EPIC_REG8(PIC_REG_WDTCON0) == 0x15U);
    ok &= (EPIC_WDT_Enable(WDT_PERIOD_1S) == EPIC_OK);
    pic16f188xx_sim_wdt_set_fixed_period(-1);

    /* Refresh and Sleep count; Idle sets IDLEN around one sleep and puts
     * it back. */
    ok &= (pic16f188xx_sim_wdt_refresh_count() == 0U);
    EPIC_WDT_Refresh();
    EPIC_WDT_Refresh();
    ok &= (pic16f188xx_sim_wdt_refresh_count() == 2U);
    EPIC_Sleep_Enter();
    ok &= (pic16f188xx_sim_sleep_count() == 1U);
    ok &= (pic16f188xx_sim_idle_count() == 0U);
    EPIC_Idle_Enter();
    ok &= (pic16f188xx_sim_sleep_count() == 2U);
    ok &= (pic16f188xx_sim_idle_count() == 1U);
    ok &= ((EPIC_REG8(PIC_REG_CPUDOZE) & PIC_CPUDOZE_IDLEN) == 0U);

    /* IDLEN set by the caller survives Idle, and a plain Sleep then
     * enters Idle in hardware. */
    EPIC_BIT_SET(EPIC_REG8(PIC_REG_CPUDOZE), PIC_CPUDOZE_IDLEN);
    EPIC_Idle_Enter();
    ok &= ((EPIC_REG8(PIC_REG_CPUDOZE) & PIC_CPUDOZE_IDLEN) != 0U);
    ok &= (pic16f188xx_sim_idle_count() == 2U);
    EPIC_Sleep_Enter();
    ok &= (pic16f188xx_sim_idle_count() == 3U);

    /* Reset status is active low: POR occurred, BOR not. */
    ok &= (EPIC_REG8(PIC_REG_PCON0) == 0x3DU);
    ok &= (EPIC_POR_GetStatus() == 1U);
    ok &= (EPIC_BOR_GetStatus() == 0U);
    EPIC_POR_ClearFlag();
    ok &= (EPIC_REG8(PIC_REG_PCON0) == 0x3FU);
    ok &= (EPIC_POR_GetStatus() == 0U);
    EPIC_BIT_CLR(EPIC_REG8(PIC_REG_PCON0), PIC_PCON0_nBOR);
    ok &= (EPIC_BOR_GetStatus() == 1U);
    EPIC_BOR_ClearFlag();
    ok &= (EPIC_BOR_GetStatus() == 0U);
    ok &= (EPIC_REG8(PIC_REG_PCON0) == 0x3FU);

    epic_harness_log("wdt_sleep: %s\n", ok ? "PASS" : "FAIL");
    return epic_harness_report(ok);
}
