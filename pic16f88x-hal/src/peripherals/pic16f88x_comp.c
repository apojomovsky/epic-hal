/* Comparator driver implementation (DS40001291H §8.0). CM1CON0/CM2CON0
 * and CM2CON1 live in Bank 2; the PIE2/PIR2 flag bits are Bank 0/1. */

#include "peripherals/pic16f88x_comp.h"
#include "core/pic16_irq.h"

/* Owned copies of the caller's handles for the weak ISRs (see
 * epic-common/MANUAL.md §3.3 for the dangling-pointer hazard this
 * avoids). */
/* The ISRs only need the callbacks, so store the pointers (1 byte each)
 * rather than full handle copies (see epic-common/MANUAL.md §3.3 for
 * the dangling-pointer hazard a copy avoids; full copies cost RAM on
 * the 128-byte 882). */
static void (*g_c1_change_cb)(void) = NULL;
static void (*g_c2_change_cb)(void) = NULL;

/**
 * @brief Build a CMxCON0 byte from a handle.
 * @param h the handle.
 * @return the CMxCON0 value.
 */
static uint8_t build_cmxcon0(const COMP_HandleTypeDef *h)
{
    uint8_t v = PIC_CMx_CxON;
    v |= (uint8_t)((uint8_t)h->Channel & PIC_CMx_CxCH_MASK);
    if (h->InputSource == COMP_INPUT_REF) v |= PIC_CMx_CxR;
    if (h->Inverted)                      v |= PIC_CMx_CxPOL;
    if (h->OutputEnable)                  v |= PIC_CMx_CxOE;
    return v;
}

/**
 * @brief Initialize comparator C1: program CM1CON0 and CM2CON1<C1RSEL>,
 *        arm the C1 change interrupt if a callback is given.
 * @param h handle with Channel, InputSource, RefSource, Inverted,
 *        OutputEnable, ChangeCallback.
 * @return EPIC_OK on success, EPIC_INVALID if `h` is NULL.
 */
EPIC_StatusTypeDef EPIC_COMP1_Init(const COMP_HandleTypeDef *h)
{
    if (!h) return EPIC_INVALID;
    g_c1_change_cb = h->ChangeCallback;

    uint8_t cm1con0 = build_cmxcon0(h);
#ifdef EPIC_BANK2_WRITE8
    EPIC_BANK2_WRITE8(CM1CON0, cm1con0);
#else
    EPIC_REG8(PIC_REG_CM1CON0) = cm1con0;
#endif

    /* C1RSEL (CM2CON1<5>): CVREF vs 0.6 V reference. */
#ifdef EPIC_BANK2_READ8
    uint8_t cm2con1 = 0u;
    EPIC_BANK2_READ8(CM2CON1, cm2con1);
    if (h->RefSource == COMP_REF_CVREF) cm2con1 |= PIC_CM2CON1_C1RSEL;
    else                                 cm2con1 &= (uint8_t)~PIC_CM2CON1_C1RSEL;
    EPIC_BANK2_WRITE8(CM2CON1, cm2con1);
#else
    uint8_t cm2con1 = EPIC_REG8(PIC_REG_CM2CON1);
    if (h->RefSource == COMP_REF_CVREF) cm2con1 |= PIC_CM2CON1_C1RSEL;
    else                                 cm2con1 &= (uint8_t)~PIC_CM2CON1_C1RSEL;
    EPIC_REG8(PIC_REG_CM2CON1) = cm2con1;
#endif

    EPIC_IRQ_ClearFlag(PIC16_IRQ_C1);
    if (h->ChangeCallback) EPIC_IRQ_Enable(PIC16_IRQ_C1);
    else                   EPIC_IRQ_DisableSrc(PIC16_IRQ_C1);
    return EPIC_OK;
}

/**
 * @brief Initialize comparator C2: program CM2CON0 and CM2CON1<C2RSEL>,
 *        arm the C2 change interrupt if a callback is given.
 * @param h handle with Channel, InputSource, RefSource, Inverted,
 *        OutputEnable, ChangeCallback.
 * @return EPIC_OK on success, EPIC_INVALID if `h` is NULL.
 */
