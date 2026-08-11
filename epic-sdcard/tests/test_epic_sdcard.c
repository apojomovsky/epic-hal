/** Host tests for the vendored mmc.c/crc.c against epic_sdcard_mock_spi.c:
 *  the actual shipped protocol logic, not epic_sdcard.c's HAL binding
 *  (real-target-only). */

#include "mmc.h"
#include "crc.h"
#include "epic_sdcard_mock_spi.h"

#include <stdio.h>
#include <string.h>

static int g_pass = 0, g_fail = 0;
#define CHECK(c, m) do { if (c) { g_pass++; } else { printf("FAIL: %s\n", m); g_fail++; } } while (0)

/** @brief Build a fresh, uninitialized mmc_card. */
static struct mmc_card new_card(void)
{
    struct mmc_card card = {0};
    card.max_speed_hz = 20000000UL;
    card.spi_instance = 0u;
    return card;
}

/** @brief Verify the mock card completes the init sequence and reports the expected size. */
static void test_init_sequence(void)
{
    epic_sdcard_mock_reset();
    struct mmc_card card = new_card();
    mmc_init(&card, 1u);

    CHECK(!mmc_is_initialized(&card), "before init_card: not initialized");

    int8_t res = mmc_init_card(&card);
    CHECK(res == 0, "init_card: succeeds against the mock");
    CHECK(mmc_is_initialized(&card), "after init_card: initialized");
    CHECK(mmc_get_num_blocks(&card) == EPIC_SDCARD_MOCK_REPORTED_BLOCKS,
          "init_card: reports the mock's advertised block count");
    CHECK(mmc_ready(&card), "ready() true immediately after init");
}

/** @brief Verify a pre-seeded block reads back byte-for-byte. */
static void test_read_preseeded_block(void)
{
    epic_sdcard_mock_reset();
    uint8_t *backing = epic_sdcard_mock_block(1);
    for (uint16_t i = 0; i < EPIC_SDCARD_MOCK_BLOCK_SIZE; i++) {
        backing[i] = (uint8_t)(i * 3 + 7);
    }

    struct mmc_card card = new_card();
    mmc_init(&card, 1u);
    CHECK(mmc_init_card(&card) == 0, "read test: init_card succeeds");

    uint8_t data[EPIC_SDCARD_MOCK_BLOCK_SIZE];
    memset(data, 0, sizeof(data));
    int8_t res = mmc_read_block(&card, 1u, data);
    CHECK(res == 0, "read_block: succeeds (CRC16 self-check passes)");
    CHECK(memcmp(data, epic_sdcard_mock_block(1), EPIC_SDCARD_MOCK_BLOCK_SIZE) == 0,
          "read_block: bytes match what was pre-seeded");
}

/** @brief Verify a write followed by a read round-trips the block. */
static void test_write_then_read_round_trip(void)
{
    epic_sdcard_mock_reset();
    struct mmc_card card = new_card();
    mmc_init(&card, 1u);
    CHECK(mmc_init_card(&card) == 0, "round-trip: init_card succeeds");

    uint8_t written[EPIC_SDCARD_MOCK_BLOCK_SIZE];
    for (uint16_t i = 0; i < EPIC_SDCARD_MOCK_BLOCK_SIZE; i++) {
        written[i] = (uint8_t)(255 - i);
    }

    int8_t wres = mmc_write_block(&card, 2u, written);
    CHECK(wres == 0, "write_block: succeeds");
    CHECK(memcmp(epic_sdcard_mock_block(2), written, EPIC_SDCARD_MOCK_BLOCK_SIZE) == 0,
          "write_block: mock's backing store actually got the bytes");

    uint8_t read_back[EPIC_SDCARD_MOCK_BLOCK_SIZE];
    memset(read_back, 0, sizeof(read_back));
    int8_t rres = mmc_read_block(&card, 2u, read_back);
    CHECK(rres == 0, "round-trip: read_block succeeds");
    CHECK(memcmp(read_back, written, EPIC_SDCARD_MOCK_BLOCK_SIZE) == 0,
          "round-trip: read-back bytes match what was written");
}

/** @brief Verify writes to one block do not affect another. */
static void test_two_blocks_independent(void)
{
    epic_sdcard_mock_reset();
    struct mmc_card card = new_card();
    mmc_init(&card, 1u);
    CHECK(mmc_init_card(&card) == 0, "independence test: init_card succeeds");

    uint8_t a[EPIC_SDCARD_MOCK_BLOCK_SIZE], b[EPIC_SDCARD_MOCK_BLOCK_SIZE];
    memset(a, 0xAA, sizeof(a));
    memset(b, 0x55, sizeof(b));
    CHECK(mmc_write_block(&card, 0u, a) == 0, "independence: write block 0");
    CHECK(mmc_write_block(&card, 3u, b) == 0, "independence: write block 3");

    uint8_t read0[EPIC_SDCARD_MOCK_BLOCK_SIZE], read3[EPIC_SDCARD_MOCK_BLOCK_SIZE];
    CHECK(mmc_read_block(&card, 0u, read0) == 0, "independence: read block 0");
    CHECK(mmc_read_block(&card, 3u, read3) == 0, "independence: read block 3");
    CHECK(memcmp(read0, a, sizeof(a)) == 0, "independence: block 0 unaffected by block 3's write");
    CHECK(memcmp(read3, b, sizeof(b)) == 0, "independence: block 3 has its own data");
}

