/** Host tests for epic-settings over the simulated EEPROM backend. */

#include "epic_settings.h"
#include "core/epic_harness.h"

#if defined(PIC18F2455) || defined(PIC18F2550) || defined(PIC18F4455) || defined(PIC18F4550)
  #include "pic18fxx5x_sim.h"
  #define SIM_EEPROM_BYTE(addr, data)  pic18_sim_drive_eeprom_byte((addr), (data))
  #define SIM_EEPROM_DONE(addr, data)  pic18_sim_drive_eeprom_done((addr), (data))
#elif defined(PIC16F882) || defined(PIC16F883) || defined(PIC16F884) || \
      defined(PIC16F886) || defined(PIC16F887)
  #include "pic16f88x_sim.h"
  #define SIM_EEPROM_BYTE(addr, data)  pic16f88x_sim_drive_eeprom_byte((addr), (data))
  #define SIM_EEPROM_DONE(addr, data)  pic16f88x_sim_drive_eeprom_done((addr), (data))
#else
  #include "pic16f87xa_sim.h"
  #define SIM_EEPROM_BYTE(addr, data)  pic16f87xa_sim_drive_eeprom_byte((addr), (data))
  #define SIM_EEPROM_DONE(addr, data)  pic16f87xa_sim_drive_eeprom_done((addr), (data))
#endif

#include <stdio.h>
#include <string.h>

static int g_pass = 0;
static int g_fail = 0;

#define CHECK(cond, msg) \
    do { \
        if (cond) { \
            g_pass++; \
        } else { \
            printf("FAIL: %s\n", msg); \
            g_fail++; \
        } \
    } while (0)

typedef struct {
    uint8_t mode;
    uint16_t limit;
    uint8_t flags;
} settings_blob_t;

/** @brief Re-initialize the harness before each test. */
static void reset_env(void)
{
    epic_harness_init(1000u);
}

/** @brief Verify save followed by load round-trips the blob. */
static void test_save_then_load_roundtrip(void)
{
    reset_env();

    const settings_blob_t in = { 3u, 0x1234u, 0xA5u };
    settings_blob_t out = { 0u, 0u, 0u };

    CHECK(epic_settings_save(0x10u, &in, (uint8_t)sizeof(in)), "roundtrip: save succeeds");
    CHECK(epic_settings_load(0x10u, &out, (uint8_t)sizeof(out)), "roundtrip: load succeeds");
    CHECK(memcmp(&in, &out, sizeof(in)) == 0, "roundtrip: blob matches exactly");
}

/** @brief Verify loading a blank region fails without touching the output buffer. */
static void test_blank_region_fails_without_touching_output(void)
{
    reset_env();

    settings_blob_t out = { 9u, 0xBEEFu, 0x5Au };
    settings_blob_t before = out;

    CHECK(!epic_settings_load(0x20u, &out, (uint8_t)sizeof(out)), "blank: load reports invalid");
    CHECK(memcmp(&out, &before, sizeof(out)) == 0, "blank: output untouched");
}

/** @brief Verify corruption is detected without partially overwriting the output. */
static void test_corruption_detected_without_partial_overwrite(void)
{
    reset_env();

    const settings_blob_t in = { 1u, 0x2222u, 0x33u };
    settings_blob_t out = { 7u, 0x7777u, 0x77u };
    settings_blob_t before = out;

    CHECK(epic_settings_save(0x30u, &in, (uint8_t)sizeof(in)), "corrupt: save succeeds");
    SIM_EEPROM_BYTE(0x31u, (uint8_t)~((const uint8_t *)&in)[1]);

    CHECK(!epic_settings_load(0x30u, &out, (uint8_t)sizeof(out)), "corrupt: load reports invalid");
    CHECK(memcmp(&out, &before, sizeof(out)) == 0, "corrupt: output untouched");
}

