/**
 * @file    pic16f87xa_comp.c
 * @brief   Comparator driver, implementation (DS39582B §12.0).
 */

#include "peripherals/pic16f87xa_comp.h"
#include "core/pic16_irq.h"

static const COMP_HandleTypeDef *g_comp = NULL;

HAL_StatusTypeDef HAL_COMP_Init(const COMP_HandleTypeDef *h)
{
    if (!h) return HAL_INVALID;
    g_comp = h;

    /* Build CMCON (Bank 1, address 0x9C). */
    uint8_t v = (uint8_t)(h->Mode & PIC_CMCON_CM_MASK);
    if (h->CIS)        v |= PIC_CMCON_CIS;
    if (h->C1Inverted) v |= PIC_CMCON_C1INV;
    if (h->C2Inverted) v |= PIC_CMCON_C2INV;
#ifdef PIC8_BANK1_WRITE8
    /* See PIC8_BANK1_WRITE8's header comment (target/pic16f87xa_platform.h)
     * and pic16f87xa-hal/docs/ARCHITECTURE.md Finding 9. */
    PIC8_BANK1_WRITE8(CMCON, v);
#else
    {
        uint8_t prev = (PIC8_REG8(PIC_REG_STATUS) >> 5) & 0x03U;
        pic_select_bank(1);
        PIC8_REG8(0x9CU) = v;
        pic_select_bank(prev);
    }
#endif

    /* Interrupt enable. */
    HAL_IRQ_ClearFlag(PIC16_IRQ_CMP);
    if (h->ChangeCallback) HAL_IRQ_Enable(PIC16_IRQ_CMP);
    else                   HAL_IRQ_DisableSrc(PIC16_IRQ_CMP);

    return HAL_OK;
}

HAL_StatusTypeDef HAL_COMP_DeInit(void)
{
    HAL_IRQ_DisableSrc(PIC16_IRQ_CMP);
    HAL_IRQ_ClearFlag(PIC16_IRQ_CMP);
    {
        uint8_t prev = (PIC8_REG8(PIC_REG_STATUS) >> 5) & 0x03U;
        pic_select_bank(1);
        PIC8_REG8(0x9CU) = 0x07U;     /* POR default: comparators off. */
        pic_select_bank(prev);
    }
    g_comp = NULL;
    return HAL_OK;
}

uint8_t HAL_COMP_C1Out(void)
{
    uint8_t v = 0U;
#ifdef PIC8_BANK1_READ8
    /* See PIC8_BANK1_WRITE8's header comment (target/pic16f87xa_platform.h)
     * and pic16f87xa-hal/docs/ARCHITECTURE.md Finding 9. */
    PIC8_BANK1_READ8(CMCON, v);
#else
    uint8_t prev = (PIC8_REG8(PIC_REG_STATUS) >> 5) & 0x03U;
    pic_select_bank(1);
    v = PIC8_REG8(0x9CU);
    pic_select_bank(prev);
#endif
    return (v & PIC_CMCON_C1OUT) ? 1U : 0U;
}

uint8_t HAL_COMP_C2Out(void)
{
    uint8_t v = 0U;
#ifdef PIC8_BANK1_READ8
    PIC8_BANK1_READ8(CMCON, v);
#else
    uint8_t prev = (PIC8_REG8(PIC_REG_STATUS) >> 5) & 0x03U;
    pic_select_bank(1);
    v = PIC8_REG8(0x9CU);
    pic_select_bank(prev);
#endif
    return (v & PIC_CMCON_C2OUT) ? 1U : 0U;
}

uint8_t HAL_COMP_IsChangeFlag(void)
{
    /* CMIF lives in PIR2<6>. */
    return (PIC8_REG8(0x0DU) & 0x40U) ? 1U : 0U;
}

void HAL_COMP_ClearChangeFlag(void)
{
    HAL_IRQ_ClearFlag(PIC16_IRQ_CMP);
}

void COMP_IRQHandler(void)
{
    if (!HAL_IRQ_GetFlag(PIC16_IRQ_CMP)) return;
    HAL_IRQ_ClearFlag(PIC16_IRQ_CMP);
    if (g_comp && g_comp->ChangeCallback) g_comp->ChangeCallback();
}
