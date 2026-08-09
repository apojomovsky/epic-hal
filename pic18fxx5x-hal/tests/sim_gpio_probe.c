/**
 * @file    sim_gpio_probe.c
 * @brief   HARNESS=sim probe for the pic18fxx5x-hal GPIO audit
 *          (docs/toolchain-coverage.md class C): the GPIO driver
 *          computes TRIS/LAT/PORT addresses at runtime and derefs
 *          them, the exact shape pic18_irq.c and pic18fxx5x_ccp.c were
 *          fixed to avoid (a runtime-computed SFR address compiles to
 *          program-memory table access under XC8 v4.00, PIC18 Finding
 *          3). Every GPIO operation is exercised with known values and
 *          verified through the literal-token access path
 *          (epic_sfr_read8/write8 with compile-time PIC_REG_*
 *          constants), which is the proven-safe side.
 *
 *   Runs as the sim variant of the pic18fxx5x-hal pseudo-module under
 *   MPLAB SIM (real XC8 v4.00 code), reporting through the PIC18 sim
 *   harness.
 */

#include "core/epic_harness.h"
#include "peripherals/pic18fxx5x_gpio.h"
#include "target/pic18_platform.h"

#include <stdint.h>

static uint16_t g_fail = 0u;

static void fail(uint8_t idx)
{
    static const char hx[] = "0123456789ABCDEF";
    char c[2];
    g_fail++;
    epic_harness_log("F");
    c[0] = hx[(idx >> 4) & 0xF];
    c[1] = '\0';
    epic_harness_log(c);
    c[0] = hx[idx & 0xF];
    c[1] = '\0';
    epic_harness_log(c);
    epic_harness_log(".");
}

#define CHECK(cond, idx) do {         \
    if (!(cond)) fail(idx);            \
} while (0)

int main(void)
{
    epic_harness_init(0UL);

    /* Init PORTB all outputs: TRISB must read 0x00 through the
     * literal-token path. */
    EPIC_GPIO_Init(GPIOB, 0xFFu, GPIO_MODE_OUTPUT);
    CHECK(epic_sfr_read8(PIC_REG_TRISB) == 0x00u, 0x00);

    /* Init PORTA low nibble inputs: TRISA low nibble set. */
    EPIC_GPIO_Init(GPIOA, 0x0Fu, GPIO_MODE_INPUT);
    CHECK((epic_sfr_read8(PIC_REG_TRISA) & 0x0Fu) == 0x0Fu, 0x01);

    /* WritePin high: LATB bit 0 and PORTB bit 0 must both read 1. */
    EPIC_GPIO_WritePin(GPIOB, 0x01u, GPIO_PIN_SET);
    CHECK((epic_sfr_read8(PIC_REG_LATB) & 0x01u) == 0x01u, 0x02);
    CHECK((epic_sfr_read8(PIC_REG_PORTB) & 0x01u) == 0x01u, 0x03);

    /* TogglePin: LATB bit 0 flips to 0. */
    EPIC_GPIO_TogglePin(GPIOB, 0x01u);
    CHECK((epic_sfr_read8(PIC_REG_LATB) & 0x01u) == 0x00u, 0x04);

    /* WritePin low: back to 1 via a second write, then ReadPin. */
    EPIC_GPIO_WritePin(GPIOB, 0x01u, GPIO_PIN_SET);
    CHECK(EPIC_GPIO_ReadPin(GPIOB, 0x01u) == GPIO_PIN_SET, 0x05);
    EPIC_GPIO_WritePin(GPIOB, 0x01u, GPIO_PIN_RESET);
    CHECK(EPIC_GPIO_ReadPin(GPIOB, 0x01u) == GPIO_PIN_RESET, 0x06);

    /* Multi-bit WritePin on PORTB (0xAA pattern) and WritePort. */
    EPIC_GPIO_WritePin(GPIOB, 0xAAu, GPIO_PIN_SET);
    CHECK((epic_sfr_read8(PIC_REG_LATB) & 0xAAu) == 0xAAu, 0x07);
    EPIC_GPIO_WritePort(GPIOB, 0x55u);
    CHECK(epic_sfr_read8(PIC_REG_LATB) == 0x55u, 0x08);
    CHECK(epic_sfr_read8(PIC_REG_PORTB) == 0x55u, 0x09);

    /* DeInit returns TRISB to all inputs (0xFF). */
    EPIC_GPIO_DeInit(GPIOB);
    CHECK(epic_sfr_read8(PIC_REG_TRISB) == 0xFFu, 0x0A);

    for (uint32_t i = 0; epic_harness_running(i); i++) {
        epic_harness_tick();
    }
    return epic_harness_report(g_fail == 0u);
}
