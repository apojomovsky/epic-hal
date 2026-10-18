/*
 * Host test for lib/modbus: RTU framing, CRC and dispatch through the
 * real slave core over the host-sim USART model, no MPLAB SIM needed.
 * Requests enter via RX injection, T3.5 advances via the host tick, and
 * responses are captured from the TX ring's TXREG readback.
 */

#include "epic_modbus.h"
#include "epic_serial.h"
#include "epic_tick.h"
#include "core/epic_harness.h"
#include "epic_hal.h"

#if defined(PIC18F2455) || defined(PIC18F2550) || defined(PIC18F4455) || defined(PIC18F4550)
  #include "pic18fxx5x_sim.h"
  #define SIM_RX(b)  pic18_sim_drive_usart_rx((uint8_t)(b))
  #define TEST_FOSC_HZ 48000000UL
#elif defined(PIC16F882) || defined(PIC16F883) || defined(PIC16F884) || \
      defined(PIC16F886) || defined(PIC16F887)
  #include "pic16f88x_sim.h"
  #define SIM_RX(b)  pic16f88x_sim_drive_usart_rx((uint8_t)(b))
  #define TEST_FOSC_HZ 20000000UL
#else
  #include "pic16f87xa_sim.h"
  #define SIM_RX(b)  pic16f87xa_sim_drive_usart_rx((uint8_t)(b))
  #define TEST_FOSC_HZ 20000000UL
#endif

#include <stdio.h>
#include <string.h>

#define SLAVE_ADDR 0x11u
#define BAUD       9600u

/* T3.5 at 9600 baud is 5 ms; the settle delay clears it with margin. */
#define SETTLE_MS 10u

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

static uint16_t s_holding[4];
static uint8_t s_coils[1];

static const epic_modbus_slave_map_t s_map = {
    .coils               = s_coils,
    .num_coils           = 8,
    .discrete_inputs     = NULL,
    .num_discrete_inputs = 0,
    .holding_regs        = s_holding,
    .num_holding_regs    = 4,
    .input_regs          = NULL,
    .num_input_regs      = 0,
};

/**
 * @brief Independent CRC-16/MODBUS reference.
 */
static uint16_t ref_crc16(const uint8_t *buf, int len)
{
    uint16_t crc = 0xFFFFu;
    for (int i = 0; i < len; i++)
    {
        crc = (uint16_t)(crc ^ buf[i]);
        for (int b = 0; b < 8; b++)
        {
            crc = (crc & 1u) ? (uint16_t)((crc >> 1) ^ 0xA001u)
                             : (uint16_t)(crc >> 1);
        }
    }
    return crc;
}

/**
 * @brief Reset harness, tick, registers and slave for one case.
 */
static void reset_env(void)
{
    epic_harness_init(1000000UL);
    epic_tick_init(TEST_FOSC_HZ);
    s_holding[0] = 0x1234u;
    s_holding[1] = 0x5678u;
    s_holding[2] = 0x0000u;
    s_holding[3] = 0xFFFFu;
    s_coils[0] = 0u;
    epic_modbus_slave_init(TEST_FOSC_HZ, BAUD, SLAVE_ADDR, &s_map);
}

/**
 * @brief Append the reference CRC to a frame prefix.
 */
static void append_crc(uint8_t *frame, int prefix_len)
{
    uint16_t crc = ref_crc16(frame, prefix_len);
    frame[prefix_len] = (uint8_t)(crc & 0xFFu);
    frame[prefix_len + 1] = (uint8_t)(crc >> 8);
}

/**
 * @brief Inject a frame, settle past T3.5, and dispatch it.
 */
static void run_frame(const uint8_t *frame, int len)
{
    for (int i = 0; i < len; i++)
    {
        SIM_RX(frame[i]);
    }
    epic_modbus_slave_poll();
    epic_tick_delay_ms(SETTLE_MS);
    epic_modbus_slave_poll();
}

/**
 * @brief Drain the TX ring via TXREG readback.
 */
static int drain_tx(uint8_t *out, int max)
{
    int n = 0;
    while (epic_serial_tx_pending() > 0 && n < max)
    {
        int before = epic_serial_tx_pending();
        epic_harness_tick();
        int mid = epic_serial_tx_pending();
        if (mid < before)
        {
            out[n++] = (uint8_t)EPIC_REG8(PIC_REG_TXREG);
        }
        epic_dispatch_all_irqs();
        int after = epic_serial_tx_pending();
        if (after < mid)
        {
            out[n++] = (uint8_t)EPIC_REG8(PIC_REG_TXREG);
        }
    }
    epic_harness_tick();
    return n;
}

/**
 * @brief Check a captured response against an expected prefix plus CRC.
 */
static void check_response(const uint8_t *got, int n, const uint8_t *want, int want_len,
                           const char *tag)
{
    char msg[64];
    if (n != want_len + 2)
    {
        snprintf(msg, sizeof(msg), "%s: response length", tag);
        CHECK(0, msg);
        return;
    }
    if (memcmp(got, want, (size_t)want_len) != 0)
    {
        snprintf(msg, sizeof(msg), "%s: response bytes", tag);
        CHECK(0, msg);
        return;
    }
    {
        uint16_t c = ref_crc16(got, n - 2);
        if (got[n - 2] != (uint8_t)(c & 0xFFu) || got[n - 1] != (uint8_t)(c >> 8))
        {
            snprintf(msg, sizeof(msg), "%s: response CRC", tag);
            CHECK(0, msg);
            return;
        }
    }
    snprintf(msg, sizeof(msg), "%s", tag);
    CHECK(1, msg);
}

