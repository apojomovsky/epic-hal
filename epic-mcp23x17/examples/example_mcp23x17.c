/**
 * Host-side example: drives the MCP23017 through the module with a
 * mock device injected at the epic-bus ops seam (the host sim has no
 * SSP slave model). The real-target analog is example_mcp23x17_target.c.
 */

#include "epic_mcp23x17.h"

#include "epic_bus.h"

#include <stdint.h>
#include <stdio.h>
#include <string.h>

/* A tiny mock MCP23017 register file, wired as the epic-bus I2C ops. */
static uint8_t s_regs[22];
static uint8_t s_reg_ptr;
static int     s_phase;

enum { MOCK_ADDR, MOCK_REG, MOCK_DATA, MOCK_RADDR, MOCK_READ };

static void mock_start(void)               { s_phase = MOCK_ADDR; }
static void mock_repeated_start(void)      { s_phase = MOCK_RADDR; }
static void mock_stop(void)                { }

static int mock_write_byte(uint8_t b)
{
    if (s_phase == MOCK_ADDR || s_phase == MOCK_RADDR) {
        if ((b >> 1) != 0x20u) {
            return 0;   /* NACK anything but our device */
        }
        s_phase = (s_phase == MOCK_ADDR) ? MOCK_REG : MOCK_READ;
        return 1;
    }
    if (s_phase == MOCK_REG) {
        s_reg_ptr = b & 0x1Fu;
        s_phase = MOCK_DATA;
        return 1;
    }
    if (s_reg_ptr < sizeof(s_regs)) {
        s_regs[s_reg_ptr] = b;
        if (s_reg_ptr == 0x11u) { s_regs[0x13u] = b; }  /* GPIO->OLAT */
        if (s_reg_ptr == 0x12u) { s_regs[0x14u] = b; }
    }
    s_reg_ptr++;
    return 1;
}

static uint8_t mock_read_byte(int ack)
{
    (void)ack;
    uint8_t v = (s_reg_ptr < sizeof(s_regs)) ? s_regs[s_reg_ptr] : 0u;
    s_reg_ptr++;
    return v;
}

int main(void)
{
    static const epic_bus_i2c_ops_t ops = {
        mock_start, mock_repeated_start, mock_stop,
        mock_write_byte, mock_read_byte
    };
    epic_bus_set_i2c_ops(&ops);

    epic_mcp23x17_handle_t h;
    EPIC_MCP23X17_Init(&h, EPIC_MCP23X17_BUS_I2C, 0x20);

    /* dir low byte = PORTA (GPA0-3 out, GPA4-7 in); GPB all out. */
    uint16_t dir = 0x00F0u;
    if (EPIC_MCP23X17_SetDirectionAll(&h, dir) < 0) {
        printf("expander not on the bus\n");
        return 1;
    }

    /* Drive the outputs, then read the port back. */
    (void)EPIC_MCP23X17_WriteAll(&h, 0x00AAu);
    uint8_t latch;
    uint16_t port;
    (void)EPIC_MCP23X17_ReadOutputLatch(&h, EPIC_MCP23X17_PORTA, &latch);
    (void)EPIC_MCP23X17_ReadAll(&h, &port);

    /* The GPIO-mimic layer: the HAL-shaped per-pin calls. */
    (void)EPIC_MCP23X17_GPIO_Init(&h, EPIC_MCP23X17_PORTB, MCP23X17_PIN_All,
                                  MCP23X17_MODE_OUTPUT);
    (void)EPIC_MCP23X17_GPIO_WritePin(&h, EPIC_MCP23X17_PORTB,
                                      MCP23X17_PIN_2, MCP23X17_PIN_SET);
    (void)EPIC_MCP23X17_GPIO_TogglePin(&h, EPIC_MCP23X17_PORTB,
                                       MCP23X17_PIN_2);
    int pb2 = EPIC_MCP23X17_GPIO_ReadPin(&h, EPIC_MCP23X17_PORTB,
                                         MCP23X17_PIN_2);

    printf("latch A=0x%02X port=0x%04X PB2=%d\n", (unsigned)latch,
           (unsigned)port, pb2);
    return (latch == 0xAAu && port == 0x00AAu && pb2 == MCP23X17_PIN_RESET)
               ? 0 : 1;
}
