/**
 * @file    epic_settings.c
 * @brief   EEPROM-backed settings blobs with CRC-16 validation.
 *
 * @details
 *   EPIC_EEPROM_WriteBuffer is unsafe for multi-byte writes (it loops to
 *   the next byte without waiting for EEIF completion), so this module
 *   sequences its own byte-at-a-time writes: start, wait, clear flag,
 *   repeat. The host sim has no timed EEPROM model, so it drives each
 *   simulated write to completion immediately, then follows the same
 *   poll/clear contract as target code.
 */

#include "epic_settings.h"

#include "epic_hal.h"
#include "core/epic_harness.h"

#include <string.h>

#if !defined(__XC8)
  #if defined(PIC18F2455) || defined(PIC18F2550) || defined(PIC18F4455) || defined(PIC18F4550)
    #include "pic18fxx5x_sim.h"
    static void settings_sim_complete(uint8_t addr, uint8_t data)
    {
        pic18_sim_drive_eeprom_byte(addr, data);
        epic_sfr_write8(PIC_REG_PIR2, (uint8_t)(epic_sfr_read8(PIC_REG_PIR2) | PIC_PIR2_EEIF));
    }
  #else
    #include "pic16f87xa_sim.h"
    static void settings_sim_complete(uint8_t addr, uint8_t data)
    {
        pic16f87xa_sim_drive_eeprom_byte(addr, data);
        PIC8_REG8(PIC_REG_PIR2) |= PIC_PIR2_EEIF;
    }
  #endif
#endif

static uint16_t settings_crc16_update(uint16_t crc, uint8_t byte)
{
    crc ^= (uint16_t)((uint16_t)byte << 8);
    for (uint8_t bit = 0; bit < 8u; bit++) {
        if ((crc & 0x8000u) != 0u) {
            crc = (uint16_t)((crc << 1) ^ 0x1021u);
        } else {
            crc <<= 1;
        }
    }
    return crc;
}

static uint16_t settings_crc16(const uint8_t *data, uint8_t size)
{
    uint16_t crc = 0xFFFFu;
    for (uint8_t i = 0; i < size; i++) {
        crc = settings_crc16_update(crc, data[i]);
    }
    return crc;
}

static bool settings_write_bytes(uint8_t start, const uint8_t *buf, uint8_t len)
{
    for (uint8_t i = 0; i < len; i++) {
        uint8_t addr = (uint8_t)(start + i);
        uint8_t data = buf[i];

        if (EPIC_EEPROM_WriteByte(addr, data) != EPIC_OK) {
            return false;
        }

#if !defined(__XC8)
        settings_sim_complete(addr, data);
#endif

        while (EPIC_EEPROM_IsWriteComplete() == 0u) {
            epic_harness_tick();
        }
        EPIC_EEPROM_ClearITFlag();
    }
    return true;
}

bool epic_settings_save(uint8_t eeprom_addr, const void *data, uint8_t size)
{
    const uint8_t *bytes = (const uint8_t *)data;
    uint16_t crc = settings_crc16(bytes, size);
    uint8_t trailer[2];

    if (!settings_write_bytes(eeprom_addr, bytes, size)) {
        return false;
    }

    trailer[0] = (uint8_t)(crc >> 8);
    trailer[1] = (uint8_t)(crc & 0xFFu);
    return settings_write_bytes((uint8_t)(eeprom_addr + size), trailer, 2u);
}

bool epic_settings_load(uint8_t eeprom_addr, void *data, uint8_t size)
{
    uint8_t *out = (uint8_t *)data;
    uint16_t crc = 0xFFFFu;

    for (uint8_t i = 0; i < size; i++) {
        crc = settings_crc16_update(crc, EPIC_EEPROM_ReadByte((uint8_t)(eeprom_addr + i)));
    }

    uint16_t stored_crc = (uint16_t)((uint16_t)EPIC_EEPROM_ReadByte((uint8_t)(eeprom_addr + size)) << 8);
    stored_crc |= EPIC_EEPROM_ReadByte((uint8_t)(eeprom_addr + size + 1u));
    if (stored_crc != crc) {
        return false;
    }

    for (uint8_t i = 0; i < size; i++) {
        out[i] = EPIC_EEPROM_ReadByte((uint8_t)(eeprom_addr + i));
    }
    return true;
}

bool epic_settings_load_or_default(uint8_t eeprom_addr, void *data, uint8_t size,
                                   const void *default_data)
{
    if (epic_settings_load(eeprom_addr, data, size)) {
        return true;
    }

    memcpy(data, default_data, size);
    (void)epic_settings_save(eeprom_addr, default_data, size);
    return false;
}
