/* ULPWU driver implementation (DS40001291H §3.2.2). PCON (8Eh) is
 * Bank 1; PIE2/PIR2 are Banks 1/0. */

#include "peripherals/pic16f88x_ulpwu.h"
#include "core/pic16_irq.h"

static void (*g_ulpwu_cb)(void) = NULL;

/**
 * @brief Initialize the ULPWU module: clear the wake-up flag and arm
 *        the interrupt if a callback is given.
 * @param callback optional wake-up callback, or NULL for polling mode.
 * @return EPIC_OK on success.
 */
EPIC_StatusTypeDef EPIC_ULPWU_Init(void (*callback)(void))
{
    g_ulpwu_cb = callback;
    EPIC_IRQ_ClearFlag(PIC16_IRQ_ULPWU);
    if (callback) EPIC_IRQ_Enable(PIC16_IRQ_ULPWU);
    else          EPIC_IRQ_DisableSrc(PIC16_IRQ_ULPWU);
    return EPIC_OK;
}

/**
 * @brief De-initialize the ULPWU module: disable the interrupt, clear
 *        the flag and stop the discharge.
 * @return EPIC_OK on success.
 */
EPIC_StatusTypeDef EPIC_ULPWU_DeInit(void)
{
    EPIC_IRQ_DisableSrc(PIC16_IRQ_ULPWU);
    EPIC_IRQ_ClearFlag(PIC16_IRQ_ULPWU);
    EPIC_ULPWU_Stop();
    g_ulpwu_cb = NULL;
    return EPIC_OK;
}

/**
 * @brief Start the ULPWU discharge (set PCON<ULPWUE>).
 */
void EPIC_ULPWU_Start(void)
{
#ifdef EPIC_BANK1_READ8
    uint8_t pcon = 0u;
    EPIC_BANK1_READ8(PCON, pcon);
    pcon |= PIC_PCON_ULPWUE;
    EPIC_BANK1_WRITE8(PCON, pcon);
#else
    EPIC_REG8(PIC_REG_PCON) |= PIC_PCON_ULPWUE;
#endif
}

/**
 * @brief Stop the ULPWU discharge (clear PCON<ULPWUE>).
 */
void EPIC_ULPWU_Stop(void)
{
#ifdef EPIC_BANK1_READ8
    uint8_t pcon = 0u;
    EPIC_BANK1_READ8(PCON, pcon);
    pcon &= (uint8_t)~PIC_PCON_ULPWUE;
    EPIC_BANK1_WRITE8(PCON, pcon);
#else
    EPIC_REG8(PIC_REG_PCON) &= (uint8_t)~PIC_PCON_ULPWUE;
#endif
}

/**
 * @brief Returns 1 if a wake-up condition has occurred (PIR2<ULPWUIF>).
 * @return 1 if the wake-up flag is set, 0 otherwise.
 */
uint8_t EPIC_ULPWU_IsWakeup(void)
{
    return EPIC_IRQ_GetFlag(PIC16_IRQ_ULPWU);
}

/**
 * @brief Clear the ULPWU wake-up flag.
 */
void EPIC_ULPWU_ClearFlag(void)
{
    EPIC_IRQ_ClearFlag(PIC16_IRQ_ULPWU);
}

/**
 * @brief Weak ULPWU ISR: clears ULPWUIF and fires the callback.
 */
void ULPWU_IRQHandler(void)
{
    /* Direct flag ops (class-F). ULPWUIF is PIR2 bit 2. */
    if (!(EPIC_REG8(PIC_REG_PIR2) & PIC_PIR2_ULPWUIF)) return;
    EPIC_BIT_CLR(EPIC_REG8(PIC_REG_PIR2), PIC_PIR2_ULPWUIF);
    if (g_ulpwu_cb) g_ulpwu_cb();
}
