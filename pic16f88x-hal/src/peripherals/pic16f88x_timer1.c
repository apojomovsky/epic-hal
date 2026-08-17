/* Timer1 driver implementation (DS40001291H §6.0). */

#include "peripherals/pic16f88x_timer1.h"
#include "core/pic16_irq.h"

/* T1CON prescaler ratios, DS40001291H Register 6-1:
 *   00 → 1:1, 01 → 1:2, 10 → 1:4, 11 → 1:8 */
static const uint16_t ps_ratio[4] = { 1, 2, 4, 8 };

/* Owned copy of the caller's handle for the weak ISR (the caller's is
 * typically stack-local, out of scope by the time the ISR reads it;
 * the 87XA's pointer-holding version was a confirmed dangling-pointer
 * bug, see epic-common/MANUAL.md §3.3). Pinned to bank 2 (0x140) when
 * the part has Bank 2 GPR (883/884/886/887); the 882 (128 B RAM) has
 * none, so it falls back to the linker's best-fit scatter. */

/* The ISR only needs the callback, so store the pointer (1 byte) rather
 * than a full handle copy (see epic-common/MANUAL.md §3.3 for the
 * dangling-pointer hazard a copy avoids; a full copy costs RAM on the
 * 128-byte 882). */
static void (*g_t1_overflow_cb)(void) = NULL;

/**
 * @brief Atomically read the 16-bit counter. The datasheet warns that
 *        reading TMR1H:TMR1L in asynchronous counter mode can return
 *        inconsistent values (DS40001291H §6.5.1). Wrap that risk here
 *        so callers don't have to.
 * @return the current 16-bit TMR1H:L value.
 */
uint16_t EPIC_TIMER1_ReadCounter(void)
{
    /* Read high byte, then low byte, then high byte again; if the
     * second read differs, the low byte rolled over, so use the
     * refreshed high. Standard PIC16 idiom (DS40001291H §6.5.1). */
    uint8_t hi1, lo, hi2;
    do {
        hi1 = EPIC_REG8(PIC_REG_TMR1H);
        lo  = EPIC_REG8(PIC_REG_TMR1L);
        hi2 = EPIC_REG8(PIC_REG_TMR1H);
    } while (hi1 != hi2);

    return (uint16_t)(((uint16_t)hi2 << 8) | lo);
}

/**
 * @brief Atomically write the 16-bit counter (high byte first; writing
 *        either byte clears the prescaler).
 * @param value the 16-bit value to load.
 */
void EPIC_TIMER1_WriteCounter(uint16_t value)
{
    /* Per DS40001291H §6.8: writing TMR1H or TMR1L clears the prescaler.
     * Write high byte first. */
    EPIC_REG8(PIC_REG_TMR1H) = (uint8_t)(value >> 8);
    EPIC_REG8(PIC_REG_TMR1L) = (uint8_t)(value & 0xFFU);
}

/**
 * @brief Convert a prescaler enum to its integer ratio.
 * @param p the prescaler enum value.
 * @return the ratio (1, 2, 4 or 8), or 1 for an out-of-range value.
 */
uint16_t EPIC_TIMER1_PrescalerToRatio(TIMER1_PrescalerTypeDef p)
{
    if ((unsigned)p > 3U) return 1U;
    return ps_ratio[p];
}

/**
 * @brief Configure Timer1: stop it, arm the overflow interrupt if a
 *        callback is given, and copy the handle into driver storage.
 * @param h handle with ClockSource, ClockSync, Oscillator, Prescaler,
 *        GateEnabled, GateSource, GatePolarity, ReloadValue,
 *        OverflowCallback.
 * @return EPIC_OK on success, EPIC_INVALID if `h` is NULL.
 */
EPIC_StatusTypeDef EPIC_TIMER1_Init(const TIMER1_HandleTypeDef *h)
{
    if (!h) return EPIC_INVALID;

    /* Stop the timer before reconfiguring. */
    EPIC_BIT_CLR(EPIC_REG8(PIC_REG_T1CON), PIC_T1CON_TMR1ON);

    EPIC_IRQ_ClearFlag(PIC16_IRQ_TMR1);
    if (h->OverflowCallback) {
        EPIC_IRQ_Enable(PIC16_IRQ_TMR1);
    } else {
        EPIC_IRQ_DisableSrc(PIC16_IRQ_TMR1);
    }

    g_t1_overflow_cb = h->OverflowCallback;
    return EPIC_OK;
}

/**
 * @brief De-initialize Timer1: disable the interrupt and restore T1CON
 *        and the counter to reset values.
 * @return EPIC_OK on success.
 */
