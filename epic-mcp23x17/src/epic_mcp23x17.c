/**
 * @file    epic_mcp23x17.c
 * @brief   MCP23017 / MCP23S17 16-bit I/O expander driver (see the
 *          header and docs/ARCHITECTURE.md). One family-agnostic
 *          source; the only family dimension is the transport, which
 *          rides on epic-bus.
 */

#include "epic_mcp23x17.h"

#include "epic_bus.h"   /* built-in I2C mem + raw SPI ops transports */

/* Register map, IOCON.BANK=0 (sequential). Each port pair is the
 * base plus the port index. DS20001952E section 3.5. */
enum {
    REG_IODIR   = 0x00,   /* 1 = input (reset default) */
    REG_IPOL    = 0x02,   /* 1 = inverted input polarity */
    REG_GPINTEN = 0x04,   /* 1 = interrupt-on-change enabled per pin */
    REG_DEFVAL  = 0x06,   /* compare value for INTCON=1 pins */
    REG_INTCON  = 0x08,   /* 1 = compare to DEFVAL, 0 = on pin change */
    REG_IOCON   = 0x0A,   /* BANK MIRROR SEQOP DISSLW HAEN ODR INTPOL - */
    REG_GPPU    = 0x0B,   /* 1 = 100k pull-up on input pins */
    REG_INTF    = 0x0D,   /* read-only pending flags */
    REG_INTCAP  = 0x0F,   /* read-only captured port value */
    REG_GPIO    = 0x11,   /* read pins / write the output latch */
    REG_OLAT    = 0x13,   /* read the output latch */
};

/* SPI control byte: 0b0100_0A2A1A0_RW (HAEN disabled: address bits
 * from the dev field; RW=1 for reads). */
#define SPI_CTRL(dev, rw) ((uint8_t)(0x40u | (((dev) & 0x7u) << 1) | (rw)))

static uint8_t reg_of(epic_mcp23x17_port_t port, uint8_t base)
{
    return (uint8_t)(base + (uint8_t)port);
}

static int reg_read(epic_mcp23x17_handle_t *h, uint8_t reg,
                    uint8_t *buf, int n)
{
    if (h->transport != NULL) {
        return h->transport->read_reg(h->transport->ctx, reg, buf, n);
    }
    if (h->bus == EPIC_MCP23X17_BUS_I2C) {
        return epic_bus_i2c_mem_read(h->dev, reg, buf, n);
    }
    /* MCP23S17: CS low, control byte (read), reg, n dummy exchanges
     * capturing MISO, CS high. */
    const epic_bus_spi_ops_t *ops = epic_bus_get_spi_ops();
    ops->select();
    (void)ops->exchange(SPI_CTRL(h->dev, 1u));
    (void)ops->exchange(reg);
    for (int i = 0; i < n; i++) {
        buf[i] = ops->exchange(0u);
    }
    ops->deselect();
    return n;
}

static int reg_write(epic_mcp23x17_handle_t *h, uint8_t reg,
                     const uint8_t *buf, int n)
{
    if (h->transport != NULL) {
        return h->transport->write_reg(h->transport->ctx, reg, buf, n);
    }
    if (h->bus == EPIC_MCP23X17_BUS_I2C) {
        return epic_bus_i2c_mem_write(h->dev, reg, buf, n);
    }
    const epic_bus_spi_ops_t *ops = epic_bus_get_spi_ops();
    ops->select();
    (void)ops->exchange(SPI_CTRL(h->dev, 0u));
    (void)ops->exchange(reg);
    for (int i = 0; i < n; i++) {
        (void)ops->exchange(buf[i]);
    }
    ops->deselect();
    return n;
}

/* ─── lifecycle ─────────────────────────────────────────────────── */

int EPIC_MCP23X17_Init(epic_mcp23x17_handle_t *h,
                       epic_mcp23x17_bus_t bus, uint8_t dev)
{
    h->bus = bus;
    h->dev = dev;
    h->transport = NULL;
    return 0;
}

int EPIC_MCP23X17_InitTransport(epic_mcp23x17_handle_t *h,
                                const epic_mcp23x17_transport_t *t)
{
    h->bus = EPIC_MCP23X17_BUS_I2C;   /* informational only */
    h->dev = 0u;
    h->transport = t;
    return 0;
}

/* ─── per-port register access ──────────────────────────────────── */

int EPIC_MCP23X17_SetDirection(epic_mcp23x17_handle_t *h,
                               epic_mcp23x17_port_t port, uint8_t dir)
{
    return reg_write(h, reg_of(port, REG_IODIR), &dir, 1);
}

int EPIC_MCP23X17_GetDirection(epic_mcp23x17_handle_t *h,
                               epic_mcp23x17_port_t port, uint8_t *dir)
{
    return reg_read(h, reg_of(port, REG_IODIR), dir, 1);
}

int EPIC_MCP23X17_SetInputPolarity(epic_mcp23x17_handle_t *h,
                                   epic_mcp23x17_port_t port, uint8_t pol)
{
    return reg_write(h, reg_of(port, REG_IPOL), &pol, 1);
}

