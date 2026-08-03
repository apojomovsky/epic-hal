/**
 * @file    pic18_irq.c
 * @brief   Implementation of @ref pic18_irq.h.
 *
 * @details
 *   Each IRQ source maps to a flag bit, an enable bit, and (except INT0)
 *   a priority bit, spread across INTCON / INTCON2 / INTCON3 / PIR1 /
 *   PIE1 / IPR1 (DS39632E §9.0, Register 9-1/9-2/9-3/9-5/9-6/9-8).
 *
 *   The master enable is GIEH (INTCON<7>) and, in priority mode, GIEL
 *   (INTCON<6>). HAL_IRQ_Disable/Restore treat them as a single "interrupts
 *   on/off" switch so the API is the drop-in equivalent of PIC16's GIE.
 *   Restoring to "on" also sets IPEN (RCON<7>) to activate the two-vector
 *   priority scheme.
 *
 *   No runtime-addressed SFR access anywhere in this file, deliberately.
 *   An earlier version held a `pic18_irq_desc_t` lookup table (flag/enable/
 *   priority register addresses as `uint16_t` fields) and dispatched through
 *   two small helpers, `sfr_set(addr, mask)` / `sfr_clr(addr, mask)`, that
 *   took the register address as a runtime function parameter. That
 *   compiled, linked, and looked correct, but silently did nothing on real
 *   hardware and under MPLAB SIM: confirmed via `mdb` that `HAL_IRQ_Restore`
 *   never actually set `GIEH`/`GIEL`/`IPEN`, tracing all the way to the
 *   generated assembly for `sfr_set`, which used `movff addr,tblptrl` /
 *   `tblrd *` / `tblwt *`, PIC18's *program-memory* (flash) table
 *   read/write mechanism, not a data-memory SFR access at all. XC8's
 *   pointer classification (User's Guide §5.3.6.3) apparently can't prove a
 *   runtime `uint16_t` cast through a generic pointer targets data memory
 *   only, and defaults to the mixed-target-space representation, which for
 *   PIC18 means routing through `TBLPTR`/`TABLAT`. A `tblwt` with no
 *   accompanying NVMCON unlock/write-cycle sequence writes to an internal
 *   latch that never commits anywhere, so the "write" is a silent no-op.
 *   Neither the `__ram` pointer-target qualifier nor `-flocal` (the option
 *   gating when `__ram`/`__rom` are honored, per §5.3.6.3.2) changed the
 *   generated code when tried. See `pic18fxx5x-hal/docs/ARCHITECTURE.md`
 *   Finding 3 for the full account.
 *
 *   Manually inlining the identical read-modify-write logic, but with the
 *   register address as a genuine compile-time constant (not passed through
 *   a function parameter), worked correctly and used plain SFR access, no
 *   table instructions. That's the fix applied throughout this file: every
 *   function is now a `switch` on `irq` with one case per source, each case
 *   naming its register directly so the address is always a compile-time
 *   constant at the point of access, never a value that has crossed a
 *   function-call boundary as a `uint16_t`.
 */

#include "core/pic18_irq.h"

/* Read-modify-write against a *literal* SFR address (never a variable):
 * `reg` must be a `PIC_REG_*` macro, textually substituted, so `reg` is
 * still a compile-time constant inside `pic8_sfr_read8`/`write8` after
 * expansion. See this file's own header comment for why that distinction
 * is the entire fix. */
#define SFR_SET_BIT(reg, mask) \
    pic8_sfr_write8((reg), (uint8_t)(pic8_sfr_read8(reg) | (mask)))
#define SFR_CLR_BIT(reg, mask) \
    pic8_sfr_write8((reg), (uint8_t)(pic8_sfr_read8(reg) & (uint8_t)~(mask)))

/* ───────────────────────── public API ───────────────────────────── */

uint8_t HAL_IRQ_Disable(void)
{
    uint8_t intcon = pic8_sfr_read8(PIC_REG_INTCON);
    uint8_t prev = (intcon & (PIC_INTCON_GIEH | PIC_INTCON_GIEL)) ? 1U : 0U;
    SFR_CLR_BIT(PIC_REG_INTCON, PIC_INTCON_GIEH);
    SFR_CLR_BIT(PIC_REG_INTCON, PIC_INTCON_GIEL);
    return prev;
}

