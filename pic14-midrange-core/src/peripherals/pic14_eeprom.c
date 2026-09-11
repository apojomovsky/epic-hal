/* Shared PIC14 mid-range Data EEPROM implementation (87XA + 88X).
 * Sources: DS39582B §3 (87XA), DS40001291H §10 (88X); § numbers below
 * are the 87XA ones. */

#include "peripherals/pic14_eeprom.h"
#include "core/pic16_irq.h"

static void (*g_eeprom_cb)(void) = NULL;

/* EEPROM register placement is per family (DFP-verified): 87XA/88X keep
 * the data pair in Bank 2 and the control pair in Bank 3, while the
 * 628A keeps all four in Bank 1. The EPIC_BANKn_* accessors need a
 * literal SFR name at compile time, not a runtime addr, so the helpers
 * below dispatch on `addr` before any bank switch. */
#if PIC14MIDRANGE_HAS_EEPROM_BANK1
#define EE_ADDR_DATA  0x9AU
#define EE_ADDR_ADDR  0x9BU
#define EE_ADDR_CTRL  0x9CU
#define EE_ADDR_CTRL2 0x9DU
#define EE_DATA_BANK  1
#define EE_CTRL_BANK  1
#else
#define EE_ADDR_DATA  0x0CU
#define EE_ADDR_ADDR  0x0DU
#define EE_ADDR_CTRL  0x18CU
#define EE_ADDR_CTRL2 0x18DU
#define EE_DATA_BANK  2
#define EE_CTRL_BANK  3
#endif

/**
 * @brief Write a byte to an EEPROM control register (EECON1/EECON2).
 * Bank-2/3 parts use the EPIC_BANK3_* literal-token macros, Bank-1-only
 * parts (628A) the EPIC_BANK1_* ones; the addr dispatch below picks the
 * SFR token. Literal tokens are REQUIRED here: a runtime-address
 * EPIC_REG8 access misdirects under XC8 on banked parts (the 628A
 * b2 writes landed in Bank 0 while b3 appeared to land; see
 * pic16f628a-hal/MANUAL.md), so the variable-address fallback below is
 * host-sim only.
 * @param addr the register address.
 * @param v the byte to write.
 */
#if defined(EPIC_BANK3_WRITE8) || defined(EPIC_BANK1_WRITE8)
static void b3_write(uint16_t addr, uint8_t v)
{
#if PIC14MIDRANGE_HAS_EEPROM_BANK1
    if (addr == EE_ADDR_CTRL) EPIC_BANK1_WRITE8(EECON1, v);
    else                      EPIC_BANK1_WRITE8(EECON2, v);
#else
    if (addr == 0x18CU) EPIC_BANK3_WRITE8(EECON1, v);
    else                EPIC_BANK3_WRITE8(EECON2, v);
#endif
}

/**
 * @brief Read a byte from the EECON1 register.
 * @param addr the register address (only EECON1 is ever read).
 * @return the EECON1 value.
 */
static uint8_t b3_read(uint16_t addr)
{
    uint8_t v = 0U;
    (void)addr;   /* only EECON1 is ever read via b3_read. */
#if PIC14MIDRANGE_HAS_EEPROM_BANK1
    EPIC_BANK1_READ8(EECON1, v);
#else
    EPIC_BANK3_READ8(EECON1, v);
#endif
    return v;
}

/**
 * @brief Write a byte to an EEPROM data register (EEDATA or EEADR).
 * @param addr the register address.
 * @param v the byte to write.
 */
static void b2_write(uint16_t addr, uint8_t v)
{
#if PIC14MIDRANGE_HAS_EEPROM_BANK1
    if (addr == EE_ADDR_DATA) EPIC_BANK1_WRITE8(EEDATA, v);
    else                      EPIC_BANK1_WRITE8(EEADR, v);
#else
    if (addr == 0x0CU) EPIC_BANK2_WRITE8(EEDATA, v);
    else               EPIC_BANK2_WRITE8(EEADR, v);
#endif
}

/**
 * @brief Read a byte from an EEPROM data register (EEDATA or EEADR).
 * @param addr the register address.
 * @return the register value.
 */
static uint8_t b2_read(uint16_t addr)
{
    uint8_t v = 0U;
#if PIC14MIDRANGE_HAS_EEPROM_BANK1
    if (addr == EE_ADDR_DATA) EPIC_BANK1_READ8(EEDATA, v);
    else                      EPIC_BANK1_READ8(EEADR, v);
#else
    if (addr == 0x0CU) EPIC_BANK2_READ8(EEDATA, v);
    else               EPIC_BANK2_READ8(EEADR, v);
#endif
    return v;
}
#else
/**
 * @brief Write a byte to an EEPROM control register via bank-switched
 *        EPIC_REG8 access (HOST SIM ONLY: misdirects under XC8, which
 *        always takes the literal-token branch above).
 * @param addr the register address.
 * @param v the byte to write.
 */