/** @brief Verify reads beyond the reported size fail. */
static void test_read_beyond_reported_size_fails(void)
{
    epic_sdcard_mock_reset();
    struct mmc_card card = new_card();
    mmc_init(&card, 1u);
    CHECK(mmc_init_card(&card) == 0, "range test: init_card succeeds");

    uint8_t data[EPIC_SDCARD_MOCK_BLOCK_SIZE];
    /* mmc_read_block range-checks against card_size_blocks (the
     * *reported* 1024, not the mock's small backing store) before
     * touching SPI at all, mmc.c's own bounds check. */
    int8_t res = mmc_read_block(&card, EPIC_SDCARD_MOCK_REPORTED_BLOCKS, data);
    CHECK(res < 0, "read_block: rejects a block address at/past the reported size");
}

/** @brief Verify reads before init fail. */
static void test_read_before_init_fails(void)
{
    epic_sdcard_mock_reset();
    struct mmc_card card = new_card();
    mmc_init(&card, 1u);
    /* Never call mmc_init_card(): card_size_blocks is still 0 from
     * reset_state(), so any block address is "beyond the reported size". */

    uint8_t data[EPIC_SDCARD_MOCK_BLOCK_SIZE];
    CHECK(mmc_read_block(&card, 0u, data) < 0, "read_block before init: fails");
    CHECK(!mmc_ready(&card), "ready() false before init");
}

/** @brief Verify the CRC16 self-check property on the card data. */
static void test_crc16_self_check_property(void)
{
    /* The same identity mmc.c's __read_data_block relies on: CRC16 over
     * (data, then its own CRC16 in [high,low]/MSB-first order) is 0.
     * [low,high] (the order mmc_write_block sends, unrelated and never
     * self-checked) does not self-check to 0; only MSB-first does. */
    uint8_t data[16];
    for (uint16_t i = 0; i < sizeof(data); i++) {
        data[i] = (uint8_t)(i * 17 + 3);
    }
    uint16_t ck = add_crc16_array(0, data, sizeof(data));
    uint8_t trailer[2] = {(uint8_t)((ck >> 8) & 0xffu), (uint8_t)(ck & 0xffu)};
    uint16_t check = add_crc16_array(ck, trailer, sizeof(trailer));
    CHECK(check == 0, "crc16: data + its own [high,low] CRC bytes self-checks to 0");
}

/** @brief Verify array CRC16 matches byte-by-byte accumulation. */
static void test_crc16_array_matches_byte_by_byte(void)
{
    uint8_t data[8] = {1, 2, 3, 4, 5, 6, 7, 8};
    uint16_t via_array = add_crc16_array(0, data, sizeof(data));

    uint16_t via_bytes = 0;
    for (uint16_t i = 0; i < sizeof(data); i++) {
        via_bytes = add_crc16(via_bytes, data[i]);
    }
    CHECK(via_array == via_bytes, "crc16: add_crc16_array matches repeated add_crc16 calls");
}

/** @brief Verify CRC7 is nonzero and deterministic. */
static void test_crc7_nonzero_and_deterministic(void)
{
    uint8_t csum = 0;
    uint8_t frame[5] = {0x40, 0x00, 0x00, 0x00, 0x00}; /* CMD0 */
    for (uint8_t i = 0; i < sizeof(frame); i++) {
        csum = add_crc7(csum, frame[i]);
    }
    /* mmc.c's __send_mmc_command computes exactly this and ORs in the
     * stop bit: (crc7 << 1) | 0x1. CMD0's spec-defined CRC7 byte is
     * 0x95; if this ever drifts, add_crc7 itself changed. */
    uint8_t on_wire = (uint8_t)((csum << 1) | 0x1u);
    CHECK(on_wire == 0x95u, "crc7: CMD0's frame produces the spec-known 0x95 CRC byte");
}

/** @brief Run all epic-sdcard tests and report pass/fail counts. */
int main(void)
{
    test_init_sequence();
    test_read_preseeded_block();
    test_write_then_read_round_trip();
    test_two_blocks_independent();
    test_read_beyond_reported_size_fails();
    test_read_before_init_fails();
    test_crc16_self_check_property();
    test_crc16_array_matches_byte_by_byte();
    test_crc7_nonzero_and_deterministic();

    printf("test_epic_sdcard: %d passed, %d failed\n", g_pass, g_fail);
    return (g_fail == 0) ? 0 : 1;
}
