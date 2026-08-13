/*
 * epic-settings on-target demo: an EEPROM-backed settings blob that
 * survives power cycles. On the first boot (blank EEPROM) or after a
 * corrupt blob, load_or_default applies the defaults and persists them
 * so the next boot sees a valid blob; the demo then modifies one field,
 * saves the new blob, and reloads it to show the round-trip. Every step
 * is reported over the serial layer at 9600 baud.
 */

#include "epic_hal.h"
#include "epic_serial.h"
#include "epic_settings.h"

#define SETTINGS_ADDR 0x10u   /* blob base; addr + size + 2 CRC must fit EEPROM */
#define BAUD_RATE     9600u

typedef struct {
    uint8_t  mode;       /* operating mode 0..3 */
    uint16_t threshold;  /* trigger threshold 0..1000 */
    uint8_t  flags;      /* enable bitfield */
} app_settings_t;

/**
 * @brief Transmit a NUL-terminated string over the serial ring.
 */
static void putstr(const char *s)
{
    int len = 0;
    while (s[len] != '\0') {
        len++;
    }
    epic_serial_write((const uint8_t *)s, len);
}

/**
 * @brief Transmit an unsigned value as decimal digits.
 */
static void putu16(uint16_t v)
{
    char buf[5];
    int n = 0, i;
    do {
        buf[n++] = (char)('0' + (int)(v % 10u));
        v /= 10u;
    } while (v > 0u);
    for (i = 0; i < n / 2; i++) {
        char t = buf[i];
        buf[i] = buf[n - 1 - i];
        buf[n - 1 - i] = t;
    }
    epic_serial_write((const uint8_t *)buf, n);
}

/**
 * @brief Transmit a byte as two uppercase hex digits.
 */
static void putx8(uint8_t v)
{
    static const char hex[] = "0123456789ABCDEF";
    char buf[2];
    buf[0] = hex[(v >> 4) & 0x0Fu];
    buf[1] = hex[v & 0x0Fu];
    epic_serial_write((const uint8_t *)buf, 2);
}

/**
 * @brief Report one settings blob over serial.
 */
static void report_cfg(const char *label, const app_settings_t *cfg)
{
    putstr(label);
    putstr(" mode=");
    putu16(cfg->mode);
    putstr(" threshold=");
    putu16(cfg->threshold);
    putstr(" flags=0x");
    putx8(cfg->flags);
    putstr("\r\n");
}

/**
 * @brief Load-or-default a settings blob, modify it, save, and reload
 *        to demonstrate the EEPROM round-trip.
 */
int main(void)
{
    epic_serial_init(FOSC_HZ, BAUD_RATE);
    EPIC_IRQ_Restore(1);

    static const app_settings_t defaults = { 1u, 250u, 0x03u };
    app_settings_t cfg;

    /* First boot (blank EEPROM) or a corrupt region: the defaults are
     * copied into cfg and persisted, so the next boot loads a valid
     * blob. The return is false exactly when defaults were applied. */
    bool first_boot = !epic_settings_load_or_default(SETTINGS_ADDR, &cfg,
                                                     (uint8_t)sizeof(cfg),
                                                     &defaults);
    putstr("epic-settings: ");
    putstr(first_boot ? "first boot, defaults applied\r\n"
                      : "stored blob loaded\r\n");
    report_cfg("loaded", &cfg);

    /* Modify one field and persist the new blob. */
    cfg.threshold = 375u;
    cfg.flags |= 0x04u;
    if (epic_settings_save(SETTINGS_ADDR, &cfg, (uint8_t)sizeof(cfg))) {
        putstr("save: modified blob persisted\r\n");
    } else {
        putstr("save: EEPROM write failed\r\n");
    }

    /* Reload from EEPROM to show the save/load round-trip. */
    app_settings_t back;
    if (epic_settings_load(SETTINGS_ADDR, &back, (uint8_t)sizeof(back))) {
        report_cfg("reload", &back);
    } else {
        putstr("reload: blob invalid (blank or corrupt)\r\n");
    }

    for (;;) {
        EPIC_WDT_Refresh();
    }
}
