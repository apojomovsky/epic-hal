/* Shared PIC14 mid-range comparator implementation (87XA + 628A class
 * parts: single CMCON, identical bit layout, DFP-verified).
 * Sources: DS39582B §12 (87XA). */

#include "peripherals/pic14_comp.h"
#include "core/pic16_irq.h"

static const COMP_HandleTypeDef *g_comp = NULL;

/**
 * @brief Initialize the comparators: program CMCON from the handle and
 *        arm the change interrupt if a callback is set.
 * @param h handle with Mode, C1Inverted, C2Inverted, CIS, ChangeCallback.
 * @return EPIC_OK on success, EPIC_INVALID if `h` is NULL.
 */
EPIC_StatusTypeDef EPIC_COMP_Init(const COMP_HandleTypeDef *h)
{
    if (!h) return EPIC_INVALID;
    g_comp = h;

    /* Build CMCON (Bank 1 at 0x9C on 87XA, Bank 0 at 0x1F on the 628A). */
    uint8_t v = (uint8_t)(h->Mode & PIC_CMCON_CM_MASK);
    if (h->CIS)        v |= PIC_CMCON_CIS;
    if (h->C1Inverted) v |= PIC_CMCON_C1INV;
    if (h->C2Inverted) v |= PIC_CMCON_C2INV;
#if PIC14MIDRANGE_HAS_CMCON_BANK0
    {
        uint8_t prev = (EPIC_REG8(PIC_REG_STATUS) >> 5) & 0x03U;
        pic_select_bank(0);
        EPIC_REG8(PIC_REG_CMCON) = v;
        pic_select_bank(prev);
    }
#else
#ifdef EPIC_BANK1_WRITE8
    /* See the family target platform header: a plain bank-switch RMW
     * here silently corrupts under XC8 v4.00. */
    EPIC_BANK1_WRITE8(CMCON, v);
#else
    {
        uint8_t prev = (EPIC_REG8(PIC_REG_STATUS) >> 5) & 0x03U;
        pic_select_bank(1);
        EPIC_REG8(0x9CU) = v;
        pic_select_bank(prev);
    }
#endif
#endif

    /* Interrupt enable. */
    EPIC_IRQ_ClearFlag(PIC16_IRQ_CMP);
    if (h->ChangeCallback) EPIC_IRQ_Enable(PIC16_IRQ_CMP);
    else                   EPIC_IRQ_DisableSrc(PIC16_IRQ_CMP);

    return EPIC_OK;
}

/**
 * @brief De-initialize the comparators: disable the interrupt and reset
 *        CMCON to its power-on default (comparators off).
 * @return EPIC_OK on success.
 */
EPIC_StatusTypeDef EPIC_COMP_DeInit(void)
{
    EPIC_IRQ_DisableSrc(PIC16_IRQ_CMP);
    EPIC_IRQ_ClearFlag(PIC16_IRQ_CMP);
#if PIC14MIDRANGE_HAS_CMCON_BANK0
    {
        uint8_t prev = (EPIC_REG8(PIC_REG_STATUS) >> 5) & 0x03U;
        pic_select_bank(0);
        EPIC_REG8(PIC_REG_CMCON) = 0x07U;   /* POR default: off. */
        pic_select_bank(prev);
    }
#else
    {
        uint8_t prev = (EPIC_REG8(PIC_REG_STATUS) >> 5) & 0x03U;
        pic_select_bank(1);
        EPIC_REG8(0x9CU) = 0x07U;     /* POR default: comparators off. */
        pic_select_bank(prev);
    }
#endif
    g_comp = NULL;
    return EPIC_OK;
}

/**
 * @brief Read the C1 output bit.
 * @return 1 if C1 output is high, 0 otherwise.
 */
uint8_t EPIC_COMP_C1Out(void)
{
    uint8_t v = 0U;
#if PIC14MIDRANGE_HAS_CMCON_BANK0
    {
        uint8_t prev = (EPIC_REG8(PIC_REG_STATUS) >> 5) & 0x03U;
        pic_select_bank(0);
        v = EPIC_REG8(PIC_REG_CMCON);
        pic_select_bank(prev);
    }
#else
#ifdef EPIC_BANK1_READ8
    /* See the family target platform header: same corruption shape. */
    EPIC_BANK1_READ8(CMCON, v);
#else
    uint8_t prev = (EPIC_REG8(PIC_REG_STATUS) >> 5) & 0x03U;
    pic_select_bank(1);
    v = EPIC_REG8(0x9CU);
    pic_select_bank(prev);
#endif
#endif
    return (v & PIC_CMCON_C1OUT) ? 1U : 0U;
}

/**
 * @brief Read the C2 output bit.
 * @return 1 if C2 output is high, 0 otherwise.
 */
uint8_t EPIC_COMP_C2Out(void)
{
    uint8_t v = 0U;
#if PIC14MIDRANGE_HAS_CMCON_BANK0
    {
        uint8_t prev = (EPIC_REG8(PIC_REG_STATUS) >> 5) & 0x03U;
        pic_select_bank(0);
        v = EPIC_REG8(PIC_REG_CMCON);
        pic_select_bank(prev);
    }
#else
#ifdef EPIC_BANK1_READ8
    EPIC_BANK1_READ8(CMCON, v);
#else
    uint8_t prev = (EPIC_REG8(PIC_REG_STATUS) >> 5) & 0x03U;
    pic_select_bank(1);
    v = EPIC_REG8(0x9CU);
    pic_select_bank(prev);
#endif
#endif
    return (v & PIC_CMCON_C2OUT) ? 1U : 0U;
}

/**
 * @brief Report whether the comparator change flag is set.
 * @return 1 if CMIF (PIR2<6>) is set, 0 otherwise.
 */
uint8_t EPIC_COMP_IsChangeFlag(void)
{
#if PIC14MIDRANGE_HAS_CM_PIR1
    /* CMIF lives in PIR1<6>. */
    return (EPIC_REG8(PIC_REG_PIR1) & PIC_PIR1_CMIF) ? 1U : 0U;
#else
    /* CMIF lives in PIR2<6>. */
    return (EPIC_REG8(0x0DU) & 0x40U) ? 1U : 0U;
#endif
}

/**
 * @brief Clear the comparator change flag.
 */
void EPIC_COMP_ClearChangeFlag(void)
{
    EPIC_IRQ_ClearFlag(PIC16_IRQ_CMP);
}

/**
 * @brief Weak comparator ISR: clears CMIF and fires the change callback.
 */
void COMP_IRQHandler(void)
{
    /* Direct flag ops (class-F: the table route clobbers PCLATH in ISR
     * context; see the CCP handlers). CMIF is PIR2 bit 5 (PIR1 bit 6
     * on Bank-0-CMCON parts). */
#if PIC14MIDRANGE_HAS_CM_PIR1
    if (!(EPIC_REG8(PIC_REG_PIR1) & PIC_PIR1_CMIF)) return;
    EPIC_BIT_CLR(EPIC_REG8(PIC_REG_PIR1), PIC_PIR1_CMIF);
#else
    if (!(EPIC_REG8(PIC_REG_PIR2) & PIC_PIR2_CMIF)) return;
    EPIC_BIT_CLR(EPIC_REG8(PIC_REG_PIR2), PIC_PIR2_CMIF);
#endif
    if (g_comp && g_comp->ChangeCallback) g_comp->ChangeCallback();
}
