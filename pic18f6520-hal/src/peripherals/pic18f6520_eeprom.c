/*
 * Data EEPROM driver, implementation (DS39609B §7.0). Registers are
 * Access Bank (0xFA6-0xFA9), no bank switching; addresses are 10-bit
 * (EEADRH:EEADR, 1024 bytes), unlike the 4550/2520 families' 8-bit
 * EEADR-only. `ReadByte` sets RD then pulls the byte via the sim read
 * hook on host; real target reads EEDATA after RD instead.
 */

#include "peripherals/pic18f6520_eeprom.h"
#include "core/pic18_irq.h"

static void (*g_eeprom_cb)(void) = NULL;

/**
 * @brief  Initialize the data EEPROM driver: store the write-complete
 *         callback, clear EEIF and enable the EEPROM interrupt if a
 *         callback is given.
 * @param callback Function invoked from EEPROM_IRQHandler, or NULL.
 * @return EPIC_OK.
 */
EPIC_StatusTypeDef EPIC_EEPROM_Init(void (*callback)(void))
{
    g_eeprom_cb = callback;
    EPIC_IRQ_ClearFlag(PIC18_IRQ_EEPROM);
    if (callback) EPIC_IRQ_Enable(PIC18_IRQ_EEPROM);
    else          EPIC_IRQ_DisableSrc(PIC18_IRQ_EEPROM);
    return EPIC_OK;
}

/**
 * @brief  Disable the EEPROM module: clear EEIF, disable its interrupt,
 *         restore EECON1 to 0x00 and drop the stored callback.
 * @return EPIC_OK.
 */
EPIC_StatusTypeDef EPIC_EEPROM_DeInit(void)
{
    EPIC_IRQ_DisableSrc(PIC18_IRQ_EEPROM);
    EPIC_IRQ_ClearFlag(PIC18_IRQ_EEPROM);
    epic_sfr_write8(PIC_REG_EECON1, PIC_EECON1_POR_VALUE);   /* 0x00. */
    g_eeprom_cb = NULL;
    return EPIC_OK;
}

/**
 * @brief  Read one byte from data EEPROM. Loads EEADRH/EEADR with
 *         `addr`, sets EECON1<RD> (with EEPGD = 0), and returns the
 *         byte from EEDATA. Per §7.1: the address must be loaded first,
 *         then RD.
 * @param addr Data EEPROM address to read (0..1023).
 * @return The byte stored at `addr`.
 */
uint8_t EPIC_EEPROM_ReadByte(uint16_t addr)
{
    /* §7.1: load EEADRH/EEADR, ensure EEPGD=0/CFGS=0, then strobe RD. */
    epic_sfr_write8(PIC_REG_EEADRH, (uint8_t)((addr >> 8) & 0x03U));
    epic_sfr_write8(PIC_REG_EEADR,  (uint8_t)(addr & 0xFFU));
    epic_sfr_write8(PIC_REG_EECON1, 0x00U);                /* clear, EEPGD=0. */
    epic_sfr_write8(PIC_REG_EECON1, PIC_EECON1_RD);       /* RD = 1. */
#if !defined(__XC8) && !defined(__EPIC_CC__)
    /**
     * @brief  Host sim backend: pull the byte from the simulated EEPROM
     *         array (the flat-array sim has no RD strobe model).
     * @param addr EEPROM address to read.
     * @return The byte stored at `addr` in the simulated EEPROM.
     */
    extern uint8_t pic18_sim_eeprom_read(uint16_t addr);
    uint8_t data = pic18_sim_eeprom_read(addr);
    epic_sfr_write8(PIC_REG_EEDATA, data);
    return data;
#else
    /* Real target: the RD strobe loads the addressed byte into
     * EEDATA (DS39609B §7.1). */
    return epic_sfr_read8(PIC_REG_EEDATA);
#endif
}

