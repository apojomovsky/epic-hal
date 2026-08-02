/**
 * @file    pic16_irq.c
 * @brief   Implementation of @ref pic16_irq.h.
 *
 * @details
 *   Each IRQ source maps to a specific bit in INTCON / PIE1 / PIE2
 *   (DS39582B §14.11 + Figure 14-10). The translation table lives at the
 *   top of this file, every other function is a thin wrapper.
 */

#include "core/pic16_irq.h"

/**
 * @brief Per-IRQ descriptor: which bank the enable / flag bit lives in,
 *        and at which position.
 *
 * PIC16F87XA interrupt layout:
 *   - RBIF, INTF, TMR0IF, plus their enable bits + GIE/PEIE → INTCON.
 *   - Peripheral flags → PIR1 (Bank 0) / PIR2 (Bank 0).
 *   - Peripheral enables → PIE1 (Bank 1) / PIE2 (Bank 1).
 *   - Bank 1 mirrors of PIR1/PIR2 are at 0x8C / 0x8D (DS39582B Figure 2-3).
 */
typedef struct {
    uint8_t flag_mask;     /**< PIR/INTCON bit to test/clear. */
    uint8_t enable_mask;   /**< PIE/INTCON bit to set/clear. */
    uint8_t in_intcon;     /**< 1 = INTCON, 0 = PIR1/PIR2. */
    uint8_t pir_is_pir2;   /**< 1 = PIR2, 0 = PIR1. (Ignored if in_intcon.) */
} irq_desc_t;

static const irq_desc_t irq_table[] = {
    [PIC16_IRQ_RB]       = { PIC_INTCON_RBIF,   PIC_INTCON_RBIE,   1, 0 },
    [PIC16_IRQ_INT]      = { PIC_INTCON_INTF,   PIC_INTCON_INTE,   1, 0 },
    [PIC16_IRQ_TMR0]     = { PIC_INTCON_TMR0IF, PIC_INTCON_TMR0IE, 1, 0 },
    [PIC16_IRQ_TMR1]     = { PIC_PIR1_TMR1IF,   PIC_PIE1_TMR1IE,   0, 0 },
    [PIC16_IRQ_TMR2]     = { PIC_PIR1_TMR2IF,   PIC_PIE1_TMR2IE,   0, 0 },
    [PIC16_IRQ_CCP1]     = { PIC_PIR1_CCP1IF,   PIC_PIE1_CCP1IE,   0, 0 },
    [PIC16_IRQ_CCP2]     = { PIC_PIR2_CCP2IF,   PIC_PIE2_CCP2IE,   0, 1 },
    [PIC16_IRQ_SSP]      = { PIC_PIR1_SSPIF,    PIC_PIE1_SSPIE,    0, 0 },
    [PIC16_IRQ_BCL]      = { PIC_PIR2_BCLIF,    PIC_PIE2_BCLIE,    0, 1 },
    [PIC16_IRQ_USART_TX] = { PIC_PIR1_TXIF,     PIC_PIE1_TXIE,     0, 0 },
    [PIC16_IRQ_USART_RX] = { PIC_PIR1_RCIF,     PIC_PIE1_RCIE,     0, 0 },
    [PIC16_IRQ_ADC]      = { PIC_PIR1_ADIF,     PIC_PIE1_ADIE,     0, 0 },
    [PIC16_IRQ_EEPROM]   = { PIC_PIR2_EEIF,     PIC_PIE2_EEIE,     0, 1 },
    [PIC16_IRQ_CMP]      = { PIC_PIR2_CMIF,     PIC_PIE2_CMIE,     0, 1 },
#if PIC16F87XA_FAMILY_HAS_PSP
    [PIC16_IRQ_PSP]      = { PIC_PIR1_PSPIF,    PIC_PIE1_PSPIE,    0, 0 },
#endif
};

#define IRQ_TABLE_SIZE  (sizeof irq_table / sizeof irq_table[0])

/* Macro, not a `static` function (empirically probed under MPLAB SIM,
 * same symptom as PIC8_PIE_ENABLE_BIT's own header comment: a
 * function-call boundary here silently lost the returned address by the
 * time it reached the caller's read/write, confirmed by a dedicated
 * probe that wrote PIE1 correctly via a hand-computed address but not
 * via this function; see pic16f87xa-hal/docs/ARCHITECTURE.md's Finding
 * 2 for the current, still-unconfirmed best explanation). PIR1 = 0x0C,
 * PIR2 = 0x0D. */
