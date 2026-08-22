/* Data EEPROM driver implementation (DS39582B §3.0). */

#include "peripherals/pic16f87xa_eeprom.h"
#include "core/pic16_irq.h"

static void (*g_eeprom_cb)(void) = NULL;

/* EEPROM registers live in Banks 2/3. EPIC_BANK2/3_* need a literal SFR
 * name at compile time (inline-asm operands), not a runtime addr, so
 * this dispatches on `addr` before any bank switch, then invokes the
 * named macro; every real call site passes a compile-time constant, so
 * XC8 folds this to the single matching branch either way. */

/**
 * @brief Write a byte to a Bank-3 EEPROM register (EECON1 or EECON2).
 * @param addr the register address (0x18C or 0x18D).
 * @param v the byte to write.
 */
#ifdef EPIC_BANK3_WRITE8
static void b3_write(uint16_t addr, uint8_t v)
{
    if (addr == 0x18CU) EPIC_BANK3_WRITE8(EECON1, v);
    else                EPIC_BANK3_WRITE8(EECON2, v);
}

/**
 * @brief Read a byte from the Bank-3 EECON1 register.
 * @param addr the register address (only EECON1 is ever read).
 * @return the EECON1 value.
 */
static uint8_t b3_read(uint16_t addr)
{
    uint8_t v = 0U;
    (void)addr;   /* only EECON1 is ever read via b3_read. */
    EPIC_BANK3_READ8(EECON1, v);
    return v;
}

/**
 * @brief Write a byte to a Bank-2 EEPROM register (EEDATA or EEADR).
 * @param addr the register address (0x0C or 0x0D).
 * @param v the byte to write.
 */
static void b2_write(uint16_t addr, uint8_t v)
{
    if (addr == 0x0CU) EPIC_BANK2_WRITE8(EEDATA, v);
    else               EPIC_BANK2_WRITE8(EEADR, v);
}

/**
 * @brief Read a byte from a Bank-2 EEPROM register (EEDATA or EEADR).
 * @param addr the register address (0x0C or 0x0D).
 * @return the register value.
 */
static uint8_t b2_read(uint16_t addr)
{
    uint8_t v = 0U;
    if (addr == 0x0CU) EPIC_BANK2_READ8(EEDATA, v);
    else               EPIC_BANK2_READ8(EEADR, v);
    return v;
}
#else
/**
 * @brief Write a byte to a Bank-3 EEPROM register (EECON1 or EECON2)
 *        via bank-switched EPIC_REG8 access.
 * @param addr the register address (0x18C or 0x18D).
 * @param v the byte to write.
 */
static void b3_write(uint16_t addr, uint8_t v)
{
    uint8_t prev = (EPIC_REG8(PIC_REG_STATUS) >> 5) & 0x03U;
    pic_select_bank(3);
    EPIC_REG8(addr) = v;
    pic_select_bank(prev);
}

/**
 * @brief Read a byte from a Bank-3 EEPROM register.
 * @param addr the register address.
 * @return the register value.
 */
static uint8_t b3_read(uint16_t addr)
{
    uint8_t prev = (EPIC_REG8(PIC_REG_STATUS) >> 5) & 0x03U;
    pic_select_bank(3);
    uint8_t v = EPIC_REG8(addr);
    pic_select_bank(prev);
    return v;
}

/**
 * @brief Write a byte to a Bank-2 EEPROM register (EEDATA or EEADR)
 *        via bank-switched EPIC_REG8 access.
 * @param addr the register address (0x0C or 0x0D).
 * @param v the byte to write.
 */
static void b2_write(uint16_t addr, uint8_t v)
{
    uint8_t prev = (EPIC_REG8(PIC_REG_STATUS) >> 5) & 0x03U;
    pic_select_bank(2);
    EPIC_REG8(addr) = v;
    pic_select_bank(prev);
}

/**
 * @brief Read a byte from a Bank-2 EEPROM register (EEDATA or EEADR).
 * @param addr the register address (0x0C or 0x0D).
 * @return the register value.
 */
static uint8_t b2_read(uint16_t addr)
{
    uint8_t prev = (EPIC_REG8(PIC_REG_STATUS) >> 5) & 0x03U;
    pic_select_bank(2);
    uint8_t v = EPIC_REG8(addr);
    pic_select_bank(prev);
    return v;
}
#endif

/**
 * @brief Initialize the EEPROM driver and optionally arm the
 *        write-complete interrupt.
 * @param callback function called on EEPROM write completion, or NULL
 *        for polling mode.
 * @return EPIC_OK on success.
 */
EPIC_StatusTypeDef EPIC_EEPROM_Init(void (*callback)(void))
{
    g_eeprom_cb = callback;
    EPIC_IRQ_ClearFlag(PIC16_IRQ_EEPROM);
    if (callback) EPIC_IRQ_Enable(PIC16_IRQ_EEPROM);
    else          EPIC_IRQ_DisableSrc(PIC16_IRQ_EEPROM);
    return EPIC_OK;
}

/**
 * @brief De-initialize the EEPROM driver: disable the interrupt and
 *        clear the callback.
 * @return EPIC_OK on success.
 */
EPIC_StatusTypeDef EPIC_EEPROM_DeInit(void)
{
    EPIC_IRQ_DisableSrc(PIC16_IRQ_EEPROM);
    EPIC_IRQ_ClearFlag(PIC16_IRQ_EEPROM);
    g_eeprom_cb = NULL;
    return EPIC_OK;
}

