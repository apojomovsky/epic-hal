/**
 * Data EEPROM host-sim probe. Host-only: advances the NVM model through
 * epic_harness_tick, so it is not in the XC8 build (the mdb gate probe
 * is example_gpio_mdb.c). Covers the blank-part read, the NVM register
 * image of a read and a write, write completion with and without a
 * duration, the busy refusal, WREN drop on completion, the completion
 * callback through the shared dispatcher, the error flag, and DeInit.
 *
 * Expected register image for byte 0x42 (NVMREGS=1 selects the EEPROM
 * space, 0xF000 + 0x42 is the NVM address):
 *   after a read:  NVMADRL = 0x42, NVMADRH = 0xF0, NVMCON1 = 0x40
 *   after a write: NVMADRL = 0x42, NVMADRH = 0xF0, NVMDATL = data,
 *                  NVMCON1 = 0x44 (NVMREGS|WREN, WR already clear)
 *   once polled complete: NVMCON1 = 0x40 (WREN dropped)
 * A write that was refused or broke the key sequence reads
 * NVMCON1 = 0x4C (NVMREGS|WREN|WRERR).
 */
#include <stddef.h>
#include "pic16f188xx.h"
#include "pic16f188xx_sim.h"
#include "core/pic16f188xx_irq.h"
#include "core/epic_harness.h"
#include "peripherals/pic16f188xx_eeprom.h"

static unsigned int s_calls;

/**
 * @brief Write-complete callback that records its call.
 */
static void done_cb(void)
{
    s_calls++;
}

/**
 * @brief Step the simulator n instruction ticks.
 * @param n Tick count.
 */
static void step(unsigned int n)
{
    unsigned int i;

    for (i = 0U; i < n; i++)
    {
        epic_harness_tick();
    }
}

/**
 * @brief Data EEPROM checks on the host simulator.
 * @return Harness result: 0 on PASS, 1 on FAIL.
 */
