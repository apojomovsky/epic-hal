/**
 * @file    pic16f193x_irq.c
 * @brief   Implementation of @ref pic16f193x_irq.h.
 *
 * @details
 *   Each IRQ source maps to a specific bit in INTCON / PIE1 / PIE2 / PIE3
 *   (DS41364B §4.0 + Figure 4-1/4-2). The translation table lives at the
 *   top of this file, every other function is a thin wrapper. The
 *   descriptor extends the classic PIC16 two-bank layout to three (PIR1/2/3,
 *   PIE1/2/3): `pir_index` selects the bank (0/1/2), `in_intcon` selects the
 *   INTCON-level sources (IOC/INT/TMR0).
 */

#include "core/pic16f193x_irq.h"

/**
 * @brief Per-IRQ descriptor: which bank the enable / flag bit lives in,
 *        and at which position.
 *
 * PIC16F193X interrupt layout (DS41364B §4.0):
 *   - IOCIF, INTF, TMR0IF, plus their enable bits + GIE/PEIE -> INTCON.
 *   - Peripheral flags -> PIR1 (0x11) / PIR2 (0x12) / PIR3 (0x13).
 *   - Peripheral enables -> PIE1 (0x91) / PIE2 (0x92) / PIE3 (0x93), bank 1.
 */
typedef struct {
    uint8_t flag_mask;     /**< PIR/INTCON bit to test/clear. */
    uint8_t enable_mask;   /**< PIE/INTCON bit to set/clear. */
    uint8_t in_intcon;     /**< 1 = INTCON, 0 = PIR/PIE. */
    uint8_t pir_index;     /**< 0=PIR1/PIE1, 1=PIR2/PIE2, 2=PIR3/PIE3. */
} irq_desc_t;

static const irq_desc_t irq_table[] = {
    [PIC16F193X_IRQ_IOC]      = { PIC_INTCON_IOCIF,  PIC_INTCON_IOCIE,  1, 0 },
    [PIC16F193X_IRQ_INT]     = { PIC_INTCON_INTF,   PIC_INTCON_INTE,   1, 0 },
    [PIC16F193X_IRQ_TMR0]    = { PIC_INTCON_TMR0IF, PIC_INTCON_TMR0IE, 1, 0 },
    [PIC16F193X_IRQ_TMR1]    = { PIC_PIR1_TMR1IF,   PIC_PIE1_TMR1IE,   0, 0 },
    [PIC16F193X_IRQ_TMR2]    = { PIC_PIR1_TMR2IF,   PIC_PIE1_TMR2IE,   0, 0 },
    [PIC16F193X_IRQ_CCP1]    = { PIC_PIR1_CCP1IF,   PIC_PIE1_CCP1IE,   0, 0 },
    [PIC16F193X_IRQ_SSP]     = { PIC_PIR1_SSPIF,    PIC_PIE1_SSPIE,    0, 0 },
    [PIC16F193X_IRQ_USART_TX] = { PIC_PIR1_TXIF,    PIC_PIE1_TXIE,     0, 0 },
    [PIC16F193X_IRQ_USART_RX] = { PIC_PIR1_RCIF,    PIC_PIE1_RCIE,     0, 0 },
    [PIC16F193X_IRQ_ADC]     = { PIC_PIR1_ADIF,     PIC_PIE1_ADIE,     0, 0 },
    [PIC16F193X_IRQ_TMR1G]   = { PIC_PIR1_TMR1GIF,  PIC_PIE1_TMR1GIE,  0, 0 },
    [PIC16F193X_IRQ_CCP2]    = { PIC_PIR2_CCP2IF,   PIC_PIE2_CCP2IE,   0, 1 },
    [PIC16F193X_IRQ_LCD]     = { PIC_PIR2_LCDIF,    PIC_PIE2_LCDIE,    0, 1 },
    [PIC16F193X_IRQ_BCL]     = { PIC_PIR2_BCLIF,    PIC_PIE2_BCLIE,    0, 1 },
    [PIC16F193X_IRQ_EEPROM]  = { PIC_PIR2_EEIF,     PIC_PIE2_EEIE,     0, 1 },
    [PIC16F193X_IRQ_CMP1]    = { PIC_PIR2_C1IF,     PIC_PIE2_C1IE,     0, 1 },
    [PIC16F193X_IRQ_CMP2]    = { PIC_PIR2_C2IF,     PIC_PIE2_C2IE,     0, 1 },
    [PIC16F193X_IRQ_OSF]     = { PIC_PIR2_OSFIF,    PIC_PIE2_OSFIE,    0, 1 },
    [PIC16F193X_IRQ_TMR4]    = { PIC_PIR3_TMR4IF,   PIC_PIE3_TMR4IE,   0, 2 },
    [PIC16F193X_IRQ_TMR6]    = { PIC_PIR3_TMR6IF,   PIC_PIE3_TMR6IE,   0, 2 },
    [PIC16F193X_IRQ_CCP3]    = { PIC_PIR3_CCP3IF,   PIC_PIE3_CCP3IE,   0, 2 },
    [PIC16F193X_IRQ_CCP4]    = { PIC_PIR3_CCP4IF,   PIC_PIE3_CCP4IE,   0, 2 },
    [PIC16F193X_IRQ_CCP5]    = { PIC_PIR3_CCP5IF,   PIC_PIE3_CCP5IE,   0, 2 },
};

