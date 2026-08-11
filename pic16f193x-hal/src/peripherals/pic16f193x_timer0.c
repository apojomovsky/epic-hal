/**
 * Timer0 driver, implementation (DS41364B §15.0). OPTION_REG (Register
 * 2-2) holds T0CS/T0SE/PSA/PS<2:0>; TMR0 is at 0x15. XC8 auto-banks
 * both (TMR0 in bank 0, OPTION_REG in bank 1) on this core, so every
 * access is a plain literal `PIC_REG_*` write. The handle is copied
 * into owned static storage in EPIC_TIMER0_Init (the caller's handle is
 * typically a stack local that is gone by the time the ISR reads it
 * back; storing a pointer would dangle, the same fix as the classic
 * family's dangling-pointer finding).
 */

#include "peripherals/pic16f193x_timer0.h"
#include "core/pic16f193x_irq.h"

/* Prescaler ratios, DS41364B Register 2-2: 000 -> 1:2 ... 111 -> 1:256. */
static const uint16_t ps_ratio[8] = { 2, 4, 8, 16, 32, 64, 128, 256 };

/** Per-handle storage. The 193X has only one Timer0, so a single static
 *  slot is sufficient; EPIC_TIMER0_Init COPIES the caller's handle here. */
static TIMER0_HandleTypeDef g_t0_storage;
static const TIMER0_HandleTypeDef *g_t0_handle = NULL;

/**
 * @brief Read-modify-write helper for OPTION_REG.
 * @param clr_mask bits to clear in OPTION_REG
 * @param set_mask bits to set in OPTION_REG
 */
static void option_clr_set(uint8_t clr_mask, uint8_t set_mask)
{
    uint8_t opt = EPIC_REG8(PIC_REG_OPTION);
    opt = (uint8_t)((opt & (uint8_t)~clr_mask) | set_mask);
    EPIC_REG8(PIC_REG_OPTION) = opt;
}

/**
 * @brief Configure Timer0 from the handle and store it for the ISR.
 * @param h handle with clock source, edge, prescaler and callback
 * @return EPIC_OK on success, EPIC_INVALID if `h` is NULL
 */
EPIC_StatusTypeDef EPIC_TIMER0_Init(const TIMER0_HandleTypeDef *h)
{
    if (!h) return EPIC_INVALID;

    /* Stop the timer before reconfiguring. */
    EPIC_BIT_CLR(EPIC_REG8(PIC_REG_OPTION), PIC_OPTION_T0CS);

    /* Clear TMR0IF; configure TMR0IE if a callback is provided. */
    EPIC_IRQ_ClearFlag(PIC16F193X_IRQ_TMR0);
    if (h->OverflowCallback) {
        EPIC_IRQ_Enable(PIC16F193X_IRQ_TMR0);
    } else {
        EPIC_IRQ_DisableSrc(PIC16F193X_IRQ_TMR0);
    }

    g_t0_storage = *h;
    g_t0_handle = &g_t0_storage;
    return EPIC_OK;
}

/**
 * @brief Stop Timer0, disable its interrupt and clear the counter.
 * @return EPIC_OK on success
 */
EPIC_StatusTypeDef EPIC_TIMER0_DeInit(void)
{
    EPIC_IRQ_DisableSrc(PIC16F193X_IRQ_TMR0);
    EPIC_IRQ_ClearFlag(PIC16F193X_IRQ_TMR0);
    EPIC_BIT_CLR(EPIC_REG8(PIC_REG_OPTION), PIC_OPTION_T0CS);
    EPIC_REG8(PIC_REG_TMR0) = 0x00U;
    return EPIC_OK;
}

/**
 * @brief Enable TMR0 counting: reload the counter and program the
 *        prescaler, clock source and edge in OPTION_REG.
 * @param h handle with prescaler, clock source, edge and reload value
 * @return EPIC_OK on success, EPIC_INVALID if `h` is NULL
 */
EPIC_StatusTypeDef EPIC_TIMER0_Start(const TIMER0_HandleTypeDef *h)
{
    if (!h) return EPIC_INVALID;

    /* DS41364B §15.0: writing TMR0 when the prescaler is assigned to
     * Timer0 clears the prescaler. Reload before re-enabling. */
    EPIC_REG8(PIC_REG_TMR0) = h->ReloadValue;

    /* Program prescaler assignment + ratio + clock source + edge in one
     * atomic read-modify-write. WPUEN and INTEDG are left untouched. */
    uint8_t set_mask = (uint8_t)(h->Prescaler & PIC_OPTION_PS_MASK);
    if (!h->PrescalerAssigned) set_mask |= PIC_OPTION_PSA;
    if (h->ClockSource == TIMER0_CLOCK_EXTERNAL) set_mask |= PIC_OPTION_T0CS;
    if (h->ClockEdge   == TIMER0_EDGE_FALLING)  set_mask |= PIC_OPTION_T0SE;

    uint8_t clr_mask = (uint8_t)(PIC_OPTION_PS_MASK | PIC_OPTION_PSA |
                                 PIC_OPTION_T0CS  | PIC_OPTION_T0SE);
    option_clr_set(clr_mask, set_mask);

    return EPIC_OK;
}

/**
 * @brief Disable TMR0 counting (Timer0 halted).
 * @return EPIC_OK on success
 */
EPIC_StatusTypeDef EPIC_TIMER0_Stop(void)
{
    EPIC_BIT_CLR(EPIC_REG8(PIC_REG_OPTION), PIC_OPTION_T0CS);
    return EPIC_OK;
}

/**
 * @brief Read the current TMR0 counter value.
 * @return the 8-bit counter value
 */
uint8_t EPIC_TIMER0_ReadCounter(void)
{
    return EPIC_REG8(PIC_REG_TMR0);
}

/**
 * @brief Write a new value to the counter (also clears the prescaler).
 * @param value counter value to write, 0..255
 */
void EPIC_TIMER0_WriteCounter(uint8_t value)
{
    EPIC_REG8(PIC_REG_TMR0) = value;
}

/**
 * @brief Convert a prescaler enum to its integer ratio (2, 4, ..., 256).
 * @param p prescaler selection
 * @return the divider ratio, or 1 for an out-of-range value
 */
uint16_t EPIC_TIMER0_PrescalerToRatio(TIMER0_PrescalerTypeDef p)
{
    if ((unsigned)p > 7U) return 1U;
    return ps_ratio[p];
}

/**
 * @brief Timer0 overflow ISR: clears TMR0IF and invokes the callback.
 */
void TIMER0_IRQHandler(void)
{
    /* Direct flag ops (class-F). TMR0IF is INTCON bit 2. */
    if (!(EPIC_REG8(PIC_REG_INTCON) & PIC_INTCON_TMR0IF)) return;
    EPIC_BIT_CLR(EPIC_REG8(PIC_REG_INTCON), PIC_INTCON_TMR0IF);
    if (g_t0_handle && g_t0_handle->OverflowCallback) {
        g_t0_handle->OverflowCallback();
    }
}
