/* Comparator driver implementation (DS39582B §12.0). */

#include "peripherals/pic16f87xa_comp.h"
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

    /* Build CMCON (Bank 1, address 0x9C). */
    uint8_t v = (uint8_t)(h->Mode & PIC_CMCON_CM_MASK);
    if (h->CIS)        v |= PIC_CMCON_CIS;
    if (h->C1Inverted) v |= PIC_CMCON_C1INV;
    if (h->C2Inverted) v |= PIC_CMCON_C2INV;
#ifdef EPIC_BANK1_WRITE8
    /* See target/pic16f87xa_platform.h: a plain bank-switch RMW here
     * silently corrupts under XC8 v4.00. */
    EPIC_BANK1_WRITE8(CMCON, v);
#else
    {
        uint8_t prev = (EPIC_REG8(PIC_REG_STATUS) >> 5) & 0x03U;
        pic_select_bank(1);
        EPIC_REG8(0x9CU) = v;
        pic_select_bank(prev);
    }
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
    {
        uint8_t prev = (EPIC_REG8(PIC_REG_STATUS) >> 5) & 0x03U;
        pic_select_bank(1);
        EPIC_REG8(0x9CU) = 0x07U;     /* POR default: comparators off. */
        pic_select_bank(prev);
    }
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
#ifdef EPIC_BANK1_READ8
    /* See target/pic16f87xa_platform.h: same corruption shape, read side. */
    EPIC_BANK1_READ8(CMCON, v);
#else
    uint8_t prev = (EPIC_REG8(PIC_REG_STATUS) >> 5) & 0x03U;
    pic_select_bank(1);
    v = EPIC_REG8(0x9CU);
    pic_select_bank(prev);
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
#ifdef EPIC_BANK1_READ8
    EPIC_BANK1_READ8(CMCON, v);
#else
    uint8_t prev = (EPIC_REG8(PIC_REG_STATUS) >> 5) & 0x03U;
    pic_select_bank(1);
    v = EPIC_REG8(0x9CU);
    pic_select_bank(prev);
#endif
    return (v & PIC_CMCON_C2OUT) ? 1U : 0U;
}

/**
 * @brief Report whether the comparator change flag is set.
 * @return 1 if CMIF (PIR2<6>) is set, 0 otherwise.
 */
uint8_t EPIC_COMP_IsChangeFlag(void)
{
    /* CMIF lives in PIR2<6>. */
    return (EPIC_REG8(0x0DU) & 0x40U) ? 1U : 0U;
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
     * context; see the CCP handlers). CMIF is PIR2 bit 5. */
    if (!(EPIC_REG8(PIC_REG_PIR2) & PIC_PIR2_CMIF)) return;
    EPIC_BIT_CLR(EPIC_REG8(PIC_REG_PIR2), PIC_PIR2_CMIF);
    if (g_comp && g_comp->ChangeCallback) g_comp->ChangeCallback();
}