/**
 * @brief FC03 round trip against the byte-exact oracle frame.
 */
static void test_fc03_round_trip(void)
{
    static const uint8_t oracle[8] = {0x11u, 0x03u, 0x00u, 0x00u, 0x00u, 0x02u, 0xC6u, 0x9Bu};
    uint8_t req[8] = {SLAVE_ADDR, 0x03u, 0x00u, 0x00u, 0x00u, 0x02u, 0u, 0u};
    uint8_t resp[16];
    static const uint8_t want[7] = {0x11u, 0x03u, 0x04u, 0x12u, 0x34u, 0x56u, 0x78u};
    int n;

    reset_env();
    append_crc(req, 6);
    CHECK(memcmp(req, oracle, sizeof(req)) == 0, "fc03: request matches oracle");
    run_frame(req, (int)sizeof(req));
    n = drain_tx(resp, (int)sizeof(resp));
    CHECK(n == 9, "fc03: response length");
    if (n == 9)
    {
        CHECK(resp[0] == SLAVE_ADDR && resp[1] == 0x03u && resp[2] == 0x04u, "fc03: header");
        CHECK(resp[3] == 0x12u && resp[4] == 0x34u && resp[5] == 0x56u && resp[6] == 0x78u,
              "fc03: register bytes");
        check_response(resp, n, want, 7, "fc03: prefix and crc");
    }
}

/**
 * @brief Corrupted CRC is dropped with no response.
 */
static void test_crc_rejection(void)
{
    uint8_t req[8] = {SLAVE_ADDR, 0x03u, 0x00u, 0x00u, 0x00u, 0x02u, 0u, 0u};
    uint8_t resp[16];
    int n;

    reset_env();
    append_crc(req, 6);
    req[6] = (uint8_t)(req[6] ^ 0xFFu);
    run_frame(req, (int)sizeof(req));
    n = drain_tx(resp, (int)sizeof(resp));
    CHECK(n == 0, "crc: bad frame stays silent");
    CHECK(epic_serial_tx_pending() == 0, "crc: tx ring empty");
}

/**
 * @brief A frame for another slave is dropped with no response.
 */
static void test_address_mismatch(void)
{
    uint8_t req[8] = {0x12u, 0x03u, 0x00u, 0x00u, 0x00u, 0x02u, 0u, 0u};
    uint8_t resp[16];
    int n;

    reset_env();
    append_crc(req, 6);
    run_frame(req, (int)sizeof(req));
    n = drain_tx(resp, (int)sizeof(resp));
    CHECK(n == 0, "addr: other slave stays silent");
}

/**
 * @brief Broadcast writes apply but never get a response.
 */
static void test_broadcast_silence(void)
{
    uint8_t req[8] = {0x00u, 0x06u, 0x00u, 0x01u, 0xABu, 0xCDu, 0u, 0u};
    uint8_t resp[16];
    int n;

    reset_env();
    append_crc(req, 6);
    run_frame(req, (int)sizeof(req));
    n = drain_tx(resp, (int)sizeof(resp));
    CHECK(n == 0, "broadcast: no response");
    CHECK(s_holding[1] == 0xABCDu, "broadcast: write applied");
}

/**
 * @brief Unknown function code answers ILLEGAL_FUNCTION.
 */
static void test_exception_illegal_function(void)
{
    uint8_t req[8] = {SLAVE_ADDR, 0x07u, 0x00u, 0x00u, 0x00u, 0x02u, 0u, 0u};
    uint8_t resp[16];
    static const uint8_t want[3] = {0x11u, 0x87u, 0x01u};
    int n;

    reset_env();
    append_crc(req, 6);
    run_frame(req, (int)sizeof(req));
    n = drain_tx(resp, (int)sizeof(resp));
    check_response(resp, n, want, 3, "exc: illegal function");
}

/**
 * @brief Out-of-range read answers ILLEGAL_DATA_ADDRESS.
 */
static void test_exception_illegal_data_address(void)
{
    uint8_t req[8] = {SLAVE_ADDR, 0x03u, 0x00u, 0x04u, 0x00u, 0x01u, 0u, 0u};
    uint8_t resp[16];
    static const uint8_t want[3] = {0x11u, 0x83u, 0x02u};
    int n;

    reset_env();
    append_crc(req, 6);
    run_frame(req, (int)sizeof(req));
    n = drain_tx(resp, (int)sizeof(resp));
    check_response(resp, n, want, 3, "exc: illegal data address");
}

/**
 * @brief Zero-quantity read answers ILLEGAL_DATA_VALUE.
 */
static void test_exception_illegal_data_value(void)
{
    uint8_t req[8] = {SLAVE_ADDR, 0x03u, 0x00u, 0x00u, 0x00u, 0x00u, 0u, 0u};
    uint8_t resp[16];
    static const uint8_t want[3] = {0x11u, 0x83u, 0x03u};
    int n;

    reset_env();
    append_crc(req, 6);
    run_frame(req, (int)sizeof(req));
    n = drain_tx(resp, (int)sizeof(resp));
    check_response(resp, n, want, 3, "exc: illegal data value");
}

/**
 * @brief Run the modbus host cases and report pass/fail counts.
 */
int main(void)
{
    test_fc03_round_trip();
    test_crc_rejection();
    test_address_mismatch();
    test_broadcast_silence();
    test_exception_illegal_function();
    test_exception_illegal_data_address();
    test_exception_illegal_data_value();

    printf("test_modbus_host: %d passed, %d failed\n", g_pass, g_fail);
    return (g_fail == 0) ? 0 : 1;
}