#define pir_reg_addr(d) ((d)->pir_is_pir2 ? PIC_REG_PIR2 : PIC_REG_PIR1)

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

void HAL_IRQ_Enable(PIC16_IRQn irq)
{
    if ((unsigned)irq >= IRQ_TABLE_SIZE) return;
    const irq_desc_t *d = &irq_table[irq];
    /* irq_table is `static const`, ROM-resident on PIC16's Harvard
     * architecture; XC8 reads each field through its own runtime helper
     * (visible in the generated .s as `fcall stringdir`), not a plain
     * load. Empirically probed under MPLAB SIM: interleaving that field
     * read with an in-progress SFR read-modify-write silently corrupted
     * the SFR side. Fix: pull every field this function needs out of
     * `d` into locals FIRST, before touching any SFR. */
    uint8_t in_intcon = d->in_intcon;
    uint8_t enable_mask = d->enable_mask;
    if (in_intcon) {
        PIC8_BIT_SET(PIC8_REG8(PIC_REG_INTCON), enable_mask);
        return;
    }
    /* Bank 1 (PIE1/PIE2). See PIC8_PIE_ENABLE_BIT's own header comment
     * (target/pic16f87xa_platform.h) for the full account: a plain C
     * read-modify-write here never persisted under XC8 v4.00, any
     * C-level local-variable access performed while banked into Bank 1
     * gets misdirected. Lives in the per-platform header, not inline
     * here, because this file is shared with the host build and
     * `asm()`/`__at()` are XC8-only syntax the host's gcc/clang cannot
     * parse. */
    PIC8_PIE_ENABLE_BIT(d->pir_is_pir2, enable_mask);
    /* Peripheral IRQs also need PEIE; auto-set it as a courtesy.
     * PIC_REG_INTCON is a compile-time constant and Bank 0-resident
     * (INTCON is unbanked), so the plain compound form is fine here. */
    PIC8_BIT_SET(PIC8_REG8(PIC_REG_INTCON), PIC_INTCON_PEIE);
}

void HAL_IRQ_DisableSrc(PIC16_IRQn irq)
{
    if ((unsigned)irq >= IRQ_TABLE_SIZE) return;
    const irq_desc_t *d = &irq_table[irq];
    uint8_t in_intcon = d->in_intcon;
    uint8_t enable_mask = d->enable_mask;
    if (in_intcon) {
        PIC8_BIT_CLR(PIC8_REG8(PIC_REG_INTCON), enable_mask);
        return;
    }
    /* Same fix as HAL_IRQ_Enable, see PIC8_PIE_DISABLE_BIT's header
     * comment (target/pic16f87xa_platform.h) for the full account. */
    PIC8_PIE_DISABLE_BIT(d->pir_is_pir2, enable_mask);
}

void HAL_IRQ_ClearFlag(PIC16_IRQn irq)
{
    if ((unsigned)irq >= IRQ_TABLE_SIZE) return;
    const irq_desc_t *d = &irq_table[irq];
    uint8_t in_intcon = d->in_intcon;
    uint8_t flag_mask = d->flag_mask;
    if (in_intcon) {
        PIC8_BIT_CLR(PIC8_REG8(PIC_REG_INTCON), flag_mask);
    } else {
        /* PIR1/PIR2 are Bank 0, so no pic_select_bank needed here. */
        uint8_t addr = pir_reg_addr(d);
        uint8_t v = PIC8_REG8(addr);
        v &= (uint8_t)~flag_mask;
        PIC8_REG8(addr) = v;
    }
}

uint8_t HAL_IRQ_GetFlag(PIC16_IRQn irq)
{
    if ((unsigned)irq >= IRQ_TABLE_SIZE) return 0U;
    const irq_desc_t *d = &irq_table[irq];
    uint8_t in_intcon = d->in_intcon;
    uint8_t flag_mask = d->flag_mask;
    uint8_t addr = pir_reg_addr(d);
    uint8_t reg = in_intcon ? PIC8_REG8(PIC_REG_INTCON) : PIC8_REG8(addr);
    return (reg & flag_mask) ? 1U : 0U;
}

void HAL_IRQ_SetPriority(PIC16_IRQn irq, HAL_IRQ_Priority prio)
{
    /* PIC16F87XA has a single interrupt vector, no priority scheme
     * (DS39582B §14.11). This is the no-op half of the shared
     * HAL_IRQ_SetPriority contract; PIC18's implementation writes the
     * matching IPR bit. Both arguments are intentionally unused. */
    (void)irq;
    (void)prio;
}
