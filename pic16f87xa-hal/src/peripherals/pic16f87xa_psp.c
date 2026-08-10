/**
 * @file    pic16f87xa_psp.c
 * @brief   Parallel Slave Port driver, implementation (DS39582B §4.5).
 */

#include "peripherals/pic16f87xa_psp.h"
#include "core/pic16_irq.h"

static void (*g_psp_cb)(void) = NULL;

static uint8_t b1_trise(void)
{
    uint8_t v = 0U;
#ifdef EPIC_BANK1_READ8
    /* See target/pic16f87xa_platform.h: same corruption shape, read side. */
    EPIC_BANK1_READ8(TRISE, v);
#else
    uint8_t prev = (EPIC_REG8(PIC_REG_STATUS) >> 5) & 0x03U;
    pic_select_bank(1);
    v = EPIC_REG8(0x89U);
    pic_select_bank(prev);
#endif
    return v;
}

static void b1_trise_write(uint8_t v)
{
#ifdef EPIC_BANK1_WRITE8
    EPIC_BANK1_WRITE8(TRISE, v);
#else
    uint8_t prev = (EPIC_REG8(PIC_REG_STATUS) >> 5) & 0x03U;
    pic_select_bank(1);
    EPIC_REG8(0x89U) = v;
    pic_select_bank(prev);
#endif
}

EPIC_StatusTypeDef EPIC_PSP_Init(void (*callback)(void))
{
    g_psp_cb = callback;
    /* Clear the read-only status flags (IBF, OBF, IBOV) by writing
     * TRISE with them clear. The upper status bits (TRISE<7:5>) are
     * cleared by writing 0; the direction bits (TRISE<2:0>) and
     * PSPMODE are left to the user. */
    b1_trise_write(b1_trise() &
                   (uint8_t)~(PIC_TRISE_IBF | PIC_TRISE_OBF | PIC_TRISE_IBOV));
    EPIC_IRQ_ClearFlag(PIC16_IRQ_PSP);
    if (callback) EPIC_IRQ_Enable(PIC16_IRQ_PSP);
    else          EPIC_IRQ_DisableSrc(PIC16_IRQ_PSP);
    return EPIC_OK;
}

EPIC_StatusTypeDef EPIC_PSP_DeInit(void)
{
    EPIC_IRQ_DisableSrc(PIC16_IRQ_PSP);
    EPIC_IRQ_ClearFlag(PIC16_IRQ_PSP);
    b1_trise_write(0x07U);    /* POR default: I/O mode, no PSP. */
    g_psp_cb = NULL;
    return EPIC_OK;
}

void EPIC_PSP_Enable(void)
{
    b1_trise_write(b1_trise() | PIC_TRISE_PSPMODE);
}

void EPIC_PSP_Disable(void)
{
    b1_trise_write(b1_trise() & (uint8_t)~PIC_TRISE_PSPMODE);
}

uint8_t EPIC_PSP_IsInputBufferFull(void)
{
    return (b1_trise() & PIC_TRISE_IBF) ? 1U : 0U;
}

uint8_t EPIC_PSP_IsOutputBufferFull(void)
{
    return (b1_trise() & PIC_TRISE_OBF) ? 1U : 0U;
}

uint8_t EPIC_PSP_HasInputOverflow(void)
{
    return (b1_trise() & PIC_TRISE_IBOV) ? 1U : 0U;
}

void EPIC_PSP_ClearInputOverflow(void)
{
    b1_trise_write(b1_trise() & (uint8_t)~PIC_TRISE_IBOV);
}

void PSP_IRQHandler(void)
{
    /* Direct flag ops (class-F: the table route clobbers PCLATH in ISR
     * context; see the CCP handlers). PSPIF is PIR1 bit 7. */
    if (!(EPIC_REG8(PIC_REG_PIR1) & PIC_PIR1_PSPIF)) return;
    EPIC_BIT_CLR(EPIC_REG8(PIC_REG_PIR1), PIC_PIR1_PSPIF);
    if (g_psp_cb) g_psp_cb();
}
