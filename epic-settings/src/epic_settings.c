/**
 * EEPROM-backed settings blobs with CRC-16 validation. EPIC_EEPROM_WriteBuffer
 * is unsafe for multi-byte writes (it loops to the next byte without waiting
 * for EEIF completion), so this module sequences byte-at-a-time writes: start,
 * wait, clear flag, repeat. The host sim drives each write to completion
 * immediately, then follows the same poll/clear contract as target code.
 */

#include "epic_settings.h"

#include "epic_hal.h"
#include "core/epic_harness.h"

#include <string.h>

#if !defined(__XC8)
  #if defined(PIC18F2455) || defined(PIC18F2550) || defined(PIC18F4455) || defined(PIC18F4550)
    #include "pic18fxx5x_sim.h"
    /**
     * @brief Drive one sim EEPROM byte write and raise EEIF (PIC18).
     *
     * @param addr EEPROM address written
     * @param data byte written
     */
    static void settings_sim_complete(uint8_t addr, uint8_t data)
    {
        pic18_sim_drive_eeprom_byte(addr, data);
        epic_sfr_write8(PIC_REG_PIR2, (uint8_t)(epic_sfr_read8(PIC_REG_PIR2) | PIC_PIR2_EEIF));
    }
  #else
    #include "pic16f87xa_sim.h"
    /**
     * @brief Drive one sim EEPROM byte write and raise EEIF (PIC16).
     *
     * @param addr EEPROM address written
     * @param data byte written
     */
    static void settings_sim_complete(uint8_t addr, uint8_t data)
    {
        pic16f87xa_sim_drive_eeprom_byte(addr, data);
        EPIC_REG8(PIC_REG_PIR2) |= PIC_PIR2_EEIF;
    }
  #endif
#endif

/**
 * @brief Feed one byte into a CRC-16-CCITT (poly 0x1021) accumulator.
 *
 * @param crc  current CRC value
 * @param byte byte to fold in
 * @return the updated CRC
 */
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

/**
 * @brief Compute the CRC-16-CCITT of a byte buffer, initial value 0xFFFF.
 *
 * @param data buffer to checksum
 * @param size number of bytes
 * @return the CRC value
 */
static uint16_t settings_crc16(const uint8_t *data, uint8_t size)
{
    uint16_t crc = 0xFFFFu;
    for (uint8_t i = 0; i < size; i++) {
        crc = settings_crc16_update(crc, data[i]);
    }
    return crc;
}

/**
 * @brief Write len bytes sequentially, waiting for each write to complete.
 *
 * The EEPROM driver's WriteBuffer is unsafe for multi-byte writes, so
 * each byte is written and polled to completion before the next.
 *
 * @param start first EEPROM address to write
 * @param buf   bytes to write
 * @param len   number of bytes
 * @return true if every byte wrote; false if any write was rejected
 */
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

/**
 * @brief Save size bytes from data to EEPROM at eeprom_addr, followed by
 *        a trailing CRC-16-CCITT.
 *
 * @param eeprom_addr  EEPROM address of the blob
 * @param data         payload to save
 * @param size         payload size in bytes
 * @return true if the payload and CRC trailer were written
 */
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

/**
 * @brief Load size bytes from EEPROM at eeprom_addr into data, validating
 *        the trailing CRC-16-CCITT first.
 *
 * On failure (blank/corrupt region) data is left untouched.
 *
 * @param eeprom_addr  EEPROM address of the blob
 * @param data         buffer to receive the payload
 * @param size         payload size in bytes
 * @return true if the stored blob was valid and copied
 */
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

/**
 * @brief Load if valid, otherwise copy default_data into data and persist
 *        it to EEPROM so the next boot sees it as already valid.
 *
 * @param eeprom_addr   EEPROM address of the blob
 * @param data          buffer to receive the payload
 * @param size          payload size in bytes
 * @param default_data  fallback payload, applied and saved on load failure
 * @return true if a valid stored blob existed, false if defaults were
 *         applied
 */
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
