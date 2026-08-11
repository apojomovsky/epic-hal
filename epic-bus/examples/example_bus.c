/* epic-bus host test: verifies the I2C/SPI MEM transaction logic
 * against a mock MEM device injected through the ops seam (the host
 * sim has no SSP slave model; the default HAL ops are validated on
 * real silicon instead). */

#include "epic_bus.h"
#include "core/epic_harness.h"

static int g_fails = 0;
#define CHECK(c, m) do { if (!(c)) { epic_harness_log("FAIL: %s\n", m); g_fails++; } } while (0)

/* mock I2C MEM device (register map + transaction state machine) */
#define MOCK_DEV 0x50
static uint8_t g_reg[16];
static enum { I_IDLE, I_ADDR, I_REG, I_DATA, I_READ } g_i2c_state;
static uint8_t g_i2c_reg;

static void mock_i2c_start(void)          { g_i2c_state = I_ADDR; }
static void mock_i2c_repeated_start(void) { g_i2c_state = I_ADDR; }
static void mock_i2c_stop(void)           { g_i2c_state = I_IDLE; }
static int  mock_i2c_write_byte(uint8_t b)
{
    if (g_i2c_state == I_ADDR) {
        if ((b >> 1) != MOCK_DEV) return 0;          /* wrong device: NACK */
        if (b & 1u) { g_i2c_state = I_READ; }        /* addr+R */
        else        { g_i2c_state = I_REG; }         /* addr+W */
        return 1;
    }
    if (g_i2c_state == I_REG)  { g_i2c_reg = b; g_i2c_state = I_DATA; return 1; }
    if (g_i2c_state == I_DATA) { g_reg[g_i2c_reg & 0x0F] = b; g_i2c_reg++; return 1; }
    return 0;
}
static uint8_t mock_i2c_read_byte(int ack)
{
    (void)ack;
    uint8_t b = g_reg[g_i2c_reg & 0x0F];
    g_i2c_reg++;
    return b;
}
static const epic_bus_i2c_ops_t mock_i2c = {
    mock_i2c_start, mock_i2c_repeated_start, mock_i2c_stop,
    mock_i2c_write_byte, mock_i2c_read_byte
};

/* mock SPI MEM device */
static enum { S_IDLE, S_REG, S_XFER } g_spi_state;
static uint8_t g_spi_reg;
static void mock_spi_select(void)   { g_spi_state = S_REG; }
static void mock_spi_deselect(void) { g_spi_state = S_IDLE; }
static uint8_t mock_spi_exchange(uint8_t b)
{
    if (g_spi_state == S_REG) { g_spi_reg = b; g_spi_state = S_XFER; return 0xFFu; }
    uint8_t out = g_reg[g_spi_reg & 0x0F];
    g_reg[g_spi_reg & 0x0F] = b;   /* write what's shifted in (MOSI) to the map */
    g_spi_reg++;
    return out;
}
static const epic_bus_spi_ops_t mock_spi = {
    mock_spi_select, mock_spi_deselect, mock_spi_exchange
};

int main(void)
{
    epic_harness_init(0UL);
    for (int i = 0; i < 16; i++) { g_reg[i] = (uint8_t)(0x10 + i); }  /* known pattern */

    /* I2C */
    epic_bus_set_i2c_ops(&mock_i2c);

    uint8_t buf[8] = {0};
    int n = epic_bus_i2c_mem_read(MOCK_DEV, 0x00, buf, 4);
    CHECK(n == 4, "i2c mem_read returns 4");
    CHECK(buf[0] == 0x10 && buf[1] == 0x11 && buf[2] == 0x12 && buf[3] == 0x13,
          "i2c mem_read bytes");

    uint8_t wr[3] = { 0xA0, 0xA1, 0xA2 };
    n = epic_bus_i2c_mem_write(MOCK_DEV, 0x05, wr, 3);
    CHECK(n == 3, "i2c mem_write returns 3");
    CHECK(g_reg[5] == 0xA0 && g_reg[6] == 0xA1 && g_reg[7] == 0xA2, "i2c mem_write stored");

    n = epic_bus_i2c_mem_read(MOCK_DEV, 0x05, buf, 3);
    CHECK(n == 3 && buf[0] == 0xA0 && buf[1] == 0xA1 && buf[2] == 0xA2, "i2c read-back");

    n = epic_bus_i2c_mem_read(0x77, 0x00, buf, 1);   /* wrong device */
    CHECK(n == -1, "i2c wrong-device NACK -> -1");

    /* SPI */
    epic_bus_set_spi_ops(&mock_spi);
    for (int i = 0; i < 16; i++) { g_reg[i] = (uint8_t)(0x80 + i); }

    n = epic_bus_spi_mem_read(0x02, buf, 3);
    CHECK(n == 3, "spi mem_read returns 3");
    CHECK(buf[0] == 0x82 && buf[1] == 0x83 && buf[2] == 0x84, "spi mem_read bytes");

    uint8_t sw[2] = { 0xC0, 0xC1 };
    n = epic_bus_spi_mem_write(0x08, sw, 2);
    CHECK(n == 2, "spi mem_write returns 2");
    CHECK(g_reg[8] == 0xC0 && g_reg[9] == 0xC1, "spi mem_write stored");
    n = epic_bus_spi_mem_read(0x08, buf, 2);
    CHECK(n == 2 && buf[0] == 0xC0 && buf[1] == 0xC1, "spi read-back");

    epic_harness_log("bus: fails=%d\n", g_fails);
    return epic_harness_report(g_fails == 0);
}
