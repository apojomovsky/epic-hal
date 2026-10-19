/*
 * Host test for lib/bus: the I2C and SPI MEM transactions driven through
 * mock ops injected with epic_bus_set_i2c_ops/_set_spi_ops. Asserts the
 * exact op sequence, the return code of each NACK phase, and that the SPI
 * exchanges happen only while CS is asserted. The default HAL ops are not
 * exercised (the host sim has no SSP slave model).
 */

#include "epic_bus.h"
#include "core/epic_harness.h"

#include <stdint.h>
#include <stdio.h>
#include <string.h>

enum {
    EV_START = 1, EV_RSTART, EV_STOP, EV_WRITE, EV_READ,
    EV_SEL, EV_DESEL, EV_XCHG
};

typedef struct {
    uint8_t op;
    uint8_t val;   /* byte written/exchanged, or the ack flag of a read */
} event_t;

#define MAX_EV 32
#define NO_NACK (-1)

static event_t g_ev[MAX_EV];
static int     g_nev;
static int     g_write_calls;
static int     g_nack_at;       /* 0-based write_byte call that NACKs */
static uint8_t g_rd_next;       /* next byte the I2C mock returns */
static int     g_cs_low;
static int     g_cs_violations; /* exchanges seen while CS was high */
static uint8_t g_xchg_next;     /* MISO byte base for SPI exchanges */

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

/**
 * @brief Append one event to the log.
 */
static void ev(uint8_t op, uint8_t val)
{
    if (g_nev < MAX_EV)
    {
        g_ev[g_nev].op = op;
        g_ev[g_nev].val = val;
    }
    g_nev++;
}

/** @brief Mock I2C START. */
static void m_start(void)
{
    ev(EV_START, 0u);
}
/** @brief Mock I2C REPEATED-START. */
static void m_rstart(void)
{
    ev(EV_RSTART, 0u);
}
/** @brief Mock I2C STOP. */
static void m_stop(void)
{
    ev(EV_STOP, 0u);
}
/** @brief Mock I2C write: NACK the call numbered g_nack_at. */
static int m_write(uint8_t b)
{
    ev(EV_WRITE, b);
    return (g_write_calls++ == g_nack_at) ? 0 : 1;
}
/** @brief Mock I2C read: log the ack flag, return an incrementing byte. */
static uint8_t m_read(int ack)
{
    ev(EV_READ, (uint8_t)ack);
    return g_rd_next++;
}
static const epic_bus_i2c_ops_t mock_i2c = { m_start, m_rstart, m_stop, m_write, m_read };

/** @brief Mock SPI select. */
static void s_select(void)
{
    ev(EV_SEL, 0u);
    g_cs_low = 1;
}
/** @brief Mock SPI deselect. */
static void s_deselect(void)
{
    ev(EV_DESEL, 0u);
    g_cs_low = 0;
}
/** @brief Mock SPI exchange: flag use with CS high, return an incrementing MISO. */
static uint8_t s_xchg(uint8_t b)
{
    ev(EV_XCHG, b);
    if (!g_cs_low)
    {
        g_cs_violations++;
    }
    return g_xchg_next++;
}
static const epic_bus_spi_ops_t mock_spi = { s_select, s_deselect, s_xchg };

/**
 * @brief Reset all mock state and clear the event log.
 */
static void reset(void)
{
    memset(g_ev, 0, sizeof g_ev);
    g_nev = 0;
    g_write_calls = 0;
    g_nack_at = NO_NACK;
    g_rd_next = 0x10u;
    g_cs_low = 0;
    g_cs_violations = 0;
    g_xchg_next = 0xC0u;
}

/**
 * @brief Compare the event log against an expected sequence.
 *
 * @param exp expected events
 * @param n   number of expected events
 * @return 1 if the log matches exactly, else 0
 */
static int seq_is(const event_t *exp, int n)
{
    if (g_nev != n)
    {
        return 0;
    }
    for (int i = 0; i < n; i++)
    {
        if (g_ev[i].op != exp[i].op || g_ev[i].val != exp[i].val)
        {
            return 0;
        }
    }
    return 1;
}

#define SEQ(name, ...) \
    static const event_t name[] = { __VA_ARGS__ }
#define SEQ_IS(name) seq_is(name, (int)(sizeof name / sizeof name[0]))

#define DEV 0x50u
#define W_ADDR ((uint8_t)(DEV << 1))
#define R_ADDR ((uint8_t)((DEV << 1) | 1u))

/**
 * @brief I2C read: full sequence, ACK all but the last byte, data returned.
 */