void HAL_IRQ_Restore(uint8_t prev_state)
{
    if (prev_state) {
        /* Activate the two-vector priority scheme (DS39632E §9.0, RCON<7>)
         * before enabling the masters, so high/low routing is in effect. */
        SFR_SET_BIT(PIC_REG_RCON, PIC_RCON_IPEN);
        SFR_SET_BIT(PIC_REG_INTCON, PIC_INTCON_GIEH);
        SFR_SET_BIT(PIC_REG_INTCON, PIC_INTCON_GIEL);
    } else {
        SFR_CLR_BIT(PIC_REG_INTCON, PIC_INTCON_GIEH);
        SFR_CLR_BIT(PIC_REG_INTCON, PIC_INTCON_GIEL);
    }
}

void HAL_IRQ_Enable(PIC18_IRQn irq)
{
    switch (irq) {
    case PIC18_IRQ_INT0:     SFR_SET_BIT(PIC_REG_INTCON,  PIC_INTCON_INT0IE);  break;
    case PIC18_IRQ_INT1:     SFR_SET_BIT(PIC_REG_INTCON3, PIC_INTCON3_INT1IE); break;
    case PIC18_IRQ_INT2:     SFR_SET_BIT(PIC_REG_INTCON3, PIC_INTCON3_INT2IE); break;
    case PIC18_IRQ_RB:       SFR_SET_BIT(PIC_REG_INTCON,  PIC_INTCON_RBIE);   break;
    case PIC18_IRQ_TMR0:     SFR_SET_BIT(PIC_REG_INTCON,  PIC_INTCON_TMR0IE); break;
    case PIC18_IRQ_TMR1:     SFR_SET_BIT(PIC_REG_PIE1,    PIC_PIE1_TMR1IE);   break;
    case PIC18_IRQ_TMR2:     SFR_SET_BIT(PIC_REG_PIE1,    PIC_PIE1_TMR2IE);   break;
    case PIC18_IRQ_TMR3:     SFR_SET_BIT(PIC_REG_PIE2,    PIC_PIE2_TMR3IE);   break;
    case PIC18_IRQ_CCP1:     SFR_SET_BIT(PIC_REG_PIE1,    PIC_PIE1_CCP1IE);   break;
    case PIC18_IRQ_SSP:      SFR_SET_BIT(PIC_REG_PIE1,    PIC_PIE1_SSPIE);    break;
    case PIC18_IRQ_USART_TX: SFR_SET_BIT(PIC_REG_PIE1,    PIC_PIE1_TXIE);     break;
    case PIC18_IRQ_USART_RX: SFR_SET_BIT(PIC_REG_PIE1,    PIC_PIE1_RCIE);     break;
    case PIC18_IRQ_ADC:      SFR_SET_BIT(PIC_REG_PIE1,    PIC_PIE1_ADIE);     break;
    case PIC18_IRQ_CCP2:     SFR_SET_BIT(PIC_REG_PIE2,    PIC_PIE2_CCP2IE);   break;
    case PIC18_IRQ_CMP:      SFR_SET_BIT(PIC_REG_PIE2,    PIC_PIE2_CMIE);     break;
    case PIC18_IRQ_EEPROM:   SFR_SET_BIT(PIC_REG_PIE2,    PIC_PIE2_EEIE);     break;
#if PIC18FXX5X_FAMILY_HAS_SPP
    case PIC18_IRQ_SPP:      SFR_SET_BIT(PIC_REG_PIE1,    PIC_PIE1_SPPIE);    break;
#endif
    default: break;
    }
}