/**
 * @brief  Write one byte to data EEPROM at `addr`. Performs the mandatory
 *         unlock sequence (0x55 -> 0xAA -> WR), with EEPGD = 0. The WR
 *         bit self-clears when the cycle completes; poll IsWriteComplete.
 * @param addr Data EEPROM address to write (0..1023).
 * @param data Byte value to store.
 * @return EPIC_OK on success, EPIC_ERROR if a previous write was aborted
 *         (WRERR set).
 */
EPIC_StatusTypeDef EPIC_EEPROM_WriteByte(uint16_t addr, uint8_t data)
{
    /* §7.2: check WRERR before starting. */
    if (epic_sfr_read8(PIC_REG_EECON1) & PIC_EECON1_WRERR) return EPIC_ERROR;

    epic_sfr_write8(PIC_REG_EEDATA, data);
    epic_sfr_write8(PIC_REG_EEADRH, (uint8_t)((addr >> 8) & 0x03U));
    epic_sfr_write8(PIC_REG_EEADR,  (uint8_t)(addr & 0xFFU));
    epic_sfr_write8(PIC_REG_EECON1, 0x00U);                       /* clear, EEPGD=0. */
    epic_sfr_write8(PIC_REG_EECON1, PIC_EECON1_WREN);            /* WREN = 1. */
    /* Unlock sequence, §7.2: 0x55 then 0xAA to EECON2. */
    epic_sfr_write8(PIC_REG_EECON2, 0x55U);
    epic_sfr_write8(PIC_REG_EECON2, 0xAAU);
    epic_sfr_write8(PIC_REG_EECON1, PIC_EECON1_WREN | PIC_EECON1_WR);  /* start write. */
    /* WR is held for the write cycle. On real hardware it self-clears when
     * the cycle completes; the sim backend mirrors that in
     * pic18_sim_drive_eeprom_done(). The caller polls EEIF (PIR2<4>). */
    return EPIC_OK;
}

/**
 * @brief  Read a contiguous block of bytes from data EEPROM.
 * @param start First EEPROM address to read.
 * @param buf Destination buffer, at least `len` bytes.
 * @param len Number of bytes to read.
 */
void EPIC_EEPROM_ReadBuffer(uint16_t start, uint8_t *buf, uint16_t len)
{
    for (uint16_t i = 0; i < len; i++)
    {
        buf[i] = EPIC_EEPROM_ReadByte((uint16_t)(start + i));
    }
}

/**
 * @brief  Write a contiguous block of bytes to data EEPROM.
 * @param start First EEPROM address to write.
 * @param buf Source buffer, at least `len` bytes.
 * @param len Number of bytes to write.
 * @return EPIC_OK on success, or the first EPIC_ERROR returned by a
 *         single-byte write.
 */
EPIC_StatusTypeDef EPIC_EEPROM_WriteBuffer(uint16_t start,
                                           const uint8_t *buf,
                                           uint16_t len)
{
    EPIC_StatusTypeDef st;
    for (uint16_t i = 0; i < len; i++)
    {
        st = EPIC_EEPROM_WriteByte((uint16_t)(start + i), buf[i]);
        if (st != EPIC_OK) return st;
    }
    return EPIC_OK;
}

/**
 * @brief  Return 1 if EEIF is set, i.e. the write cycle has completed.
 * @return 1 if the last write finished, else 0.
 */
uint8_t EPIC_EEPROM_IsWriteComplete(void)
{
    return (epic_sfr_read8(PIC_REG_PIR2) & PIC_PIR2_EEIF) ? 1U : 0U;
}

/**
 * @brief  Clear EEIF; must be cleared in the user's IRQ handler.
 */
void EPIC_EEPROM_ClearITFlag(void)
{
    EPIC_IRQ_ClearFlag(PIC18_IRQ_EEPROM);
}

/**
 * @brief  Weak EEPROM interrupt handler: clears EEIF and invokes the
 *         write-complete callback registered via Init.
 */
void EEPROM_IRQHandler(void)
{
    if (!EPIC_IRQ_GetFlag(PIC18_IRQ_EEPROM)) return;
    EPIC_IRQ_ClearFlag(PIC18_IRQ_EEPROM);
    if (g_eeprom_cb) g_eeprom_cb();
}