static void test_i2c_read(void)
{
    SEQ(exp4,
        {EV_START, 0}, {EV_WRITE, W_ADDR}, {EV_WRITE, 0x07},
        {EV_RSTART, 0}, {EV_WRITE, R_ADDR},
        {EV_READ, 1}, {EV_READ, 1}, {EV_READ, 1}, {EV_READ, 0},
        {EV_STOP, 0});
    SEQ(exp1,
        {EV_START, 0}, {EV_WRITE, W_ADDR}, {EV_WRITE, 0x07},
        {EV_RSTART, 0}, {EV_WRITE, R_ADDR},
        {EV_READ, 0},
        {EV_STOP, 0});
    uint8_t buf[4] = {0};

    reset();
    CHECK(epic_bus_i2c_mem_read(DEV, 0x07u, buf, 4) == 4, "i2c read n=4 returns 4");
    CHECK(SEQ_IS(exp4), "i2c read n=4 op sequence");
    CHECK(buf[0] == 0x10u && buf[1] == 0x11u && buf[2] == 0x12u && buf[3] == 0x13u,
          "i2c read n=4 data in order");

    reset();
    CHECK(epic_bus_i2c_mem_read(DEV, 0x07u, buf, 1) == 1, "i2c read n=1 returns 1");
    CHECK(SEQ_IS(exp1), "i2c read n=1 NACKs its only byte");
    CHECK(buf[0] == 0x10u, "i2c read n=1 data");
}

/**
 * @brief I2C read: NACK in each of the three write phases returns -1 and STOPs.
 */
static void test_i2c_read_nack(void)
{
    SEQ(addr_w,
        {EV_START, 0}, {EV_WRITE, W_ADDR}, {EV_STOP, 0});
    SEQ(reg,
        {EV_START, 0}, {EV_WRITE, W_ADDR}, {EV_WRITE, 0x07}, {EV_STOP, 0});
    SEQ(addr_r,
        {EV_START, 0}, {EV_WRITE, W_ADDR}, {EV_WRITE, 0x07},
        {EV_RSTART, 0}, {EV_WRITE, R_ADDR}, {EV_STOP, 0});
    uint8_t buf[2] = {0xEEu, 0xEEu};

    reset();
    g_nack_at = 0;
    CHECK(epic_bus_i2c_mem_read(DEV, 0x07u, buf, 2) == -1, "i2c read addr-W NACK returns -1");
    CHECK(SEQ_IS(addr_w), "i2c read addr-W NACK stops immediately");

    reset();
    g_nack_at = 1;
    CHECK(epic_bus_i2c_mem_read(DEV, 0x07u, buf, 2) == -1, "i2c read reg NACK returns -1");
    CHECK(SEQ_IS(reg), "i2c read reg NACK stops before RESTART");

    reset();
    g_nack_at = 2;
    CHECK(epic_bus_i2c_mem_read(DEV, 0x07u, buf, 2) == -1, "i2c read addr-R NACK returns -1");
    CHECK(SEQ_IS(addr_r), "i2c read addr-R NACK stops before any data read");
    CHECK(buf[0] == 0xEEu && buf[1] == 0xEEu, "i2c read NACK leaves buffer untouched");
}

/**
 * @brief I2C write: sequence, and NACK at address, register and a data byte.
 */
static void test_i2c_write(void)
{
    static const uint8_t d[3] = { 0xA0u, 0xA1u, 0xA2u };
    SEQ(ok,
        {EV_START, 0}, {EV_WRITE, W_ADDR}, {EV_WRITE, 0x05},
        {EV_WRITE, 0xA0}, {EV_WRITE, 0xA1}, {EV_WRITE, 0xA2},
        {EV_STOP, 0});
    SEQ(addr,
        {EV_START, 0}, {EV_WRITE, W_ADDR}, {EV_STOP, 0});
    SEQ(reg,
        {EV_START, 0}, {EV_WRITE, W_ADDR}, {EV_WRITE, 0x05}, {EV_STOP, 0});
    SEQ(data1,
        {EV_START, 0}, {EV_WRITE, W_ADDR}, {EV_WRITE, 0x05},
        {EV_WRITE, 0xA0}, {EV_WRITE, 0xA1}, {EV_STOP, 0});
    SEQ(none,
        {EV_START, 0}, {EV_WRITE, W_ADDR}, {EV_WRITE, 0x05}, {EV_STOP, 0});

    reset();
    CHECK(epic_bus_i2c_mem_write(DEV, 0x05u, d, 3) == 3, "i2c write returns n");
    CHECK(SEQ_IS(ok), "i2c write op sequence");

    reset();
    g_nack_at = 0;
    CHECK(epic_bus_i2c_mem_write(DEV, 0x05u, d, 3) == -1, "i2c write addr NACK returns -1");
    CHECK(SEQ_IS(addr), "i2c write addr NACK stops immediately");

    reset();
    g_nack_at = 1;
    CHECK(epic_bus_i2c_mem_write(DEV, 0x05u, d, 3) == -1, "i2c write reg NACK returns -1");
    CHECK(SEQ_IS(reg), "i2c write reg NACK sends no data");

    reset();
    g_nack_at = 3;   /* second data byte */
    CHECK(epic_bus_i2c_mem_write(DEV, 0x05u, d, 3) == -1, "i2c write data NACK returns -1");
    CHECK(SEQ_IS(data1), "i2c write data NACK stops at the failing byte");

    reset();
    CHECK(epic_bus_i2c_mem_write(DEV, 0x05u, d, 0) == 0, "i2c write n=0 returns 0");
    CHECK(SEQ_IS(none), "i2c write n=0 still sends address and register");
}