void HAL_IRQ_DisableSrc(PIC18_IRQn irq)
{
    switch (irq) {
    case PIC18_IRQ_INT0:     SFR_CLR_BIT(PIC_REG_INTCON,  PIC_INTCON_INT0IE);  break;
    case PIC18_IRQ_INT1:     SFR_CLR_BIT(PIC_REG_INTCON3, PIC_INTCON3_INT1IE); break;
    case PIC18_IRQ_INT2:     SFR_CLR_BIT(PIC_REG_INTCON3, PIC_INTCON3_INT2IE); break;
    case PIC18_IRQ_RB:       SFR_CLR_BIT(PIC_REG_INTCON,  PIC_INTCON_RBIE);   break;
    case PIC18_IRQ_TMR0:     SFR_CLR_BIT(PIC_REG_INTCON,  PIC_INTCON_TMR0IE); break;
    case PIC18_IRQ_TMR1:     SFR_CLR_BIT(PIC_REG_PIE1,    PIC_PIE1_TMR1IE);   break;
    case PIC18_IRQ_TMR2:     SFR_CLR_BIT(PIC_REG_PIE1,    PIC_PIE1_TMR2IE);   break;
    case PIC18_IRQ_TMR3:     SFR_CLR_BIT(PIC_REG_PIE2,    PIC_PIE2_TMR3IE);   break;
    case PIC18_IRQ_CCP1:     SFR_CLR_BIT(PIC_REG_PIE1,    PIC_PIE1_CCP1IE);   break;
    case PIC18_IRQ_SSP:      SFR_CLR_BIT(PIC_REG_PIE1,    PIC_PIE1_SSPIE);    break;
    case PIC18_IRQ_USART_TX: SFR_CLR_BIT(PIC_REG_PIE1,    PIC_PIE1_TXIE);     break;
    case PIC18_IRQ_USART_RX: SFR_CLR_BIT(PIC_REG_PIE1,    PIC_PIE1_RCIE);     break;
    case PIC18_IRQ_ADC:      SFR_CLR_BIT(PIC_REG_PIE1,    PIC_PIE1_ADIE);     break;
    case PIC18_IRQ_CCP2:     SFR_CLR_BIT(PIC_REG_PIE2,    PIC_PIE2_CCP2IE);   break;
    case PIC18_IRQ_CMP:      SFR_CLR_BIT(PIC_REG_PIE2,    PIC_PIE2_CMIE);     break;
    case PIC18_IRQ_EEPROM:   SFR_CLR_BIT(PIC_REG_PIE2,    PIC_PIE2_EEIE);     break;
#if PIC18FXX5X_FAMILY_HAS_SPP
    case PIC18_IRQ_SPP:      SFR_CLR_BIT(PIC_REG_PIE1,    PIC_PIE1_SPPIE);    break;
#endif
    default: break;
    }
}

void HAL_IRQ_ClearFlag(PIC18_IRQn irq)
{
    switch (irq) {
    case PIC18_IRQ_INT0:     SFR_CLR_BIT(PIC_REG_INTCON,  PIC_INTCON_INT0IF);  break;
    case PIC18_IRQ_INT1:     SFR_CLR_BIT(PIC_REG_INTCON3, PIC_INTCON3_INT1IF); break;
    case PIC18_IRQ_INT2:     SFR_CLR_BIT(PIC_REG_INTCON3, PIC_INTCON3_INT2IF); break;
    case PIC18_IRQ_RB:       SFR_CLR_BIT(PIC_REG_INTCON,  PIC_INTCON_RBIF);   break;
    case PIC18_IRQ_TMR0:     SFR_CLR_BIT(PIC_REG_INTCON,  PIC_INTCON_TMR0IF); break;
    case PIC18_IRQ_TMR1:     SFR_CLR_BIT(PIC_REG_PIR1,    PIC_PIR1_TMR1IF);   break;
    case PIC18_IRQ_TMR2:     SFR_CLR_BIT(PIC_REG_PIR1,    PIC_PIR1_TMR2IF);   break;
    case PIC18_IRQ_TMR3:     SFR_CLR_BIT(PIC_REG_PIR2,    PIC_PIR2_TMR3IF);   break;
    case PIC18_IRQ_CCP1:     SFR_CLR_BIT(PIC_REG_PIR1,    PIC_PIR1_CCP1IF);   break;
    case PIC18_IRQ_SSP:      SFR_CLR_BIT(PIC_REG_PIR1,    PIC_PIR1_SSPIF);    break;
    case PIC18_IRQ_USART_TX: SFR_CLR_BIT(PIC_REG_PIR1,    PIC_PIR1_TXIF);     break;
    case PIC18_IRQ_USART_RX: SFR_CLR_BIT(PIC_REG_PIR1,    PIC_PIR1_RCIF);     break;
    case PIC18_IRQ_ADC:      SFR_CLR_BIT(PIC_REG_PIR1,    PIC_PIR1_ADIF);     break;
    case PIC18_IRQ_CCP2:     SFR_CLR_BIT(PIC_REG_PIR2,    PIC_PIR2_CCP2IF);   break;
    case PIC18_IRQ_CMP:      SFR_CLR_BIT(PIC_REG_PIR2,    PIC_PIR2_CMIF);     break;
    case PIC18_IRQ_EEPROM:   SFR_CLR_BIT(PIC_REG_PIR2,    PIC_PIR2_EEIF);     break;
#if PIC18FXX5X_FAMILY_HAS_SPP
    case PIC18_IRQ_SPP:      SFR_CLR_BIT(PIC_REG_PIR1,    PIC_PIR1_SPPIF);    break;
#endif
    default: break;
    }
}