/** @brief Verify load_or_default persists defaults on a blank region. */
static void test_load_or_default_persists_default(void)
{
    reset_env();

    const settings_blob_t def = { 4u, 0x4567u, 0xC3u };
    settings_blob_t out = { 0u, 0u, 0u };
    settings_blob_t verify = { 0u, 0u, 0u };

    CHECK(!epic_settings_load_or_default(0x40u, &out, (uint8_t)sizeof(out), &def),
          "default: blank region returns false");
    CHECK(memcmp(&out, &def, sizeof(def)) == 0, "default: caller buffer receives defaults");
    CHECK(epic_settings_load(0x40u, &verify, (uint8_t)sizeof(verify)),
          "default: persisted blob loads on next read");
    CHECK(memcmp(&verify, &def, sizeof(def)) == 0, "default: persisted blob matches defaults");
}

/** @brief Verify two blobs in separate EEPROM regions do not interfere. */
static void test_two_regions_do_not_interfere(void)
{
    reset_env();

    const settings_blob_t a = { 1u, 0x1111u, 0x11u };
    const settings_blob_t b = { 2u, 0x2222u, 0x22u };
    settings_blob_t out_a = { 0u, 0u, 0u };
    settings_blob_t out_b = { 0u, 0u, 0u };

    CHECK(epic_settings_save(0x50u, &a, (uint8_t)sizeof(a)), "regions: save A succeeds");
    CHECK(epic_settings_save(0x60u, &b, (uint8_t)sizeof(b)), "regions: save B succeeds");
    CHECK(epic_settings_load(0x50u, &out_a, (uint8_t)sizeof(out_a)), "regions: load A succeeds");
    CHECK(epic_settings_load(0x60u, &out_b, (uint8_t)sizeof(out_b)), "regions: load B succeeds");
    CHECK(memcmp(&out_a, &a, sizeof(a)) == 0, "regions: A preserved");
    CHECK(memcmp(&out_b, &b, sizeof(b)) == 0, "regions: B preserved");
}

/** @brief Verify CRC edge patterns (all-zero and all-0xFF payloads). */
static void test_crc_edge_patterns(void)
{
    reset_env();

    uint8_t zeros[8] = {0};
    uint8_t ones[8];
    uint8_t out[8];
    memset(ones, 0xFF, sizeof(ones));
    memset(out,  0x00, sizeof(out));

    CHECK(epic_settings_save(0x70u, zeros, (uint8_t)sizeof(zeros)), "crc edge: zero save succeeds");
    CHECK(epic_settings_load(0x70u, out, (uint8_t)sizeof(out)), "crc edge: zero load succeeds");
    CHECK(memcmp(out, zeros, sizeof(out)) == 0, "crc edge: zero data round-trips");

    memset(out, 0x00, sizeof(out));
    CHECK(epic_settings_save(0x80u, ones, (uint8_t)sizeof(ones)), "crc edge: 0xFF save succeeds");
    CHECK(epic_settings_load(0x80u, out, (uint8_t)sizeof(out)), "crc edge: 0xFF load succeeds");
    CHECK(memcmp(out, ones, sizeof(out)) == 0, "crc edge: 0xFF data round-trips");
}

/** @brief Verify load_or_default returns true when a valid blob exists. */
static void test_load_or_default_returns_true_when_valid(void)
{
    reset_env();

    const settings_blob_t in = { 6u, 0x9999u, 0x55u };
    const settings_blob_t def = { 1u, 0x0001u, 0x01u };
    settings_blob_t out = { 0u, 0u, 0u };

    CHECK(epic_settings_save(0x90u, &in, (uint8_t)sizeof(in)), "valid default: save succeeds");
    CHECK(epic_settings_load_or_default(0x90u, &out, (uint8_t)sizeof(out), &def),
          "valid default: returns true for existing blob");
    CHECK(memcmp(&out, &in, sizeof(in)) == 0, "valid default: existing blob preserved");
}

/** @brief Run all epic-settings tests and report pass/fail counts. */
int main(void)
{
    test_save_then_load_roundtrip();
    test_blank_region_fails_without_touching_output();
    test_corruption_detected_without_partial_overwrite();
    test_load_or_default_persists_default();
    test_two_regions_do_not_interfere();
    test_crc_edge_patterns();
    test_load_or_default_returns_true_when_valid();

    printf("test_settings: %d passed, %d failed\n", g_pass, g_fail);
    return epic_harness_report(g_fail == 0);
}