static void b3_write(uint16_t addr, uint8_t v)
{
    uint8_t prev = (EPIC_REG8(PIC_REG_STATUS) >> 5) & 0x03U;
    pic_select_bank(EE_CTRL_BANK);
    EPIC_REG8(addr) = v;
    pic_select_bank(prev);
}

/**
 * @brief Read a byte from an EEPROM control register (HOST SIM ONLY).
 * @param addr the register address.
 * @return the register value.
 */
static uint8_t b3_read(uint16_t addr)
{
    uint8_t prev = (EPIC_REG8(PIC_REG_STATUS) >> 5) & 0x03U;
    pic_select_bank(EE_CTRL_BANK);
    uint8_t v = EPIC_REG8(addr);
    pic_select_bank(prev);
    return v;
}

/**
 * @brief Write a byte to an EEPROM data register (HOST SIM ONLY).
 * @param addr the register address.
 * @param v the byte to write.
 */
static void b2_write(uint16_t addr, uint8_t v)
{
    uint8_t prev = (EPIC_REG8(PIC_REG_STATUS) >> 5) & 0x03U;
    pic_select_bank(EE_DATA_BANK);
    EPIC_REG8(addr) = v;
    pic_select_bank(prev);
}

/**
 * @brief Read a byte from an EEPROM data register (HOST SIM ONLY).
 * @param addr the register address.
 * @return the register value.
 */
static uint8_t b2_read(uint16_t addr)
{
    uint8_t prev = (EPIC_REG8(PIC_REG_STATUS) >> 5) & 0x03U;
    pic_select_bank(EE_DATA_BANK);
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
    b2_write(EE_ADDR_ADDR, addr);             /* EEADR. */
    b3_write(EE_ADDR_CTRL, 0x00U);            /* EECON1 = 0, set RD. */
    b3_write(EE_ADDR_CTRL, 0x01U);            /* EECON1<RD>=1. */
#if !defined(__XC8) && !defined(__EPIC_CC__)
    /* Host sim backend: pull the byte from the simulated EEPROM
     * array (the flat-array sim has no RD strobe model). */
    /**
     * @brief Read a byte from the simulated EEPROM array (host only).
     * @param addr the EEPROM address to read.
     * @return the stored byte.
     */
    extern uint8_t pic14_sim_eeprom_read(uint8_t addr);
    b2_write(EE_ADDR_DATA, pic14_sim_eeprom_read(addr));
    return b2_read(EE_ADDR_DATA);
#else
    /* Real target: the RD strobe loads the addressed byte into EEDATA
     * (DS39582B §5.5). */
    return b2_read(EE_ADDR_DATA);
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
    if (b3_read(EE_ADDR_CTRL) & PIC_EECON1_WRERR) return EPIC_ERROR;

    b2_write(EE_ADDR_DATA, data);             /* EEDATA. */
    b2_write(EE_ADDR_ADDR, addr);             /* EEADR. */
    b3_write(EE_ADDR_CTRL, 0x00U);            /* clear WREN/WR. */
    b3_write(EE_ADDR_CTRL, 0x04U);            /* WREN=1. */
    /* Unlock sequence, §3.4 / Example 3-1. */
    b3_write(EE_ADDR_CTRL2, 0x55U);           /* EECON2 = 0x55. */
    b3_write(EE_ADDR_CTRL2, 0xAAU);           /* EECON2 = 0xAA. */
    b3_write(EE_ADDR_CTRL, PIC_EECON1_WREN | PIC_EECON1_WR);  /* start write. */
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
 * @return 1 if EEIF is set (PIR2<4>, PIR1<7> on Bank-1 parts).
 */
uint8_t EPIC_EEPROM_IsWriteComplete(void)
{
#if PIC14MIDRANGE_HAS_EE_PIR1
    /* EEIF lives in PIR1<7>. */
    return (EPIC_REG8(PIC_REG_PIR1) & PIC_PIR1_EEIF) ? 1U : 0U;
#else
    /* EEIF lives in PIR2<4>. */
    return (EPIC_REG8(0x0DU) & 0x10U) ? 1U : 0U;
#endif
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
     * context; see the CCP handlers). EEIF is PIR2 bit 4 (PIR1 bit 7
     * on Bank-1-EEPROM parts). */
#if PIC14MIDRANGE_HAS_EE_PIR1
    if (!(EPIC_REG8(PIC_REG_PIR1) & PIC_PIR1_EEIF)) return;
    EPIC_BIT_CLR(EPIC_REG8(PIC_REG_PIR1), PIC_PIR1_EEIF);
#else
    if (!(EPIC_REG8(PIC_REG_PIR2) & PIC_PIR2_EEIF)) return;
    EPIC_BIT_CLR(EPIC_REG8(PIC_REG_PIR2), PIC_PIR2_EEIF);
#endif
    if (g_eeprom_cb) g_eeprom_cb();
}