int main(void)
{
    int ok = 1;
    unsigned int i;

    epic_harness_init(10000UL);
    EPIC_IRQ_Restore(1U);

    /* POR: NVMADRH reads 0x80; a blank part reads 0xFF everywhere. */
    ok &= (EPIC_REG8(PIC_REG_NVMADRH) == 0x80U);
    ok &= (EPIC_EEPROM_Init(NULL) == EPIC_OK);
    ok &= (EPIC_REG8(PIC_REG_NVMCON1) == 0x00U);
    ok &= ((EPIC_REG8(PIC_REG_PIE7) & PIC_PIE7_NVMIE) == 0U);
    ok &= (EPIC_EEPROM_ReadByte(0U) == 0xFFU);
    ok &= (EPIC_EEPROM_ReadByte(255U) == 0xFFU);
    ok &= (EPIC_REG8(PIC_REG_NVMADRL) == 0xFFU);
    ok &= (EPIC_REG8(PIC_REG_NVMADRH) == 0xF0U);
    ok &= (EPIC_REG8(PIC_REG_NVMCON1) == 0x40U);

    /* Instant write: the byte lands, NVMIF is set and the register image
     * matches the header; polling drops WREN. */
    ok &= (EPIC_EEPROM_WriteByte(0x42U, 0xA5U) == EPIC_OK);
    ok &= (pic16f188xx_sim_eeprom_peek(0x42U) == 0xA5U);
    ok &= (EPIC_REG8(PIC_REG_NVMADRL) == 0x42U);
    ok &= (EPIC_REG8(PIC_REG_NVMADRH) == 0xF0U);
    ok &= (EPIC_REG8(PIC_REG_NVMDATL) == 0xA5U);
    ok &= (EPIC_REG8(PIC_REG_NVMCON1) == 0x44U);
    ok &= (EPIC_IRQ_GetFlag(PIC16F188XX_IRQ_EEPROM) == 1U);
    ok &= (EPIC_EEPROM_IsWriteComplete() == 1U);
    ok &= (EPIC_REG8(PIC_REG_NVMCON1) == 0x40U);
    ok &= (EPIC_EEPROM_HasWriteError() == 0U);
    ok &= (EPIC_EEPROM_ReadByte(0x42U) == 0xA5U);
    ok &= (EPIC_REG8(PIC_REG_NVMDATL) == 0xA5U);
    ok &= (EPIC_EEPROM_ReadByte(0x41U) == 0xFFU);
    ok &= (EPIC_EEPROM_ReadByte(0x43U) == 0xFFU);

    /* Timed write: WR stays set for 5 ticks, a second write is refused
     * without touching the address registers, the 5th tick completes. */
    EPIC_IRQ_ClearFlag(PIC16F188XX_IRQ_EEPROM);
    pic16f188xx_sim_eeprom_set_write_ticks(5U);
    ok &= (EPIC_EEPROM_WriteByte(0x10U, 0x3CU) == EPIC_OK);
    ok &= ((EPIC_REG8(PIC_REG_NVMCON1) & PIC_NVMCON1_WR) != 0U);
    ok &= (EPIC_EEPROM_IsWriteComplete() == 0U);
    ok &= (pic16f188xx_sim_eeprom_peek(0x10U) == 0xFFU);
    ok &= (EPIC_EEPROM_WriteByte(0x11U, 0x77U) == EPIC_BUSY);
    ok &= (EPIC_REG8(PIC_REG_NVMADRL) == 0x10U);
    step(4U);
    ok &= (EPIC_EEPROM_IsWriteComplete() == 0U);
    ok &= (EPIC_IRQ_GetFlag(PIC16F188XX_IRQ_EEPROM) == 0U);
    step(1U);
    ok &= (EPIC_EEPROM_IsWriteComplete() == 1U);
    ok &= (pic16f188xx_sim_eeprom_peek(0x10U) == 0x3CU);
    ok &= (pic16f188xx_sim_eeprom_peek(0x11U) == 0xFFU);
    ok &= (EPIC_IRQ_GetFlag(PIC16F188XX_IRQ_EEPROM) == 1U);
    ok &= (EPIC_EEPROM_ReadByte(0x10U) == 0x3CU);

    /* Completion callback: Init arms NVMIE and PEIE, the dispatcher clears
     * NVMIF and runs the slot once, on the tick the write finishes. */
    EPIC_IRQ_ClearFlag(PIC16F188XX_IRQ_EEPROM);
    s_calls = 0U;
    ok &= (EPIC_EEPROM_Init(done_cb) == EPIC_OK);
    ok &= ((EPIC_REG8(PIC_REG_PIE7) & PIC_PIE7_NVMIE) != 0U);
    ok &= ((EPIC_REG8(PIC_REG_INTCON) & PIC_INTCON_PEIE) != 0U);
    pic16f188xx_sim_eeprom_set_write_ticks(3U);
    ok &= (EPIC_EEPROM_WriteByte(0x20U, 0x11U) == EPIC_OK);
    step(2U);
    ok &= (s_calls == 0U);
    step(1U);
    ok &= (s_calls == 1U);
    ok &= (EPIC_IRQ_GetFlag(PIC16F188XX_IRQ_EEPROM) == 0U);
    ok &= (pic16f188xx_sim_eeprom_peek(0x20U) == 0x11U);
    step(5U);
    ok &= (s_calls == 1U);

    /* Error flag: a broken key sequence (0x55 then WR, no 0xAA) leaves WR
     * clear and sets WRERR; the next write clears it before starting. */
    pic16f188xx_sim_eeprom_set_write_ticks(0U);
    ok &= (EPIC_EEPROM_Init(NULL) == EPIC_OK);
    epic_sfr_write8(PIC_REG_NVMCON1, (uint8_t)(PIC_NVMCON1_NVMREGS | PIC_NVMCON1_WREN));
    epic_sfr_write8(PIC_REG_NVMCON2, 0x55U);
    epic_sfr_write8(PIC_REG_NVMCON1,
        (uint8_t)(PIC_NVMCON1_NVMREGS | PIC_NVMCON1_WREN | PIC_NVMCON1_WR));
    ok &= (EPIC_REG8(PIC_REG_NVMCON1) == 0x4CU);
    ok &= (EPIC_EEPROM_HasWriteError() == 1U);
    ok &= (EPIC_EEPROM_IsWriteComplete() == 1U);
    ok &= (EPIC_REG8(PIC_REG_NVMCON1) == 0x48U);
    ok &= (EPIC_EEPROM_WriteByte(0x30U, 0x5AU) == EPIC_OK);
    ok &= (EPIC_EEPROM_HasWriteError() == 0U);
    ok &= (pic16f188xx_sim_eeprom_peek(0x30U) == 0x5AU);

    /* Whole array: every address stores and reads back its own value. */
    for (i = 0U; i < EEPROM_SIZE_BYTES; i++)
    {
        ok &= (EPIC_EEPROM_WriteByte((uint8_t)i, (uint8_t)(i ^ 0x5AU)) == EPIC_OK);
        ok &= (EPIC_EEPROM_IsWriteComplete() == 1U);
    }
    for (i = 0U; i < EEPROM_SIZE_BYTES; i++)
    {
        ok &= (EPIC_EEPROM_ReadByte((uint8_t)i) == (uint8_t)(i ^ 0x5AU));
    }

    /* DeInit: POR NVMCON1, interrupt off, callback dropped. */
    ok &= (EPIC_EEPROM_Init(done_cb) == EPIC_OK);
    ok &= (EPIC_EEPROM_DeInit() == EPIC_OK);
    ok &= (EPIC_REG8(PIC_REG_NVMCON1) == 0x00U);
    ok &= ((EPIC_REG8(PIC_REG_PIE7) & PIC_PIE7_NVMIE) == 0U);
    s_calls = 0U;
    pic16f188xx_sim_eeprom_set_write_ticks(2U);
    ok &= (EPIC_EEPROM_WriteByte(0x01U, 0x99U) == EPIC_OK);
    step(4U);
    ok &= (s_calls == 0U);
    ok &= (pic16f188xx_sim_eeprom_peek(0x01U) == 0x99U);

    epic_harness_log("eeprom: %s\n", ok ? "PASS" : "FAIL");
    return epic_harness_report(ok);
}
