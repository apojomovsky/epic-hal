/**
 * @file    pic18fxx5x_comp.c
 * @brief   Comparator driver, implementation (DS39632E §22.0).
 *
 *   Simpler than the PIC16 driver: CMCON is in the Access Bank (0xFB4),
 *   no bank switching needed. The sim backend sets C1OUT/C2OUT + CMIF
 *   from `pic18_sim_drive_comp()`.
 */

#include "peripherals/pic18fxx5x_comp.h"
#include "core/pic18_irq.h"

/* ───────────────────────── handle storage ───────────────────────── */

static COMP_HandleTypeDef        g_comp_storage;
static const COMP_HandleTypeDef *g_comp = NULL;

/* ───────────────────────── public API ───────────────────────────── */

EPIC_StatusTypeDef EPIC_COMP_Init(const COMP_HandleTypeDef *h)
{
    if (!h) return EPIC_INVALID;
    g_comp_storage = *h;
    g_comp = &g_comp_storage;

    /* Build CMCON (Register 22-1, Access Bank 0xFB4).
     *   bits 2:0 CM2:CM0 (mode)
     *   bit 3   CIS
     *   bit 4   C1INV
     *   bit 5   C2INV
     *   bits 7:6 C2OUT:C1OUT (read-only status, cleared on write). */
    uint8_t v = (uint8_t)(h->Mode & PIC_CMCON_CM_MASK);
    if (h->CIS)        v |= PIC_CMCON_CIS;
    if (h->C1Inverted) v |= PIC_CMCON_C1INV;
    if (h->C2Inverted) v |= PIC_CMCON_C2INV;
    pic8_sfr_write8(PIC_REG_CMCON, v);

    /* Interrupt enable. */
    EPIC_IRQ_ClearFlag(PIC18_IRQ_CMP);
    if (h->ChangeCallback) EPIC_IRQ_Enable(PIC18_IRQ_CMP);
    else                   EPIC_IRQ_DisableSrc(PIC18_IRQ_CMP);

    return EPIC_OK;
}

EPIC_StatusTypeDef EPIC_COMP_DeInit(void)
{
    EPIC_IRQ_DisableSrc(PIC18_IRQ_CMP);
    EPIC_IRQ_ClearFlag(PIC18_IRQ_CMP);
    pic8_sfr_write8(PIC_REG_CMCON, PIC_CMCON_POR_VALUE);   /* 0x07, off. */
    g_comp = NULL;
    return EPIC_OK;
}

uint8_t EPIC_COMP_C1Out(void)
{
    return (pic8_sfr_read8(PIC_REG_CMCON) & PIC_CMCON_C1OUT) ? 1U : 0U;
}

uint8_t EPIC_COMP_C2Out(void)
{
    return (pic8_sfr_read8(PIC_REG_CMCON) & PIC_CMCON_C2OUT) ? 1U : 0U;
}

uint8_t EPIC_COMP_IsChangeFlag(void)
{
    return (pic8_sfr_read8(PIC_REG_PIR2) & PIC_PIR2_CMIF) ? 1U : 0U;
}

void EPIC_COMP_ClearChangeFlag(void)
{
    EPIC_IRQ_ClearFlag(PIC18_IRQ_CMP);
}

/* ───────────────────────── ISR ───────────────────────────────────── */

void COMP_IRQHandler(void)
{
    if (!EPIC_IRQ_GetFlag(PIC18_IRQ_CMP)) return;
    EPIC_IRQ_ClearFlag(PIC18_IRQ_CMP);
    if (g_comp && g_comp->ChangeCallback) g_comp->ChangeCallback();
}