/**
 * @brief I2C ops injection: NULL restores the default, set/get round-trip.
 */
static void test_i2c_ops_install(void)
{
    epic_bus_set_i2c_ops(&mock_i2c);
    CHECK(epic_bus_get_i2c_ops() == &mock_i2c, "i2c ops get returns injected table");
    epic_bus_set_i2c_ops(NULL);
    CHECK(epic_bus_get_i2c_ops() != &mock_i2c && epic_bus_get_i2c_ops() != NULL,
          "i2c ops NULL restores a non-mock default");
    epic_bus_set_i2c_ops(&mock_i2c);
}

/**
 * @brief SPI read: CS brackets reg byte plus n dummy-byte exchanges, MISO captured.
 */
static void test_spi_read(void)
{
    SEQ(exp3,
        {EV_SEL, 0}, {EV_XCHG, 0x8A},
        {EV_XCHG, 0}, {EV_XCHG, 0}, {EV_XCHG, 0},
        {EV_DESEL, 0});
    SEQ(exp0,
        {EV_SEL, 0}, {EV_XCHG, 0x8A}, {EV_DESEL, 0});
    uint8_t buf[3] = {0};

    reset();
    CHECK(epic_bus_spi_mem_read(0x8Au, buf, 3) == 3, "spi read returns n");
    CHECK(SEQ_IS(exp3), "spi read op sequence");
    /* the register-phase MISO byte (0xC0) is discarded */
    CHECK(buf[0] == 0xC1u && buf[1] == 0xC2u && buf[2] == 0xC3u,
          "spi read captures MISO after the register byte");
    CHECK(g_cs_violations == 0 && !g_cs_low, "spi read exchanges only under CS, CS released");

    reset();
    CHECK(epic_bus_spi_mem_read(0x8Au, buf, 0) == 0, "spi read n=0 returns 0");
    CHECK(SEQ_IS(exp0), "spi read n=0 still frames the register byte in CS");
}

/**
 * @brief SPI write: CS brackets reg byte plus the data bytes.
 */
static void test_spi_write(void)
{
    static const uint8_t d[3] = { 0x01u, 0x02u, 0x03u };
    SEQ(exp,
        {EV_SEL, 0}, {EV_XCHG, 0x0A},
        {EV_XCHG, 0x01}, {EV_XCHG, 0x02}, {EV_XCHG, 0x03},
        {EV_DESEL, 0});

    reset();
    CHECK(epic_bus_spi_mem_write(0x0Au, d, 3) == 3, "spi write returns n");
    CHECK(SEQ_IS(exp), "spi write op sequence");
    CHECK(g_cs_violations == 0 && !g_cs_low, "spi write exchanges only under CS, CS released");
}

/**
 * @brief SPI ops injection: set/get round-trip, NULL restores the default.
 */
static void test_spi_ops_install(void)
{
    epic_bus_set_spi_ops(&mock_spi);
    CHECK(epic_bus_get_spi_ops() == &mock_spi, "spi ops get returns injected table");
    epic_bus_set_spi_ops(NULL);
    CHECK(epic_bus_get_spi_ops() != &mock_spi && epic_bus_get_spi_ops() != NULL,
          "spi ops NULL restores a non-mock default");
    epic_bus_set_spi_ops(&mock_spi);
}

/**
 * @brief Test entry: run every MEM transaction case and report.
 */
int main(void)
{
    epic_harness_init(1000UL);
    epic_bus_set_i2c_ops(&mock_i2c);
    epic_bus_set_spi_ops(&mock_spi);

    test_i2c_read();
    test_i2c_read_nack();
    test_i2c_write();
    test_i2c_ops_install();
    test_spi_read();
    test_spi_write();
    test_spi_ops_install();

    printf("test_bus_mem: %d passed, %d failed\n", g_pass, g_fail);
    return epic_harness_report(g_fail == 0);
}
