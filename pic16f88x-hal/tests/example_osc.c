/* Oscillator driver smoke test on the sim backend: internal-frequency
 * select (OSCCON<IRCF>), system-clock source (SCS), tuning (OSCTUNE),
 * and the fail-safe clock monitor interrupt (OSFIF/OSFIE). */

#include "pic16f88x.h"
#include "pic16f88x_sim.h"
#include "pic16f88x_sfr.h"
#include "peripherals/pic16f88x_osc.h"
#include "core/pic16_irq.h"
#include "core/epic_harness.h"
#include <stdio.h>

#define CHECK(cond, msg) do { \
    if (!(cond)) { printf("FAIL: %s\n", msg); return 1; } \
} while (0)

static volatile uint32_t osf_fires = 0;

/**
 * @brief Count oscillator-fail callbacks.
 */
static void on_osc_fail(void)
{
    osf_fires++;
}

/**
 * @brief Smoke-test the oscillator driver: IRCF, SCS, OSCTUNE, FSCM.
 */
int main(void)
{
    pic16f88x_sim_reset();
    pic16f88x_sim_set_irq_callback(epic_dispatch_all_irqs);

    /* 1. Set the internal frequency to 8 MHz (IRCF = 111). */
    EPIC_OSC_SetInternalFreq(OSC_IRCF_8MHZ);
    CHECK(EPIC_OSC_GetInternalFreq() == 0x7U, "IRCF not 8 MHz after SetInternalFreq");

    /* 2. Run the system clock from the internal oscillator (SCS = 1). */
    EPIC_OSC_SetSystemClockSource(1U);
    {
        uint8_t prev = (EPIC_REG8(PIC_REG_STATUS) >> 5) & 0x03U;
        pic_select_bank(1);
        uint8_t osccon = EPIC_REG8(PIC_REG_OSCCON);
        pic_select_bank(prev);
        CHECK((osccon & PIC_OSCCON_SCS) != 0U, "SCS not set after SetSystemClockSource(1)");
        CHECK((osccon & PIC_OSCCON_IRCF_MASK) == (0x7U << PIC_OSCCON_IRCF_POS),
              "IRCF not 8 MHz in OSCCON");
    }

    /* 3. Tune the HFINTOSC. */
    EPIC_OSC_Tune(0x05U);
    {
        uint8_t prev = (EPIC_REG8(PIC_REG_STATUS) >> 5) & 0x03U;
        pic_select_bank(1);
        uint8_t osctune = EPIC_REG8(PIC_REG_OSCTUNE);
        pic_select_bank(prev);
        CHECK(osctune == 0x05U, "OSCTUNE not 0x05 after Tune");
    }

    /* 4. Fail-safe monitor: arm the callback, drive an OSF event. */
    CHECK(EPIC_OSC_FailSafeInit(on_osc_fail) == EPIC_OK, "FailSafeInit failed");
    EPIC_IRQ_Restore(1);
    pic16f88x_sim_drive_osc_fail();
    /* The dispatcher routes OSFIF to OSF_IRQHandler, which clears the
     * flag and fires the callback. Assert on the callback count. */
    CHECK(osf_fires >= 1U, "OSF callback not fired");

    /* 5. Switch back to the external clock (SCS = 0). */
    EPIC_OSC_SetSystemClockSource(0U);
    {
        uint8_t prev = (EPIC_REG8(PIC_REG_STATUS) >> 5) & 0x03U;
        pic_select_bank(1);
        uint8_t osccon = EPIC_REG8(PIC_REG_OSCCON);
        pic_select_bank(prev);
        CHECK((osccon & PIC_OSCCON_SCS) == 0U, "SCS not cleared after SetSystemClockSource(0)");
    }

    printf("OK: oscillator driver, IRCF/SCS/tune/FSCM all pass.\n");
    return 0;
}