EPIC_StatusTypeDef EPIC_TIMER1_DeInit(void)
{
    EPIC_IRQ_DisableSrc(PIC16_IRQ_TMR1);
    EPIC_IRQ_ClearFlag(PIC16_IRQ_TMR1);
    EPIC_REG8(PIC_REG_T1CON) = PIC_T1CON_POR_VALUE;
    EPIC_REG8(PIC_REG_TMR1H) = 0x00U;
    EPIC_REG8(PIC_REG_TMR1L) = 0x00U;
    g_t1_overflow_cb = NULL;
    return EPIC_OK;
}

/**
 * @brief Start Timer1 counting: reload the counter and program T1CON
 *        (prescaler, oscillator, sync, clock source, gate bits),
 *        setting TMR1ON. The gate source (T1GSS) lives in CM2CON1
 *        (Bank 2).
 * @param h handle whose ReloadValue and config are applied.
 * @return EPIC_OK on success, EPIC_INVALID if `h` is NULL.
 */
EPIC_StatusTypeDef EPIC_TIMER1_Start(const TIMER1_HandleTypeDef *h)
{
    if (!h) return EPIC_INVALID;

    EPIC_TIMER1_WriteCounter(h->ReloadValue);

    /* Program T1CON in one RMW.
     *   T1GINV           → bit 7
     *   TMR1GE           → bit 6
     *   T1CKPS1:T1CKPS0  → bits 5:4
     *   T1OSCEN          → bit 3
     *   T1SYNC           → bit 2
     *   TMR1CS           → bit 1
     *   TMR1ON           → bit 0 (set last) */
    uint8_t v = 0U;
    if (h->GatePolarity == TIMER1_GATE_ACTIVE_HIGH) v |= PIC_T1CON_T1GINV;
    if (h->GateEnabled) v |= PIC_T1CON_TMR1GE;
    v |= (uint8_t)((h->Prescaler & 0x3U) << 4);
    if (h->Oscillator  == TIMER1_OSCILLATOR_ON) v |= PIC_T1CON_T1OSCEN;
    if (h->ClockSync   == TIMER1_ASYNC_EXTERNAL) v |= PIC_T1CON_T1SYNC;
    if (h->ClockSource == TIMER1_CLOCK_EXTERNAL) v |= PIC_T1CON_TMR1CS;
    v |= PIC_T1CON_TMR1ON;
    EPIC_REG8(PIC_REG_T1CON) = v;

    /* Gate source select, CM2CON1<T1GSS> (Bank 2, DS40001291H §8.8.1).
     * Only meaningful when TMR1GE is set. */
#ifdef EPIC_BANK2_READ8
    uint8_t cm2con1 = 0u;
    EPIC_BANK2_READ8(CM2CON1, cm2con1);
    if (h->GateSource == TIMER1_GATE_SRC_T1G) {
        cm2con1 |= PIC_CM2CON1_T1GSS;
    } else {
        cm2con1 &= (uint8_t)~PIC_CM2CON1_T1GSS;
    }
    EPIC_BANK2_WRITE8(CM2CON1, cm2con1);
#else
    uint8_t cm2con1 = EPIC_REG8(PIC_REG_CM2CON1);
    if (h->GateSource == TIMER1_GATE_SRC_T1G) {
        cm2con1 |= PIC_CM2CON1_T1GSS;
    } else {
        cm2con1 &= (uint8_t)~PIC_CM2CON1_T1GSS;
    }
    EPIC_REG8(PIC_REG_CM2CON1) = cm2con1;
#endif

    return EPIC_OK;
}

/**
 * @brief Stop Timer1 counting by clearing TMR1ON.
 * @return EPIC_OK on success.
 */
EPIC_StatusTypeDef EPIC_TIMER1_Stop(void)
{
    EPIC_BIT_CLR(EPIC_REG8(PIC_REG_T1CON), PIC_T1CON_TMR1ON);
    return EPIC_OK;
}

/**
 * @brief Weak Timer1 ISR: clears TMR1IF and fires the overflow
 *        callback.
 */
void TIMER1_IRQHandler(void)
{
    /* Direct flag ops (class-F: the table route clobbers PCLATH in ISR
     * context; see the CCP handlers). TMR1IF is PIR1 bit 0. */
    if (!(EPIC_REG8(PIC_REG_PIR1) & PIC_PIR1_TMR1IF)) return;
    EPIC_BIT_CLR(EPIC_REG8(PIC_REG_PIR1), PIC_PIR1_TMR1IF);
    if (g_t1_overflow_cb) {
        g_t1_overflow_cb();
    }
}