/**
 * @brief Read one byte from data EEPROM (loads EEADR, strobes RD).
 * @param addr the EEPROM address, 0..255.
 * @return the byte stored at `addr`.
 */
uint8_t EPIC_EEPROM_ReadByte(uint8_t addr)
{
    b2_write(0x0DU, addr);                  /* EEADR. */
    b3_write(0x18CU, 0x00U);                /* EECON1 = 0, set RD. */
    b3_write(0x18CU, 0x01U);                /* EECON1<RD>=1. */
#if !defined(__XC8)
    /* Host sim backend: pull the byte from the simulated EEPROM
     * array (the flat-array sim has no RD strobe model). */
    /**
     * @brief Read a byte from the simulated EEPROM array (host only).
     * @param addr the EEPROM address to read.
     * @return the stored byte.
     */
    extern uint8_t pic16f87xa_sim_eeprom_read(uint8_t addr);
    b2_write(0x0CU, pic16f87xa_sim_eeprom_read(addr));
    return b2_read(0x0CU);
#else
    /* Real target: the RD strobe loads the addressed byte into EEDATA
     * (DS39582B §5.5). */
    return b2_read(0x0CU);
#endif
}

/**
 * @brief Write one byte to data EEPROM with the 0x55/0xAA unlock
 *        sequence. Non-blocking: completion is signalled by EEIF.
 * @param addr the EEPROM address, 0..255.
 * @param data the byte to store.
 * @return EPIC_OK on success, EPIC_ERROR if a previous write was
 *         aborted (WRERR set).
 */
EPIC_StatusTypeDef EPIC_EEPROM_WriteByte(uint8_t addr, uint8_t data)
{
    /* §3.4: check WRERR before starting. */
    if (b3_read(0x18CU) & PIC_EECON1_WRERR) return EPIC_ERROR;

    b2_write(0x0CU, data);                  /* EEDATA. */
    b2_write(0x0DU, addr);                  /* EEADR. */
    b3_write(0x18CU, 0x00U);                /* clear WREN/WR. */
    b3_write(0x18CU, 0x04U);                /* WREN=1. */
    /* Unlock sequence, §3.4 / Example 3-1. */
    b3_write(0x18DU, 0x55U);                /* EECON2 = 0x55. */
    b3_write(0x18DU, 0xAAU);                /* EECON2 = 0xAA. */
    b3_write(0x18CU, PIC_EECON1_WREN | PIC_EECON1_WR);  /* start write. */
    /* WR is held for the write cycle (DS39582B §3.4). On real
     * hardware the CPU sees it clear when the cycle completes; the
     * sim backend mirrors that in sim_step(). The caller polls EEIF
     * (PIR2<4>) to detect completion. */
    return EPIC_OK;
}

/**
 * @brief Read a contiguous block of EEPROM bytes.
 * @param start the first address to read.
 * @param buf where the bytes are written.
 * @param len the number of bytes to read.
 */
void EPIC_EEPROM_ReadBuffer(uint8_t start, uint8_t *buf, uint8_t len)
{
    for (uint8_t i = 0; i < len; i++) {
        buf[i] = EPIC_EEPROM_ReadByte((uint8_t)(start + i));
    }
}

/**
 * @brief Write a contiguous block of EEPROM bytes.
 * @param start the first address to write.
 * @param buf the bytes to store.
 * @param len the number of bytes to write.
 * @return EPIC_OK on success, or the first non-OK write status.
 */
EPIC_StatusTypeDef EPIC_EEPROM_WriteBuffer(uint8_t start,
                                                const uint8_t *buf,
                                                uint8_t len)
{
    EPIC_StatusTypeDef st;
    for (uint8_t i = 0; i < len; i++) {
        st = EPIC_EEPROM_WriteByte((uint8_t)(start + i), buf[i]);
        if (st != EPIC_OK) return st;
    }
    return EPIC_OK;
}

/**
 * @brief Report whether the EEPROM write completed.
 * @return 1 if EEIF (PIR2<4>) is set, 0 otherwise.
 */
uint8_t EPIC_EEPROM_IsWriteComplete(void)
{
    /* EEIF lives in PIR2<4>. */
    return (EPIC_REG8(0x0DU) & 0x10U) ? 1U : 0U;
}

/**
 * @brief Clear the EEPROM write-complete flag.
 */
void EPIC_EEPROM_ClearITFlag(void)
{
    EPIC_IRQ_ClearFlag(PIC16_IRQ_EEPROM);
}

/**
 * @brief Weak EEPROM ISR: clears EEIF and fires the write-complete
 *        callback.
 */
void EEPROM_IRQHandler(void)
{
    /* Direct flag ops (class-F: the table route clobbers PCLATH in ISR
     * context; see the CCP handlers). EEIF is PIR2 bit 4. */
    if (!(EPIC_REG8(PIC_REG_PIR2) & PIC_PIR2_EEIF)) return;
    EPIC_BIT_CLR(EPIC_REG8(PIC_REG_PIR2), PIC_PIR2_EEIF);
#ifndef EPIC_AT
    if (g_eeprom_cb) g_eeprom_cb();
#else
    (void)g_eeprom_cb;
#endif
}
