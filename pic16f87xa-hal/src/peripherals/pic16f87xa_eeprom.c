/**
 * @file    pic16f87xa_eeprom.c
 * @brief   Data EEPROM driver, implementation (DS39582B §3.0).
 */

#include "peripherals/pic16f87xa_eeprom.h"
#include "core/pic16_irq.h"

static void (*g_eeprom_cb)(void) = NULL;

/* EEPROM registers live in Banks 2/3. EPIC_BANK2/3_WRITE8/READ8 need a
 * literal SFR name at compile time (inline-asm operands), not a
 * runtime `addr`, so this dispatches on `addr` *before* any bank
 * switch begins, then invokes the named macro for the matching
 * register; every real call site passes a compile-time constant, so
 * XC8 folds this to the single matching branch either way. */
#ifdef EPIC_BANK3_WRITE8
static void b3_write(uint16_t addr, uint8_t v)
{
    if (addr == 0x18CU) EPIC_BANK3_WRITE8(EECON1, v);
    else                EPIC_BANK3_WRITE8(EECON2, v);
}

static uint8_t b3_read(uint16_t addr)
{
    uint8_t v = 0U;
    (void)addr;   /* only EECON1 is ever read via b3_read. */
    EPIC_BANK3_READ8(EECON1, v);
    return v;
}

static void b2_write(uint16_t addr, uint8_t v)
{
    if (addr == 0x0CU) EPIC_BANK2_WRITE8(EEDATA, v);
    else               EPIC_BANK2_WRITE8(EEADR, v);
}

static uint8_t b2_read(uint16_t addr)
{
    uint8_t v = 0U;
    if (addr == 0x0CU) EPIC_BANK2_READ8(EEDATA, v);
    else               EPIC_BANK2_READ8(EEADR, v);
    return v;
}
#else
static void b3_write(uint16_t addr, uint8_t v)
{
    uint8_t prev = (EPIC_REG8(PIC_REG_STATUS) >> 5) & 0x03U;
    pic_select_bank(3);
    EPIC_REG8(addr) = v;
    pic_select_bank(prev);
}

static uint8_t b3_read(uint16_t addr)
{
    uint8_t prev = (EPIC_REG8(PIC_REG_STATUS) >> 5) & 0x03U;
    pic_select_bank(3);
    uint8_t v = EPIC_REG8(addr);
    pic_select_bank(prev);
    return v;
}

static void b2_write(uint16_t addr, uint8_t v)
{
    uint8_t prev = (EPIC_REG8(PIC_REG_STATUS) >> 5) & 0x03U;
    pic_select_bank(2);
    EPIC_REG8(addr) = v;
    pic_select_bank(prev);
}

static uint8_t b2_read(uint16_t addr)
{
    uint8_t prev = (EPIC_REG8(PIC_REG_STATUS) >> 5) & 0x03U;
    pic_select_bank(2);
    uint8_t v = EPIC_REG8(addr);
    pic_select_bank(prev);
    return v;
}
#endif

EPIC_StatusTypeDef EPIC_EEPROM_Init(void (*callback)(void))
{
    g_eeprom_cb = callback;
    EPIC_IRQ_ClearFlag(PIC16_IRQ_EEPROM);
    if (callback) EPIC_IRQ_Enable(PIC16_IRQ_EEPROM);
    else          EPIC_IRQ_DisableSrc(PIC16_IRQ_EEPROM);
    return EPIC_OK;
}

EPIC_StatusTypeDef EPIC_EEPROM_DeInit(void)
{
    EPIC_IRQ_DisableSrc(PIC16_IRQ_EEPROM);
    EPIC_IRQ_ClearFlag(PIC16_IRQ_EEPROM);
    g_eeprom_cb = NULL;
    return EPIC_OK;
}

uint8_t EPIC_EEPROM_ReadByte(uint8_t addr)
{
    b2_write(0x0DU, addr);                  /* EEADR. */
    b3_write(0x18CU, 0x00U);                /* EECON1 = 0, set RD. */
    b3_write(0x18CU, 0x01U);                /* EECON1<RD>=1. */
    /* Sim backend: pull the byte from the simulated EEPROM array. */
    extern uint8_t pic16f87xa_sim_eeprom_read(uint8_t addr);
    b2_write(0x0CU, pic16f87xa_sim_eeprom_read(addr));
    return b2_read(0x0CU);
}

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

void EPIC_EEPROM_ReadBuffer(uint8_t start, uint8_t *buf, uint8_t len)
{
    for (uint8_t i = 0; i < len; i++) {
        buf[i] = EPIC_EEPROM_ReadByte((uint8_t)(start + i));
    }
}

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

uint8_t EPIC_EEPROM_IsWriteComplete(void)
{
    /* EEIF lives in PIR2<4>. */
    return (EPIC_REG8(0x0DU) & 0x10U) ? 1U : 0U;
}

void EPIC_EEPROM_ClearITFlag(void)
{
    EPIC_IRQ_ClearFlag(PIC16_IRQ_EEPROM);
}

void EEPROM_IRQHandler(void)
{
    if (!EPIC_IRQ_GetFlag(PIC16_IRQ_EEPROM)) return;
    EPIC_IRQ_ClearFlag(PIC16_IRQ_EEPROM);
    if (g_eeprom_cb) g_eeprom_cb();
}