int EPIC_MCP23X17_GetInputPolarity(epic_mcp23x17_handle_t *h,
                                   epic_mcp23x17_port_t port, uint8_t *pol)
{
    return reg_read(h, reg_of(port, REG_IPOL), pol, 1);
}

int EPIC_MCP23X17_SetPullUps(epic_mcp23x17_handle_t *h,
                             epic_mcp23x17_port_t port, uint8_t pu)
{
    return reg_write(h, reg_of(port, REG_GPPU), &pu, 1);
}

int EPIC_MCP23X17_GetPullUps(epic_mcp23x17_handle_t *h,
                             epic_mcp23x17_port_t port, uint8_t *pu)
{
    return reg_read(h, reg_of(port, REG_GPPU), pu, 1);
}

int EPIC_MCP23X17_WritePort(epic_mcp23x17_handle_t *h,
                            epic_mcp23x17_port_t port, uint8_t val)
{
    return reg_write(h, reg_of(port, REG_GPIO), &val, 1);
}

int EPIC_MCP23X17_ReadPort(epic_mcp23x17_handle_t *h,
                           epic_mcp23x17_port_t port, uint8_t *val)
{
    return reg_read(h, reg_of(port, REG_GPIO), val, 1);
}

int EPIC_MCP23X17_ReadOutputLatch(epic_mcp23x17_handle_t *h,
                                  epic_mcp23x17_port_t port, uint8_t *val)
{
    return reg_read(h, reg_of(port, REG_OLAT), val, 1);
}

/* ─── 16-bit composite helpers (low byte = PORTA) ───────────────── */

int EPIC_MCP23X17_SetDirectionAll(epic_mcp23x17_handle_t *h, uint16_t dir)
{
    uint8_t pair[2] = { (uint8_t)dir, (uint8_t)(dir >> 8) };
    return reg_write(h, REG_IODIR, pair, 2);
}

int EPIC_MCP23X17_GetDirectionAll(epic_mcp23x17_handle_t *h, uint16_t *dir)
{
    uint8_t pair[2];
    int st = reg_read(h, REG_IODIR, pair, 2);
    if (st < 0) {
        return st;
    }
    *dir = (uint16_t)((uint16_t)pair[0] | ((uint16_t)pair[1] << 8));
    return st;
}

int EPIC_MCP23X17_WriteAll(epic_mcp23x17_handle_t *h, uint16_t val)
{
    uint8_t pair[2] = { (uint8_t)val, (uint8_t)(val >> 8) };
    return reg_write(h, REG_GPIO, pair, 2);
}

int EPIC_MCP23X17_ReadAll(epic_mcp23x17_handle_t *h, uint16_t *val)
{
    uint8_t pair[2];
    int st = reg_read(h, REG_GPIO, pair, 2);
    if (st < 0) {
        return st;
    }
    *val = (uint16_t)((uint16_t)pair[0] | ((uint16_t)pair[1] << 8));
    return st;
}

int EPIC_MCP23X17_SetPullUpsAll(epic_mcp23x17_handle_t *h, uint16_t pu)
{
    uint8_t pair[2] = { (uint8_t)pu, (uint8_t)(pu >> 8) };
    return reg_write(h, REG_GPPU, pair, 2);
}

/* ─── IOCON configuration ───────────────────────────────────────── */

int EPIC_MCP23X17_SetConfig(epic_mcp23x17_handle_t *h, uint8_t iocon)
{
    return reg_write(h, REG_IOCON, &iocon, 1);
}

int EPIC_MCP23X17_GetConfig(epic_mcp23x17_handle_t *h, uint8_t *iocon)
{
    return reg_read(h, REG_IOCON, iocon, 1);
}

/* ─── interrupt support ─────────────────────────────────────────── */

int EPIC_MCP23X17_SetInterruptEnable(epic_mcp23x17_handle_t *h,
                                     epic_mcp23x17_port_t port, uint8_t mask)
{
    return reg_write(h, reg_of(port, REG_GPINTEN), &mask, 1);
}

int EPIC_MCP23X17_SetInterruptDefault(epic_mcp23x17_handle_t *h,
                                      epic_mcp23x17_port_t port, uint8_t val)
{
    return reg_write(h, reg_of(port, REG_DEFVAL), &val, 1);
}

int EPIC_MCP23X17_SetInterruptControl(epic_mcp23x17_handle_t *h,
                                      epic_mcp23x17_port_t port, uint8_t mask)
{
    return reg_write(h, reg_of(port, REG_INTCON), &mask, 1);
}

int EPIC_MCP23X17_ReadInterruptFlag(epic_mcp23x17_handle_t *h,
                                    epic_mcp23x17_port_t port, uint8_t *flags)
{
    return reg_read(h, reg_of(port, REG_INTF), flags, 1);
}

int EPIC_MCP23X17_ReadInterruptCapture(epic_mcp23x17_handle_t *h,
                                       epic_mcp23x17_port_t port,
                                       uint8_t *capture)
{
    return reg_read(h, reg_of(port, REG_INTCAP), capture, 1);
}