EPIC_StatusTypeDef EPIC_COMP2_Init(const COMP_HandleTypeDef *h)
{
    if (!h) return EPIC_INVALID;
    g_c2_change_cb = h->ChangeCallback;

    uint8_t cm2con0 = build_cmxcon0(h);
#ifdef EPIC_BANK2_WRITE8
    EPIC_BANK2_WRITE8(CM2CON0, cm2con0);
#else
    EPIC_REG8(PIC_REG_CM2CON0) = cm2con0;
#endif

#ifdef EPIC_BANK2_READ8
    uint8_t cm2con1 = 0u;
    EPIC_BANK2_READ8(CM2CON1, cm2con1);
    if (h->RefSource == COMP_REF_CVREF) cm2con1 |= PIC_CM2CON1_C2RSEL;
    else                                 cm2con1 &= (uint8_t)~PIC_CM2CON1_C2RSEL;
    EPIC_BANK2_WRITE8(CM2CON1, cm2con1);
#else
    uint8_t cm2con1 = EPIC_REG8(PIC_REG_CM2CON1);
    if (h->RefSource == COMP_REF_CVREF) cm2con1 |= PIC_CM2CON1_C2RSEL;
    else                                 cm2con1 &= (uint8_t)~PIC_CM2CON1_C2RSEL;
    EPIC_REG8(PIC_REG_CM2CON1) = cm2con1;
#endif

    EPIC_IRQ_ClearFlag(PIC16_IRQ_C2);
    if (h->ChangeCallback) EPIC_IRQ_Enable(PIC16_IRQ_C2);
    else                   EPIC_IRQ_DisableSrc(PIC16_IRQ_C2);
    return EPIC_OK;
}

/**
 * @brief De-initialize comparator C1.
 * @return EPIC_OK on success.
 */
EPIC_StatusTypeDef EPIC_COMP1_DeInit(void)
{
    EPIC_IRQ_DisableSrc(PIC16_IRQ_C1);
    EPIC_IRQ_ClearFlag(PIC16_IRQ_C1);
#ifdef EPIC_BANK2_WRITE8
    EPIC_BANK2_WRITE8(CM1CON0, 0x00U);
#else
    EPIC_REG8(PIC_REG_CM1CON0) = 0x00U;
#endif
    g_c1_change_cb = NULL;
    return EPIC_OK;
}

/**
 * @brief De-initialize comparator C2.
 * @return EPIC_OK on success.
 */
EPIC_StatusTypeDef EPIC_COMP2_DeInit(void)
{
    EPIC_IRQ_DisableSrc(PIC16_IRQ_C2);
    EPIC_IRQ_ClearFlag(PIC16_IRQ_C2);
#ifdef EPIC_BANK2_WRITE8
    EPIC_BANK2_WRITE8(CM2CON0, 0x00U);
#else
    EPIC_REG8(PIC_REG_CM2CON0) = 0x00U;
#endif
    g_c2_change_cb = NULL;
    return EPIC_OK;
}

/**
 * @brief Returns 1 if C1 output is high (CM1CON0<C1OUT>).
 * @return 1 if C1 output is high, 0 otherwise.
 */
uint8_t EPIC_COMP_C1Out(void)
{
#ifdef EPIC_BANK2_READ8
    uint8_t cm1con0 = 0u;
    EPIC_BANK2_READ8(CM1CON0, cm1con0);
    return (cm1con0 & PIC_CMx_CxOUT) ? 1U : 0U;
#else
    return (EPIC_REG8(PIC_REG_CM1CON0) & PIC_CMx_CxOUT) ? 1U : 0U;
#endif
}

/**
 * @brief Returns 1 if C2 output is high (CM2CON0<C2OUT>).
 * @return 1 if C2 output is high, 0 otherwise.
 */
uint8_t EPIC_COMP_C2Out(void)
{
#ifdef EPIC_BANK2_READ8
    uint8_t cm2con0 = 0u;
    EPIC_BANK2_READ8(CM2CON0, cm2con0);
    return (cm2con0 & PIC_CMx_CxOUT) ? 1U : 0U;
#else
    return (EPIC_REG8(PIC_REG_CM2CON0) & PIC_CMx_CxOUT) ? 1U : 0U;
#endif
}

