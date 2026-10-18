/**
 * Data EEPROM driver, implementation (DS40001802D §10.4). Every access
 * names its register literally, so XC8 emits a bank select per access.
 * NVMCON1 reads and writes go through epic_sfr_read8/write8, so the host
 * simulator can model the RD and WR actions. The callback slot is a
 * file-scope static written from main context only.
 */
#include <stddef.h>
#include "peripherals/pic16f188xx_eeprom.h"
#include "core/pic16f188xx_irq.h"

/* NVMADRH for the data EEPROM: the address is 0xF000 + byte (Table 10-3);
 * bit 7 reads back as 1 whatever is written (Register 10-4). */
#define EEPROM_ADRH 0xF0U

/* ISR callback slot, written from main context only. A scalar, not an
 * array element: the handler references a fixed symbol. */
static void (*s_eeprom_done_cb)(void) = NULL;

/**
 * @brief Prepare the data EEPROM driver.
 * @details Leaves WREN and NVMIF clear and stores the callback before the
 * interrupt is armed, so a completion never calls a stale slot.
 * @param callback Write-complete callback, or NULL to poll.
 * @return EPIC_OK on success.
 */
EPIC_StatusTypeDef EPIC_EEPROM_Init(void (*callback)(void))
{
    epic_sfr_write8(PIC_REG_NVMCON1, 0U);
    EPIC_IRQ_ClearFlag(PIC16F188XX_IRQ_EEPROM);
    s_eeprom_done_cb = callback;
    if (callback != NULL)
    {
        EPIC_IRQ_Enable(PIC16F188XX_IRQ_EEPROM);
    }
    else
    {
        EPIC_IRQ_DisableSrc(PIC16F188XX_IRQ_EEPROM);
    }
    return EPIC_OK;
}

/**
 * @brief Disable the NVM interrupt and restore NVMCON1 to its POR state.
 * @return EPIC_OK on success.
 */
EPIC_StatusTypeDef EPIC_EEPROM_DeInit(void)
{
    EPIC_IRQ_DisableSrc(PIC16F188XX_IRQ_EEPROM);
    EPIC_IRQ_ClearFlag(PIC16F188XX_IRQ_EEPROM);
    epic_sfr_write8(PIC_REG_NVMCON1, 0U);
    s_eeprom_done_cb = NULL;
    return EPIC_OK;
}

/**
 * @brief Read one byte from the data EEPROM.
 * @details Waits out a write in progress, loads NVMADR and sets RD with
 * NVMREGS; NVMDATL holds the byte in the next instruction (§10.4.1).
 * WREN is dropped and WRERR kept by the NVMCON1 write.
 * @param addr Byte address 0..255.
 * @return The byte stored at addr.
 */
uint8_t EPIC_EEPROM_ReadByte(uint8_t addr)
{
    while ((epic_sfr_read8(PIC_REG_NVMCON1) & PIC_NVMCON1_WR) != 0U)
    {
    }
    EPIC_REG8(PIC_REG_NVMADRL) = addr;
    EPIC_REG8(PIC_REG_NVMADRH) = EEPROM_ADRH;
    epic_sfr_write8(PIC_REG_NVMCON1,
        (uint8_t)((epic_sfr_read8(PIC_REG_NVMCON1) & PIC_NVMCON1_WRERR) |
                  PIC_NVMCON1_NVMREGS | PIC_NVMCON1_RD));
    return epic_sfr_read8(PIC_REG_NVMDATL);
}

/**
 * @brief Start a byte write to the data EEPROM.
 * @details One NVMCON1 write sets NVMREGS and WREN and clears WRERR, FREE
 * and LWLO (FREE would turn the write into an erase). The unlock runs with
 * GIE off; the write then proceeds without the CPU (§10.4.3).
 * @param addr Byte address 0..255.
 * @param data Byte to store.
 * @return EPIC_OK when the write started, EPIC_BUSY while one is running.
 */
EPIC_StatusTypeDef EPIC_EEPROM_WriteByte(uint8_t addr, uint8_t data)
{
    uint8_t gie;

    if ((epic_sfr_read8(PIC_REG_NVMCON1) & PIC_NVMCON1_WR) != 0U)
    {
        return EPIC_BUSY;
    }
    EPIC_REG8(PIC_REG_NVMADRL) = addr;
    EPIC_REG8(PIC_REG_NVMADRH) = EEPROM_ADRH;
    EPIC_REG8(PIC_REG_NVMDATL) = data;
    epic_sfr_write8(PIC_REG_NVMCON1,
        (uint8_t)(PIC_NVMCON1_NVMREGS | PIC_NVMCON1_WREN));
    EPIC_IRQ_ClearFlag(PIC16F188XX_IRQ_EEPROM);

    gie = EPIC_IRQ_Disable();
    epic_nvm_start_write();
    EPIC_IRQ_Restore(gie);
    return EPIC_OK;
}

/**
 * @brief Poll whether the write finished, and drop WREN once it has.
 * @details The WREN drop keeps WRERR: the write carries the flag back.
 * @return 1 when WR is clear, 0 while a write runs.
 */
uint8_t EPIC_EEPROM_IsWriteComplete(void)
{
    uint8_t con1 = epic_sfr_read8(PIC_REG_NVMCON1);

    if ((con1 & PIC_NVMCON1_WR) != 0U)
    {
        return 0U;
    }
    if ((con1 & PIC_NVMCON1_WREN) != 0U)
    {
        epic_sfr_write8(PIC_REG_NVMCON1, (uint8_t)(con1 & (uint8_t)~PIC_NVMCON1_WREN));
    }
    return 1U;
}

/**
 * @brief Report the NVMCON1 write error flag.
 * @return 1 when WRERR is set, 0 otherwise.
 */
uint8_t EPIC_EEPROM_HasWriteError(void)
{
    return (epic_sfr_read8(PIC_REG_NVMCON1) & PIC_NVMCON1_WRERR) ? 1U : 0U;
}

/**
 * @brief Acknowledge NVMIF, then run the write-complete callback if one is set.
 * @details Called from epic_dispatch_all_irqs only when NVMIF and NVMIE are
 * both set. Clears NVMIF before the callback so the vector does not
 * re-enter. No cstack autos: every access names its register literally.
 */
void EEPROM_IRQHandler(void)
{
    EPIC_IRQ_ClearFlag(PIC16F188XX_IRQ_EEPROM);
    if (s_eeprom_done_cb != NULL)
    {
        s_eeprom_done_cb();
    }
}