uint8_t HAL_IRQ_GetFlag(PIC18_IRQn irq)
{
    switch (irq) {
    case PIC18_IRQ_INT0:     return (pic8_sfr_read8(PIC_REG_INTCON)  & PIC_INTCON_INT0IF)  ? 1U : 0U;
    case PIC18_IRQ_INT1:     return (pic8_sfr_read8(PIC_REG_INTCON3) & PIC_INTCON3_INT1IF) ? 1U : 0U;
    case PIC18_IRQ_INT2:     return (pic8_sfr_read8(PIC_REG_INTCON3) & PIC_INTCON3_INT2IF) ? 1U : 0U;
    case PIC18_IRQ_RB:       return (pic8_sfr_read8(PIC_REG_INTCON)  & PIC_INTCON_RBIF)    ? 1U : 0U;
    case PIC18_IRQ_TMR0:     return (pic8_sfr_read8(PIC_REG_INTCON)  & PIC_INTCON_TMR0IF)  ? 1U : 0U;
    case PIC18_IRQ_TMR1:     return (pic8_sfr_read8(PIC_REG_PIR1)    & PIC_PIR1_TMR1IF)    ? 1U : 0U;
    case PIC18_IRQ_TMR2:     return (pic8_sfr_read8(PIC_REG_PIR1)    & PIC_PIR1_TMR2IF)    ? 1U : 0U;
    case PIC18_IRQ_TMR3:     return (pic8_sfr_read8(PIC_REG_PIR2)    & PIC_PIR2_TMR3IF)    ? 1U : 0U;
    case PIC18_IRQ_CCP1:     return (pic8_sfr_read8(PIC_REG_PIR1)    & PIC_PIR1_CCP1IF)    ? 1U : 0U;
    case PIC18_IRQ_SSP:      return (pic8_sfr_read8(PIC_REG_PIR1)    & PIC_PIR1_SSPIF)     ? 1U : 0U;
    case PIC18_IRQ_USART_TX: return (pic8_sfr_read8(PIC_REG_PIR1)    & PIC_PIR1_TXIF)      ? 1U : 0U;
    case PIC18_IRQ_USART_RX: return (pic8_sfr_read8(PIC_REG_PIR1)    & PIC_PIR1_RCIF)      ? 1U : 0U;
    case PIC18_IRQ_ADC:      return (pic8_sfr_read8(PIC_REG_PIR1)    & PIC_PIR1_ADIF)      ? 1U : 0U;
    case PIC18_IRQ_CCP2:     return (pic8_sfr_read8(PIC_REG_PIR2)    & PIC_PIR2_CCP2IF)    ? 1U : 0U;
    case PIC18_IRQ_CMP:      return (pic8_sfr_read8(PIC_REG_PIR2)    & PIC_PIR2_CMIF)      ? 1U : 0U;
    case PIC18_IRQ_EEPROM:   return (pic8_sfr_read8(PIC_REG_PIR2)    & PIC_PIR2_EEIF)      ? 1U : 0U;
#if PIC18FXX5X_FAMILY_HAS_SPP
    case PIC18_IRQ_SPP:      return (pic8_sfr_read8(PIC_REG_PIR1)    & PIC_PIR1_SPPIF)     ? 1U : 0U;
#endif
    default: return 0U;
    }
}

