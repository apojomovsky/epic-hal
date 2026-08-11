/** Host-sim smoke for saving, loading, and defaulting a settings blob. */

#include "epic_settings.h"
#include "core/epic_harness.h"

#if defined(PIC18F2455) || defined(PIC18F2550) || defined(PIC18F4455) || defined(PIC18F4550)
  #include "pic18fxx5x_sim.h"
  #define SIM_EEPROM_BYTE(addr, data)  pic18_sim_drive_eeprom_byte((addr), (data))
#else
  #include "pic16f87xa_sim.h"
  #define SIM_EEPROM_BYTE(addr, data)  pic16f87xa_sim_drive_eeprom_byte((addr), (data))
#endif

#include <string.h>

typedef struct {
    uint8_t mode;
    uint16_t threshold;
    uint8_t flags;
} app_settings_t;

/**
 * @brief Smoke-test saving, loading, and defaulting a settings blob.
 */
int main(void)
{
    epic_harness_init(1000u);

    const app_settings_t defaults = { 1u, 250u, 0x03u };
    const app_settings_t saved    = { 2u, 375u, 0x05u };
    app_settings_t cfg            = { 0u, 0u, 0u };

    epic_settings_load_or_default(0x10u, &cfg, (uint8_t)sizeof(cfg), &defaults);
    epic_harness_log("first boot: mode=%u threshold=%u flags=0x%02X\n",
                     cfg.mode, cfg.threshold, cfg.flags);

    epic_settings_save(0x10u, &saved, (uint8_t)sizeof(saved));
    memset(&cfg, 0, sizeof(cfg));
    epic_settings_load(0x10u, &cfg, (uint8_t)sizeof(cfg));
    epic_harness_log("loaded: mode=%u threshold=%u flags=0x%02X\n",
                     cfg.mode, cfg.threshold, cfg.flags);

    /* Corrupt one stored byte; the next load falls back to defaults. */
    SIM_EEPROM_BYTE(0x10u, 0xFFu);
    epic_settings_load_or_default(0x10u, &cfg, (uint8_t)sizeof(cfg), &defaults);
    epic_harness_log("after corruption: mode=%u threshold=%u flags=0x%02X\n",
                     cfg.mode, cfg.threshold, cfg.flags);

    return epic_harness_report(memcmp(&cfg, &defaults, sizeof(cfg)) == 0);
}
