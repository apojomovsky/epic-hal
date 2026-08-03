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
#ifdef PIC8_BANK1_READ8
    /* See PIC8_BANK1_WRITE8's header comment (target/pic16f87xa_platform.h)
     * and pic16f87xa-hal/docs/ARCHITECTURE.md Finding 9. */
    PIC8_BANK1_READ8(TRISE, v);
#else
    uint8_t prev = (PIC8_REG8(PIC_REG_STATUS) >> 5) & 0x03U;
    pic_select_bank(1);
    v = PIC8_REG8(0x89U);
    pic_select_bank(prev);
#endif
    return v;
}

static void b1_trise_write(uint8_t v)
{
#ifdef PIC8_BANK1_WRITE8
    PIC8_BANK1_WRITE8(TRISE, v);
#else
    uint8_t prev = (PIC8_REG8(PIC_REG_STATUS) >> 5) & 0x03U;
    pic_select_bank(1);
    PIC8_REG8(0x89U) = v;
    pic_select_bank(prev);
#endif
}

HAL_StatusTypeDef HAL_PSP_Init(void (*callback)(void))
{
    g_psp_cb = callback;
    /* Clear the read-only status flags (IBF, OBF, IBOV) by writing
     * TRISE with them clear. The lower 4 bits of TRISE are
     * read-only; PSPIE/PSPMODE are left to the user. */
    b1_trise_write(b1_trise() &
                   (uint8_t)~(PIC_TRISE_IBF | PIC_TRISE_OBF | PIC_TRISE_IBOV));
    HAL_IRQ_ClearFlag(PIC16_IRQ_PSP);
    if (callback) HAL_IRQ_Enable(PIC16_IRQ_PSP);
    else          HAL_IRQ_DisableSrc(PIC16_IRQ_PSP);
    return HAL_OK;
}

HAL_StatusTypeDef HAL_PSP_DeInit(void)
{
    HAL_IRQ_DisableSrc(PIC16_IRQ_PSP);
    HAL_IRQ_ClearFlag(PIC16_IRQ_PSP);
    b1_trise_write(0x07U);    /* POR default: I/O mode, no PSP. */
    g_psp_cb = NULL;
    return HAL_OK;
}

void HAL_PSP_Enable(void)
{
    b1_trise_write(b1_trise() | PIC_TRISE_PSPMODE);
}

void HAL_PSP_Disable(void)
{
    b1_trise_write(b1_trise() & (uint8_t)~PIC_TRISE_PSPMODE);
}

uint8_t HAL_PSP_IsInputBufferFull(void)
{
    return (b1_trise() & PIC_TRISE_IBF) ? 1U : 0U;
}

uint8_t HAL_PSP_IsOutputBufferFull(void)
{
    return (b1_trise() & PIC_TRISE_OBF) ? 1U : 0U;
}

uint8_t HAL_PSP_HasInputOverflow(void)
{
    return (b1_trise() & PIC_TRISE_IBOV) ? 1U : 0U;
}

void HAL_PSP_ClearInputOverflow(void)
{
    b1_trise_write(b1_trise() & (uint8_t)~PIC_TRISE_IBOV);
}

void PSP_IRQHandler(void)
{
    if (!HAL_IRQ_GetFlag(PIC16_IRQ_PSP)) return;
    HAL_IRQ_ClearFlag(PIC16_IRQ_PSP);
    if (g_psp_cb) g_psp_cb();
}
