/**
 * Host test for epic-mcp23x17: the full module + epic-bus +
 * mock-device stack, through the epic-bus ops seam. A mock MCP23017
 * (a 22-byte register file with the I2C framing) and an SPI twin
 * (control-byte framing) are injected as the epic-bus i2c/spi ops;
 * a third case drives the module's own injectable transport directly.
 *
 * Register map under test (BANK=0 sequential, DS20001952E 3.5): IODIR
 * 0x00, IPOL 0x02, GPINTEN 0x04, DEFVAL 0x06, INTCON 0x08, IOCON 0x0A,
 * GPPU 0x0B, INTF 0x0D, INTCAP 0x0F, GPIO 0x11, OLAT 0x13 (each pair
 * is base + port).
 */

#include "epic_mcp23x17.h"

#include "epic_bus.h"

#include <stdint.h>
#include <stdio.h>
#include <string.h>

static int g_fail = 0;

#define CHECK(cond)                                                     \
    do {                                                                \
        if (!(cond)) {                                                  \
            printf("FAIL %s:%d: %s\n", __FILE__, __LINE__, #cond);      \
            g_fail = 1;                                                 \
        }                                                               \
    } while (0)

typedef struct {
    uint8_t regs[22];       /* BANK=0 sequential register file */
    uint8_t dev;            /* the 7-bit address the mock answers */
    uint8_t reg_ptr;
    int     phase;          /* P_ADDR .. P_READ below */
    int     nack_addr;      /* force an address NACK (error tests) */
} mock_i2c_t;

enum {
    MOCK_I2C_ADDR,    /* first address byte of a transaction */
    MOCK_I2C_REG,     /* the register byte */
    MOCK_I2C_DATA,    /* write data bytes */
    MOCK_I2C_RADDR,   /* the read-address byte after a repeated start */
    MOCK_I2C_READ,    /* read data bytes */
};

static mock_i2c_t g_i2c;

static void mock_i2c_start(void)        { g_i2c.phase = MOCK_I2C_ADDR; }
static void mock_i2c_repeated_start(void) { g_i2c.phase = MOCK_I2C_RADDR; }
static void mock_i2c_stop(void)         { }

static int mock_i2c_write_byte(uint8_t b)
{
    if (g_i2c.phase == MOCK_I2C_ADDR || g_i2c.phase == MOCK_I2C_RADDR) {
        if (g_i2c.nack_addr || ((b >> 1) != g_i2c.dev)) {
            return 0;   /* NACK */
        }
        g_i2c.phase = (g_i2c.phase == MOCK_I2C_ADDR) ? MOCK_I2C_REG
                                                     : MOCK_I2C_READ;
        return 1;
    }
    if (g_i2c.phase == MOCK_I2C_REG) {
        g_i2c.reg_ptr = b & 0x1Fu;
        g_i2c.phase = MOCK_I2C_DATA;
        return 1;
    }
    /* data byte into the register file, pointer increments (SEQOP=0);
     * a GPIO write mirrors into the output latch (DS20001952E 3.5.10). */
    if (g_i2c.reg_ptr < sizeof(g_i2c.regs)) {
        g_i2c.regs[g_i2c.reg_ptr] = b;
        if (g_i2c.reg_ptr == 0x11u) { g_i2c.regs[0x13u] = b; }
        if (g_i2c.reg_ptr == 0x12u) { g_i2c.regs[0x14u] = b; }
    }
    g_i2c.reg_ptr++;
    return 1;
}

static uint8_t mock_i2c_read_byte(int ack)
{
    (void)ack;
    uint8_t v = 0u;
    if (g_i2c.reg_ptr < sizeof(g_i2c.regs)) {
        v = g_i2c.regs[g_i2c.reg_ptr];
    }
    g_i2c.reg_ptr++;
    return v;
}

typedef struct {
    uint8_t regs[22];
    uint8_t dev;            /* A2A1A0 value the mock answers to */
    int     expect_ctrl;    /* next exchange is the control byte */
    int     expect_reg;     /* next exchange is the register byte */
    uint8_t reg_ptr;
    int     reading;        /* control byte had the read bit */
} mock_spi_t;

static mock_spi_t g_spi;
static int g_spi_sel_count = 0;

static void mock_spi_select(void)
{
    g_spi_sel_count++;
    g_spi.expect_ctrl = 1;
    g_spi.expect_reg = 0;
}
static void mock_spi_deselect(void) { }

static uint8_t mock_spi_exchange(uint8_t b)
{
    if (g_spi.expect_ctrl) {
        g_spi.expect_ctrl = 0;
        g_spi.expect_reg = 1;
        g_spi.reading = (b & 0x01u) != 0u;
        uint8_t want = (uint8_t)(0x40u | ((g_spi.dev & 0x7u) << 1));
        return (b & 0xFEu) == want ? 0xFFu : 0x00u;   /* MISO idle */
    }
    if (g_spi.expect_reg) {
        g_spi.expect_reg = 0;
        g_spi.reg_ptr = b;   /* the register byte, both directions */
        return 0xFFu;
    }
    /* data bytes: writes go to the register file, reads return it. */
    if (g_spi.reading) {
        uint8_t v = (g_spi.reg_ptr < sizeof(g_spi.regs))
                        ? g_spi.regs[g_spi.reg_ptr] : 0u;
        g_spi.reg_ptr++;
        return v;
    }
    if (g_spi.reg_ptr < sizeof(g_spi.regs)) {
        g_spi.regs[g_spi.reg_ptr] = b;
        if (g_spi.reg_ptr == 0x11u) { g_spi.regs[0x13u] = b; }
        if (g_spi.reg_ptr == 0x12u) { g_spi.regs[0x14u] = b; }
    }
    g_spi.reg_ptr++;
    return 0xFFu;
}

typedef struct {
    uint8_t regs[22];
    int nack;
} mock_transport_t;

static mock_transport_t g_mock;

static int mock_read_reg(void *ctx, uint8_t reg, uint8_t *buf, int n)
{
    mock_transport_t *m = (mock_transport_t *)ctx;
    if (m->nack) {
        return -1;
    }
    for (int i = 0; i < n; i++) {
        buf[i] = (reg + i < sizeof(m->regs)) ? m->regs[reg + i] : 0u;
    }
    return n;
}

static int mock_write_reg(void *ctx, uint8_t reg, const uint8_t *buf, int n)
{
    mock_transport_t *m = (mock_transport_t *)ctx;
    if (m->nack) {
        return -1;
    }
    for (int i = 0; i < n; i++) {
        if (reg + i < sizeof(m->regs)) {
            m->regs[reg + i] = buf[i];
            if (reg + i == 0x11u) { m->regs[0x13u] = buf[i]; }
            if (reg + i == 0x12u) { m->regs[0x14u] = buf[i]; }
        }
    }
    return n;
}

static const epic_mcp23x17_transport_t g_mock_transport = {
    mock_read_reg, mock_write_reg, &g_mock
};

static void setup_i2c(uint8_t dev)
{
    memset(&g_i2c, 0, sizeof(g_i2c));
    g_i2c.dev = dev;
    g_i2c.regs[0x00] = 0xFFu;   /* POR: all inputs */
    g_i2c.regs[0x01] = 0xFFu;
    static const epic_bus_i2c_ops_t ops = {
        mock_i2c_start, mock_i2c_repeated_start, mock_i2c_stop,
        mock_i2c_write_byte, mock_i2c_read_byte
    };
    epic_bus_set_i2c_ops(&ops);
}

static void setup_spi(uint8_t dev)
{
    memset(&g_spi, 0, sizeof(g_spi));
    g_spi.dev = dev;
    g_spi.regs[0x00] = 0xFFu;   /* POR: all inputs */
    g_spi.regs[0x01] = 0xFFu;
    g_spi_sel_count = 0;
    static const epic_bus_spi_ops_t ops = {
        mock_spi_select, mock_spi_deselect, mock_spi_exchange
    };
    epic_bus_set_spi_ops(&ops);
}

static void run_semantics(epic_mcp23x17_handle_t *h, const char *label)
{
    uint8_t v;
    uint16_t w;
    int st;

    /* reset default: everything input (IODIR=0xFF), GPIO reads 0. */
    st = EPIC_MCP23X17_GetDirection(h, EPIC_MCP23X17_PORTA, &v);
    CHECK(st > 0 && v == 0xFFu);
    st = EPIC_MCP23X17_ReadPort(h, EPIC_MCP23X17_PORTB, &v);
    CHECK(st > 0 && v == 0x00u);

    /* direction: A low nibble out, B high nibble out. */
    st = EPIC_MCP23X17_SetDirection(h, EPIC_MCP23X17_PORTA, 0x0Fu);
    CHECK(st > 0);
    st = EPIC_MCP23X17_GetDirection(h, EPIC_MCP23X17_PORTA, &v);
    CHECK(st > 0 && v == 0x0Fu);
    st = EPIC_MCP23X17_SetDirectionAll(h, 0x0F0Fu);
    CHECK(st > 0);
    st = EPIC_MCP23X17_GetDirectionAll(h, &w);
    CHECK(st > 0 && w == 0x0F0Fu);

    /* GPIO write goes to the output latch; read returns the port. */
    st = EPIC_MCP23X17_WriteAll(h, 0xA5A5u);
    CHECK(st > 0);
    st = EPIC_MCP23X17_ReadOutputLatch(h, EPIC_MCP23X17_PORTA, &v);
    CHECK(st > 0 && v == 0xA5u);
    st = EPIC_MCP23X17_ReadOutputLatch(h, EPIC_MCP23X17_PORTB, &v);
    CHECK(st > 0 && v == 0xA5u);

    /* 16-bit read returns both ports (GPIOA low byte). */
    st = EPIC_MCP23X17_ReadAll(h, &w);
    CHECK(st > 0 && w == 0xA5A5u);
    st = EPIC_MCP23X17_ReadPort(h, EPIC_MCP23X17_PORTA, &v);
    CHECK(st > 0 && v == 0xA5u);

    /* polarity + pull-ups. */
    st = EPIC_MCP23X17_SetInputPolarity(h, EPIC_MCP23X17_PORTB, 0x80u);
    CHECK(st > 0);
    st = EPIC_MCP23X17_GetInputPolarity(h, EPIC_MCP23X17_PORTB, &v);
    CHECK(st > 0 && v == 0x80u);
    st = EPIC_MCP23X17_SetPullUpsAll(h, 0x00FFu);
    CHECK(st > 0);
    st = EPIC_MCP23X17_GetPullUps(h, EPIC_MCP23X17_PORTA, &v);
    CHECK(st > 0 && v == 0xFFu);
    st = EPIC_MCP23X17_GetPullUps(h, EPIC_MCP23X17_PORTB, &v);
    CHECK(st > 0 && v == 0x00u);

    /* IOCON config. */
    st = EPIC_MCP23X17_SetConfig(h, 0x40u);   /* MIRROR */
    CHECK(st > 0);
    st = EPIC_MCP23X17_GetConfig(h, &v);
    CHECK(st > 0 && v == 0x40u);

    /* interrupt enable / default / control + flag + capture. */
    st = EPIC_MCP23X17_SetInterruptEnable(h, EPIC_MCP23X17_PORTA, 0x01u);
    CHECK(st > 0);
    st = EPIC_MCP23X17_SetInterruptDefault(h, EPIC_MCP23X17_PORTA, 0x00u);
    CHECK(st > 0);
    st = EPIC_MCP23X17_SetInterruptControl(h, EPIC_MCP23X17_PORTA, 0x01u);
    CHECK(st > 0);
    st = EPIC_MCP23X17_ReadInterruptFlag(h, EPIC_MCP23X17_PORTA, &v);
    CHECK(st > 0 && v == 0x00u);
    st = EPIC_MCP23X17_ReadInterruptCapture(h, EPIC_MCP23X17_PORTA, &v);
    CHECK(st > 0 && v == 0x00u);

    (void)label;
}

static void run_mimic(epic_mcp23x17_handle_t *h)
{
    uint8_t dir, pu;
    int st;

    /* Init OUTPUT: only the masked direction bits clear, others keep
     * the POR default (input). */
    st = EPIC_MCP23X17_GPIO_Init(h, EPIC_MCP23X17_PORTA,
                                 MCP23X17_PIN_0 | MCP23X17_PIN_1,
                                 MCP23X17_MODE_OUTPUT);
    CHECK(st > 0);
    st = EPIC_MCP23X17_GetDirection(h, EPIC_MCP23X17_PORTA, &dir);
    CHECK(st > 0 && dir == 0xFCu);   /* bits 0,1 out; rest still in */

    /* Init INPUT_PULLUP: the pins go input with the pull-ups on. */
    st = EPIC_MCP23X17_GPIO_Init(h, EPIC_MCP23X17_PORTA,
                                 MCP23X17_PIN_2, MCP23X17_MODE_INPUT_PULLUP);
    CHECK(st > 0);
    st = EPIC_MCP23X17_GetDirection(h, EPIC_MCP23X17_PORTA, &dir);
    CHECK(st > 0 && (dir & MCP23X17_PIN_2) != 0u);
    st = EPIC_MCP23X17_GetPullUps(h, EPIC_MCP23X17_PORTA, &pu);
    CHECK(st > 0 && (pu & MCP23X17_PIN_2) != 0u);

    /* Init INPUT clears the pull-up of a previously-pulled pin. */
    st = EPIC_MCP23X17_GPIO_Init(h, EPIC_MCP23X17_PORTA,
                                 MCP23X17_PIN_2, MCP23X17_MODE_INPUT);
    CHECK(st > 0);
    st = EPIC_MCP23X17_GetPullUps(h, EPIC_MCP23X17_PORTA, &pu);
    CHECK(st > 0 && (pu & MCP23X17_PIN_2) == 0u);

    /* WritePin: masked RMW of the latch, others preserved. */
    st = EPIC_MCP23X17_GPIO_Init(h, EPIC_MCP23X17_PORTB, MCP23X17_PIN_All,
                                 MCP23X17_MODE_OUTPUT);
    CHECK(st > 0);
    st = EPIC_MCP23X17_WritePort(h, EPIC_MCP23X17_PORTB, 0x0Fu);
    CHECK(st > 0);
    st = EPIC_MCP23X17_GPIO_WritePin(h, EPIC_MCP23X17_PORTB,
                                     MCP23X17_PIN_4, MCP23X17_PIN_SET);
    CHECK(st > 0);
    st = EPIC_MCP23X17_ReadOutputLatch(h, EPIC_MCP23X17_PORTB, &pu);
    CHECK(st > 0 && pu == 0x1Fu);   /* 0x0F plus bit 4, bits 5-7 kept */
    st = EPIC_MCP23X17_GPIO_WritePin(h, EPIC_MCP23X17_PORTB,
                                     MCP23X17_PIN_0, MCP23X17_PIN_RESET);
    CHECK(st > 0);
    st = EPIC_MCP23X17_ReadOutputLatch(h, EPIC_MCP23X17_PORTB, &pu);
    CHECK(st > 0 && pu == 0x1Eu);

    /* TogglePin flips only the masked bits. */
    st = EPIC_MCP23X17_GPIO_TogglePin(h, EPIC_MCP23X17_PORTB,
                                      MCP23X17_PIN_1 | MCP23X17_PIN_4);
    CHECK(st > 0);
    st = EPIC_MCP23X17_ReadOutputLatch(h, EPIC_MCP23X17_PORTB, &pu);
    CHECK(st > 0 && pu == 0x0Cu);   /* bits 1 and 4 toggled off */

    /* ReadPin extracts the bit state from the port. */
    st = EPIC_MCP23X17_GPIO_ReadPin(h, EPIC_MCP23X17_PORTB, MCP23X17_PIN_2);
    CHECK(st == MCP23X17_PIN_SET);
    st = EPIC_MCP23X17_GPIO_ReadPin(h, EPIC_MCP23X17_PORTB, MCP23X17_PIN_0);
    CHECK(st == MCP23X17_PIN_RESET);

    /* NACK surfaces as -1 on the mimic too. */
    g_mock.nack = 1;
    st = EPIC_MCP23X17_GPIO_WritePin(h, EPIC_MCP23X17_PORTA,
                                     MCP23X17_PIN_0, MCP23X17_PIN_SET);
    CHECK(st == -1);
    st = EPIC_MCP23X17_GPIO_ReadPin(h, EPIC_MCP23X17_PORTA, MCP23X17_PIN_0);
    CHECK(st == -1);
    g_mock.nack = 0;
}

int main(void)
{
    epic_mcp23x17_handle_t h;

    /* (1) I2C path: the built-in transport over the mock I2C device. */
    setup_i2c(0x20);
    CHECK(EPIC_MCP23X17_Init(&h, EPIC_MCP23X17_BUS_I2C, 0x20) == 0);
    run_semantics(&h, "i2c");

    /* I2C device mismatch: the mock NACKs the address, the accessor
     * must surface -1. */
    setup_i2c(0x20);
    CHECK(EPIC_MCP23X17_Init(&h, EPIC_MCP23X17_BUS_I2C, 0x21) == 0);
    CHECK(EPIC_MCP23X17_GetDirection(&h, EPIC_MCP23X17_PORTA, &(uint8_t){0}) == -1);

    /* forced address NACK. */
    setup_i2c(0x20);
    g_i2c.nack_addr = 1;
    CHECK(EPIC_MCP23X17_Init(&h, EPIC_MCP23X17_BUS_I2C, 0x20) == 0);
    CHECK(EPIC_MCP23X17_WritePort(&h, EPIC_MCP23X17_PORTA, 0x01u) == -1);

    /* (2) SPI path: the built-in transport over the mock SPI device.
     * The control-byte framing is asserted by the mock's MISO check. */
    setup_spi(0);
    CHECK(EPIC_MCP23X17_Init(&h, EPIC_MCP23X17_BUS_SPI, 0) == 0);
    run_semantics(&h, "spi");
    CHECK(g_spi_sel_count > 0);   /* CS was asserted */

    /* SPI with HAEN-style addressing: dev selects the control byte's
     * A2A1A0 bits. */
    setup_spi(5);
    CHECK(EPIC_MCP23X17_Init(&h, EPIC_MCP23X17_BUS_SPI, 5) == 0);
    CHECK(EPIC_MCP23X17_SetDirection(&h, EPIC_MCP23X17_PORTA, 0x00u) == 1);

    /* (3) the module's own injectable transport. */
    memset(&g_mock, 0, sizeof(g_mock));
    g_mock.regs[0x00] = 0xFFu;   /* POR: all inputs */
    g_mock.regs[0x01] = 0xFFu;
    CHECK(EPIC_MCP23X17_InitTransport(&h, &g_mock_transport) == 0);
    run_semantics(&h, "transport");
    memset(&g_mock, 0, sizeof(g_mock));
    g_mock.regs[0x00] = 0xFFu;   /* back to the POR defaults */
    g_mock.regs[0x01] = 0xFFu;
    run_mimic(&h);
    g_mock.nack = 1;
    CHECK(EPIC_MCP23X17_ReadPort(&h, EPIC_MCP23X17_PORTA, &(uint8_t){0}) == -1);

    if (g_fail) {
        printf("test_mcp23x17: FAILURES\n");
        return 1;
    }
    printf("test_mcp23x17: all checks passed\n");
    return 0;
}
