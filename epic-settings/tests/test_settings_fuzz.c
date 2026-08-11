/** Host property tests for epic-settings over the simulated EEPROM backend:
 *  randomized save/load round trips, a per-byte corruption sweep proving
 *  that flipping ANY stored byte (payload or CRC) is detected with the
 *  output buffer untouched, defaults on blank regions, region isolation,
 *  and the top-of-EEPROM boundary. Deterministic (fixed-seed LCG). */

#include "epic_settings.h"
#include "core/epic_harness.h"

#if defined(PIC18F2455) || defined(PIC18F2550) || defined(PIC18F4455) || defined(PIC18F4550)
  #include "pic18fxx5x_sim.h"
  #define SIM_EEPROM_BYTE(addr, data)  pic18_sim_drive_eeprom_byte((addr), (data))
  #define SIM_EEPROM_READ(addr)        pic18_sim_eeprom_read((addr))
#else
  #include "pic16f87xa_sim.h"
  #define SIM_EEPROM_BYTE(addr, data)  pic16f87xa_sim_drive_eeprom_byte((addr), (data))
  /** @brief Read one EEPROM byte from the PIC16F87XA simulator. */
  extern uint8_t pic16f87xa_sim_eeprom_read(uint8_t addr);
  #define SIM_EEPROM_READ(addr)        pic16f87xa_sim_eeprom_read((addr))
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

static uint32_t g_seed = 0xE57E0001u;
/** @brief Next value of the deterministic LCG. */
static uint32_t rnd(void)
{
    g_seed = (1664525u * g_seed + 1013904223u);
    return g_seed;
}

#define MAX_BLOB 40u
#define MAX_ADDR 191u   /* keeps addr+size+2 below the default region */

static uint8_t g_blob[MAX_BLOB];

/** @brief Re-initialize the harness before each test. */
static void reset_env(void)
{
    epic_harness_init(1000000UL);
}

/** @brief Fill a buffer with random bytes from the LCG. */
static void fill_random(uint8_t *buf, uint8_t len)
{
    for (uint8_t i = 0; i < len; i++) {
        buf[i] = (uint8_t)rnd();
    }
}

/**
 * @brief Save/load round trip plus the per-byte corruption sweep.
 *
 * For every stored byte position (payload and both CRC bytes), flip one
 * bit and verify load fails with the caller buffer untouched, then
 * restore.
 */
static void roundtrip_and_corrupt(uint8_t addr, uint8_t size)
{
    uint8_t out[MAX_BLOB + 2u];
    memset(out, 0x5A, sizeof(out));

    fill_random(g_blob, size);
    CHECK(epic_settings_save(addr, g_blob, size), "save succeeds");
    CHECK(epic_settings_load(addr, out, size), "load succeeds");
    CHECK(memcmp(out, g_blob, size) == 0, "round trip byte-exact");

    /* Corruption sweep over payload + the two trailing CRC bytes. */
    for (int pos = 0; pos < (int)size + 2; pos++) {
        uint8_t stored = SIM_EEPROM_READ((uint8_t)(addr + (uint8_t)pos));
        uint8_t corrupt = (uint8_t)(stored ^ 0x80u);
        SIM_EEPROM_BYTE((uint8_t)(addr + (uint8_t)pos), corrupt);

        memset(out, 0x5A, sizeof(out));
        CHECK(!epic_settings_load(addr, out, size), "corrupt byte detected");
        for (uint8_t i = 0; i < size; i++) {
            if (out[i] != 0x5Au) {
                CHECK(0, "output untouched on corrupt load");
                break;
            }
        }

        SIM_EEPROM_BYTE((uint8_t)(addr + (uint8_t)pos), stored);
    }

    /* After the sweep the stored blob is intact again. */
    memset(out, 0x5A, sizeof(out));
    CHECK(epic_settings_load(addr, out, size), "load succeeds after sweep");
    CHECK(memcmp(out, g_blob, size) == 0, "blob intact after sweep");
}

/** @brief Verify many random save/load round trips all succeed. */
static void test_random_roundtrips(void)
{
    reset_env();
    for (int it = 0; it < 200; it++) {
        uint8_t addr = (uint8_t)(rnd() % MAX_ADDR);
        uint8_t size = (uint8_t)(1u + rnd() % MAX_BLOB);
        roundtrip_and_corrupt(addr, size);
    }
}

/** @brief Verify defaults are applied on blank regions. */
static void test_defaults_on_blank(void)
{
    reset_env();

    /* Dedicated high region never touched by the random scenarios. */
    const uint8_t addr = 240u;
    const uint8_t size = 8u;
    uint8_t def[MAX_BLOB];
    uint8_t out[MAX_BLOB];
    fill_random(def, size);

    memset(out, 0x00, sizeof(out));
    CHECK(!epic_settings_load_or_default(addr, out, size, def),
          "blank region reports invalid");
    CHECK(memcmp(out, def, size) == 0, "defaults copied into caller buffer");

    memset(out, 0x00, sizeof(out));
    CHECK(epic_settings_load(addr, out, size), "defaults persisted");
    CHECK(memcmp(out, def, size) == 0, "persisted defaults match");

    /* A valid region returns true and preserves the stored blob. */
    memset(out, 0x00, sizeof(out));
    CHECK(epic_settings_load_or_default(addr, out, size, def),
          "valid region returns true");
    CHECK(memcmp(out, def, size) == 0, "existing blob preserved");
}

/** @brief Verify a blob at the top of EEPROM round-trips. */
static void test_top_of_eeprom(void)
{
    reset_env();
    static const uint8_t sizes[] = { 1u, 5u, 16u, 40u };
    for (size_t i = 0; i < sizeof(sizes)/sizeof(sizes[0]); i++) {
        uint8_t size = sizes[i];
        uint8_t addr = (uint8_t)(256u - (uint16_t)size - 2u);
        roundtrip_and_corrupt(addr, size);
    }
}

/** @brief Verify separate regions remain isolated under fuzzing. */
static void test_region_isolation(void)
{
    reset_env();

    static const struct { uint8_t addr; uint8_t size; } regions[] = {
        { 10u, 4u }, { 100u, 20u }, { 200u, 30u },
    };
    uint8_t blobs[3][MAX_BLOB];
    uint8_t out[MAX_BLOB];
    for (size_t r = 0; r < 3; r++) {
        fill_random(blobs[r], regions[r].size);
        CHECK(epic_settings_save(regions[r].addr, blobs[r], regions[r].size),
              "region save succeeds");
    }
    /* Save/load interleaved, then verify every region independently. */
    for (size_t r = 0; r < 3; r++) {
        memset(out, 0x00, sizeof(out));
        CHECK(epic_settings_load(regions[r].addr, out, regions[r].size),
              "region load succeeds");
        CHECK(memcmp(out, blobs[r], regions[r].size) == 0,
              "region blob preserved");
    }
}

/** @brief Run all epic-settings fuzz tests and report pass/fail counts. */
int main(void)
{
    test_random_roundtrips();
    test_defaults_on_blank();
    test_top_of_eeprom();
    test_region_isolation();

    printf("test_settings_fuzz: %d passed, %d failed\n", g_pass, g_fail);
    return epic_harness_report(g_fail == 0);
}