/**
 * @brief Returns 1 if C1IF is set.
 * @return 1 if the C1 change flag is set, 0 otherwise.
 */
uint8_t EPIC_COMP_C1ChangeFlag(void)
{
    return EPIC_IRQ_GetFlag(PIC16_IRQ_C1);
}

/**
 * @brief Returns 1 if C2IF is set.
 * @return 1 if the C2 change flag is set, 0 otherwise.
 */
uint8_t EPIC_COMP_C2ChangeFlag(void)
{
    return EPIC_IRQ_GetFlag(PIC16_IRQ_C2);
}

/**
 * @brief Clear the C1IF flag.
 */
void EPIC_COMP_ClearC1Flag(void)
{
    EPIC_IRQ_ClearFlag(PIC16_IRQ_C1);
}

/**
 * @brief Clear the C2IF flag.
 */
void EPIC_COMP_ClearC2Flag(void)
{
    EPIC_IRQ_ClearFlag(PIC16_IRQ_C2);
}

/**
 * @brief Select the Timer1 gate source (CM2CON1<T1GSS>).
 * @param src COMP_GATE_SRC_T1G or COMP_GATE_SRC_C2OUT.
 */
void EPIC_COMP_SetT1GateSource(uint8_t src)
{
#ifdef EPIC_BANK2_READ8
    uint8_t cm2con1 = 0u;
    EPIC_BANK2_READ8(CM2CON1, cm2con1);
    if (src == COMP_GATE_SRC_T1G) cm2con1 |= PIC_CM2CON1_T1GSS;
    else                           cm2con1 &= (uint8_t)~PIC_CM2CON1_T1GSS;
    EPIC_BANK2_WRITE8(CM2CON1, cm2con1);
#else
    uint8_t cm2con1 = EPIC_REG8(PIC_REG_CM2CON1);
    if (src == COMP_GATE_SRC_T1G) cm2con1 |= PIC_CM2CON1_T1GSS;
    else                           cm2con1 &= (uint8_t)~PIC_CM2CON1_T1GSS;
    EPIC_REG8(PIC_REG_CM2CON1) = cm2con1;
#endif
}

/**
 * @brief Enable or disable C2's output synchronization to Timer1.
 * @param enable 1 to synchronize, 0 for asynchronous output.
 */
void EPIC_COMP_SetC2Sync(uint8_t enable)
{
#ifdef EPIC_BANK2_READ8
    uint8_t cm2con1 = 0u;
    EPIC_BANK2_READ8(CM2CON1, cm2con1);
    if (enable) cm2con1 |= PIC_CM2CON1_C2SYNC;
    else        cm2con1 &= (uint8_t)~PIC_CM2CON1_C2SYNC;
    EPIC_BANK2_WRITE8(CM2CON1, cm2con1);
#else
    uint8_t cm2con1 = EPIC_REG8(PIC_REG_CM2CON1);
    if (enable) cm2con1 |= PIC_CM2CON1_C2SYNC;
    else        cm2con1 &= (uint8_t)~PIC_CM2CON1_C2SYNC;
    EPIC_REG8(PIC_REG_CM2CON1) = cm2con1;
#endif
}

/**
 * @brief Weak C1 change ISR: clears C1IF and fires the change callback.
 */
void COMP1_IRQHandler(void)
{
    /* Direct flag ops (class-F). C1IF is PIR2 bit 5. */
    if (!(EPIC_REG8(PIC_REG_PIR2) & PIC_PIR2_C1IF)) return;
    EPIC_BIT_CLR(EPIC_REG8(PIC_REG_PIR2), PIC_PIR2_C1IF);
    if (g_c1_change_cb) g_c1_change_cb();
}

/**
 * @brief Weak C2 change ISR: clears C2IF and fires the change callback.
 */
void COMP2_IRQHandler(void)
{
    /* Direct flag ops (class-F). C2IF is PIR2 bit 6. */
    if (!(EPIC_REG8(PIC_REG_PIR2) & PIC_PIR2_C2IF)) return;
    EPIC_BIT_CLR(EPIC_REG8(PIC_REG_PIR2), PIC_PIR2_C2IF);
    if (g_c2_change_cb) g_c2_change_cb();
}