void HAL_IRQ_SetPriority(PIC18_IRQn irq, HAL_IRQ_Priority prio)
{
    uint8_t high = (prio == HAL_IRQ_PRIORITY_HIGH) ? 1U : 0U;
    switch (irq) {
    case PIC18_IRQ_INT0:     break; /* always high, no bit to set. */
    case PIC18_IRQ_INT1:
        if (high) SFR_SET_BIT(PIC_REG_INTCON3, PIC_INTCON3_INT1IP);
        else      SFR_CLR_BIT(PIC_REG_INTCON3, PIC_INTCON3_INT1IP);
        break;
    case PIC18_IRQ_INT2:
        if (high) SFR_SET_BIT(PIC_REG_INTCON3, PIC_INTCON3_INT2IP);
        else      SFR_CLR_BIT(PIC_REG_INTCON3, PIC_INTCON3_INT2IP);
        break;
    case PIC18_IRQ_RB:
        if (high) SFR_SET_BIT(PIC_REG_INTCON2, PIC_INTCON2_RBIP);
        else      SFR_CLR_BIT(PIC_REG_INTCON2, PIC_INTCON2_RBIP);
        break;
    case PIC18_IRQ_TMR0:
        if (high) SFR_SET_BIT(PIC_REG_INTCON2, PIC_INTCON2_TMR0IP);
        else      SFR_CLR_BIT(PIC_REG_INTCON2, PIC_INTCON2_TMR0IP);
        break;
    case PIC18_IRQ_TMR1:
        if (high) SFR_SET_BIT(PIC_REG_IPR1, PIC_IPR1_TMR1IP);
        else      SFR_CLR_BIT(PIC_REG_IPR1, PIC_IPR1_TMR1IP);
        break;
    case PIC18_IRQ_TMR2:
        if (high) SFR_SET_BIT(PIC_REG_IPR1, PIC_IPR1_TMR2IP);
        else      SFR_CLR_BIT(PIC_REG_IPR1, PIC_IPR1_TMR2IP);
        break;
    case PIC18_IRQ_TMR3:
        if (high) SFR_SET_BIT(PIC_REG_IPR2, PIC_IPR2_TMR3IP);
        else      SFR_CLR_BIT(PIC_REG_IPR2, PIC_IPR2_TMR3IP);
        break;
    case PIC18_IRQ_CCP1:
        if (high) SFR_SET_BIT(PIC_REG_IPR1, PIC_IPR1_CCP1IP);
        else      SFR_CLR_BIT(PIC_REG_IPR1, PIC_IPR1_CCP1IP);
        break;
    case PIC18_IRQ_SSP:
        if (high) SFR_SET_BIT(PIC_REG_IPR1, PIC_IPR1_SSPIP);
        else      SFR_CLR_BIT(PIC_REG_IPR1, PIC_IPR1_SSPIP);
        break;
    case PIC18_IRQ_USART_TX:
        if (high) SFR_SET_BIT(PIC_REG_IPR1, PIC_IPR1_TXIP);
        else      SFR_CLR_BIT(PIC_REG_IPR1, PIC_IPR1_TXIP);
        break;
    case PIC18_IRQ_USART_RX:
        if (high) SFR_SET_BIT(PIC_REG_IPR1, PIC_IPR1_RCIP);
        else      SFR_CLR_BIT(PIC_REG_IPR1, PIC_IPR1_RCIP);
        break;
    case PIC18_IRQ_ADC:
        if (high) SFR_SET_BIT(PIC_REG_IPR1, PIC_IPR1_ADIP);
        else      SFR_CLR_BIT(PIC_REG_IPR1, PIC_IPR1_ADIP);
        break;
    case PIC18_IRQ_CCP2:
        if (high) SFR_SET_BIT(PIC_REG_IPR2, PIC_IPR2_CCP2IP);
        else      SFR_CLR_BIT(PIC_REG_IPR2, PIC_IPR2_CCP2IP);
        break;
    case PIC18_IRQ_CMP:
        if (high) SFR_SET_BIT(PIC_REG_IPR2, PIC_IPR2_CMIP);
        else      SFR_CLR_BIT(PIC_REG_IPR2, PIC_IPR2_CMIP);
        break;
    case PIC18_IRQ_EEPROM:
        if (high) SFR_SET_BIT(PIC_REG_IPR2, PIC_IPR2_EEIP);
        else      SFR_CLR_BIT(PIC_REG_IPR2, PIC_IPR2_EEIP);
        break;
#if PIC18FXX5X_FAMILY_HAS_SPP
    case PIC18_IRQ_SPP:
        if (high) SFR_SET_BIT(PIC_REG_IPR1, PIC_IPR1_SPPIP);
        else      SFR_CLR_BIT(PIC_REG_IPR1, PIC_IPR1_SPPIP);
        break;
#endif
    default: break;
    }
}