#define IRQ_TABLE_SIZE  (sizeof irq_table / sizeof irq_table[0])

/* PIR1 = 0x11, PIR2 = 0x12, PIR3 = 0x13 (DS41364B Table 2-4). */
#define pir_reg_addr(d) \
    ((d)->pir_index == 0 ? PIC_REG_PIR1 : \
     (d)->pir_index == 1 ? PIC_REG_PIR2 : PIC_REG_PIR3)

/* ───────────────────────── public API ───────────────────────────── */

uint8_t HAL_IRQ_Disable(void)
{
    uint8_t s = PIC8_REG8(PIC_REG_INTCON);
    uint8_t prev = (s & PIC_INTCON_GIE) ? 1U : 0U;
    PIC8_REG8(PIC_REG_INTCON) = s & (uint8_t)~PIC_INTCON_GIE;
    return prev;
}

void HAL_IRQ_Restore(uint8_t prev_state)
{
    if (prev_state) PIC8_BIT_SET(PIC8_REG8(PIC_REG_INTCON), PIC_INTCON_GIE);
    else            PIC8_BIT_CLR(PIC8_REG8(PIC_REG_INTCON), PIC_INTCON_GIE);
}

void HAL_IRQ_Enable(PIC16F193X_IRQn irq)
{
    if ((unsigned)irq >= IRQ_TABLE_SIZE) return;
    const irq_desc_t *d = &irq_table[irq];
    /* Pull every field this function needs into locals before touching
     * any SFR: the classic-PIC16 build interleaved a ROM-table field
     * read with an in-progress SFR RMW and corrupted the SFR side
     * (pic16f87xa-hal/docs/ARCHITECTURE.md Finding 2). Same defensive
     * shape here until the §4 codegen probe clears this core. */
    uint8_t in_intcon   = d->in_intcon;
    uint8_t enable_mask = d->enable_mask;
    uint8_t pir_index   = d->pir_index;
    if (in_intcon) {
        PIC8_BIT_SET(PIC8_REG8(PIC_REG_INTCON), enable_mask);
        return;
    }
    /* PIE1/2/3 are bank 1; the per-platform macro handles the access. */
    PIC8_PIE_ENABLE_BIT(pir_index, enable_mask);
    /* Peripheral IRQs also need PEIE; auto-set it as a courtesy. */
    PIC8_BIT_SET(PIC8_REG8(PIC_REG_INTCON), PIC_INTCON_PEIE);
}

void HAL_IRQ_DisableSrc(PIC16F193X_IRQn irq)
{
    if ((unsigned)irq >= IRQ_TABLE_SIZE) return;
    const irq_desc_t *d = &irq_table[irq];
    uint8_t in_intcon   = d->in_intcon;
    uint8_t enable_mask = d->enable_mask;
    uint8_t pir_index   = d->pir_index;
    if (in_intcon) {
        PIC8_BIT_CLR(PIC8_REG8(PIC_REG_INTCON), enable_mask);
        return;
    }
    PIC8_PIE_DISABLE_BIT(pir_index, enable_mask);
}

void HAL_IRQ_ClearFlag(PIC16F193X_IRQn irq)
{
    if ((unsigned)irq >= IRQ_TABLE_SIZE) return;
    const irq_desc_t *d = &irq_table[irq];
    uint8_t in_intcon = d->in_intcon;
    uint8_t flag_mask = d->flag_mask;
    if (in_intcon) {
        PIC8_BIT_CLR(PIC8_REG8(PIC_REG_INTCON), flag_mask);
    } else {
        uint8_t addr = pir_reg_addr(d);
        uint8_t v = PIC8_REG8(addr);
        v &= (uint8_t)~flag_mask;
        PIC8_REG8(addr) = v;
    }
}

uint8_t HAL_IRQ_GetFlag(PIC16F193X_IRQn irq)
{
    if ((unsigned)irq >= IRQ_TABLE_SIZE) return 0U;
    const irq_desc_t *d = &irq_table[irq];
    uint8_t in_intcon = d->in_intcon;
    uint8_t flag_mask = d->flag_mask;
    uint8_t reg = in_intcon ? PIC8_REG8(PIC_REG_INTCON)
                            : PIC8_REG8(pir_reg_addr(d));
    return (reg & flag_mask) ? 1U : 0U;
}

void HAL_IRQ_SetPriority(PIC16F193X_IRQn irq, HAL_IRQ_Priority prio)
{
    /* PIC16F193X has a single interrupt vector, no priority scheme
     * (DS41364B §4.0). This is the no-op half of the shared
     * HAL_IRQ_SetPriority contract; PIC18's implementation writes the
     * matching IPR bit. Both arguments are intentionally unused. */
    (void)irq;
    (void)prio;
}
