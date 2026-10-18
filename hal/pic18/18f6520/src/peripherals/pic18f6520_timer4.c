/*
 * Timer4 driver, implementation (DS39609B §15.0, Register 15-1).
 */

#include "peripherals/pic18f6520_timer4.h"
#include "core/pic18_irq.h"

/* The ECAN quads have no Timer4 and carry CAN registers at 0xF76-0xF78,
 * so every register touch is compiled out there. */
#if PIC18F6520_FAMILY_HAS_CAN
 #define T4_PRESENT 0
#else
 #define T4_PRESENT 1
#endif

/* T4CON prescaler, DS39609B Register 15-1:
 *   00 -> 1:1, 01 -> 1:4, 1x -> 1:16 */
static const uint8_t pre_ratio[4] = { 1, 4, 16, 16 };

/* Only the callback is read back (by the weak ISR below), so it is the
 * only handle field kept. */
static void (*g_t4_cb)(void) = NULL;

/**
 * @brief  Read the Timer4 counter register (TMR4).
 * @return The current TMR4 value, 0 on parts without Timer4.
 */
uint8_t EPIC_TIMER4_ReadCounter(void)
{
    if (!T4_PRESENT) return 0U;
    return EPIC_REG8(PIC_REG_TMR4);
}

/**
 * @brief  Write the Timer4 counter register (TMR4).
 * @param value Byte to load into TMR4.
 */
void EPIC_TIMER4_WriteCounter(uint8_t value)
{
    if (!T4_PRESENT) return;
    EPIC_REG8(PIC_REG_TMR4) = value;
}

/**
 * @brief  Read the Timer4 period register (PR4).
 * @return The current PR4 value, 0 on parts without Timer4.
 */
uint8_t EPIC_TIMER4_ReadPeriod(void)
{
    if (!T4_PRESENT) return 0U;
    return EPIC_REG8(PIC_REG_PR4);
}

/**
 * @brief  Write the Timer4 period register (PR4).
 * @param period Byte to load into PR4.
 */
void EPIC_TIMER4_WritePeriod(uint8_t period)
{
    if (!T4_PRESENT) return;
    EPIC_REG8(PIC_REG_PR4) = period;
}

/**
 * @brief  Map a Timer4 prescaler enum to its ratio (DS39609B Register
 *         15-1: 00 -> 1:1, 01 -> 1:4, 1x -> 1:16).
 * @param p Prescaler selection (T4CKPS<1:0> value).
 * @return The divide ratio (1, 4 or 16).
 */
uint16_t EPIC_TIMER4_PrescalerToRatio(TIMER4_PrescalerTypeDef p)
{
    if ((unsigned)p > 3U) return 1U;
    return pre_ratio[p];
}

/**
 * @brief  Map a Timer4 postscaler enum to its ratio (DS39609B Register
 *         15-1: 1:(N+1)).
 * @param p Postscaler selection (T4OUTPS<3:0> value).
 * @return The divide ratio (1..16).
 */
uint16_t EPIC_TIMER4_PostscalerToRatio(TIMER4_PostscalerTypeDef p)
{
    if ((unsigned)p > 15U) return 1U;
    return (uint16_t)((unsigned)p + 1U);
}

/**
 * @brief  Initialize Timer4 from a handle: stop the timer, clear TMR4IF,
 *         enable the overflow interrupt if a callback is given and store
 *         the callback.
 * @param h Handle describing the Timer4 configuration.
 * @return EPIC_OK on success, EPIC_INVALID if `h` is NULL or the part
 *         has no Timer4.
 */
EPIC_StatusTypeDef EPIC_TIMER4_Init(const TIMER4_HandleTypeDef *h)
{
    if (!h || !T4_PRESENT) return EPIC_INVALID;

    EPIC_BIT_CLR(EPIC_REG8(PIC_REG_T4CON), PIC_T4CON_TMR4ON);

    EPIC_IRQ_ClearFlag(PIC18_IRQ_TMR4);
    if (h->OverflowCallback)
    {
        EPIC_IRQ_Enable(PIC18_IRQ_TMR4);
    }
    else
    {
        EPIC_IRQ_DisableSrc(PIC18_IRQ_TMR4);
    }

    g_t4_cb = h->OverflowCallback;
    return EPIC_OK;
}

/**
 * @brief  De-initialize Timer4: disable its interrupt, clear the flag,
 *         restore T4CON to its power-on value, reset PR4 and drop the
 *         stored callback.
 * @return EPIC_OK, or EPIC_INVALID on parts without Timer4.
 */
EPIC_StatusTypeDef EPIC_TIMER4_DeInit(void)
{
    if (!T4_PRESENT) return EPIC_INVALID;

    EPIC_IRQ_DisableSrc(PIC18_IRQ_TMR4);
    EPIC_IRQ_ClearFlag(PIC18_IRQ_TMR4);
    EPIC_REG8(PIC_REG_T4CON) = PIC_T4CON_POR_VALUE;
    EPIC_REG8(PIC_REG_PR4) = PIC_PR4_POR_VALUE;
    g_t4_cb = NULL;
    return EPIC_OK;
}

/**
 * @brief  Start Timer4: write PR4 first (to avoid spurious matches) then
 *         program T4CON (postscaler, prescaler) and set TMR4ON.
 * @param h Handle whose configuration is applied.
 * @return EPIC_OK on success, EPIC_INVALID if `h` is NULL or the part
 *         has no Timer4.
 */
EPIC_StatusTypeDef EPIC_TIMER4_Start(const TIMER4_HandleTypeDef *h)
{
    if (!h || !T4_PRESENT) return EPIC_INVALID;

    EPIC_REG8(PIC_REG_PR4) = h->Period;

    /* T4CON: T4OUTPS<3:0> bits 6:3, TMR4ON bit 2, T4CKPS<1:0> bits 1:0. */
    uint8_t v = 0U;
    v |= (uint8_t)((h->Postscaler & 0xFU) << 3);
    v |= PIC_T4CON_TMR4ON;
    v |= (uint8_t)(h->Prescaler & 0x3U);
    EPIC_REG8(PIC_REG_T4CON) = v;

    return EPIC_OK;
}

/**
 * @brief  Stop Timer4 by clearing TMR4ON.
 * @return EPIC_OK, or EPIC_INVALID on parts without Timer4.
 */
EPIC_StatusTypeDef EPIC_TIMER4_Stop(void)
{
    if (!T4_PRESENT) return EPIC_INVALID;

    EPIC_BIT_CLR(EPIC_REG8(PIC_REG_T4CON), PIC_T4CON_TMR4ON);
    return EPIC_OK;
}

/**
 * @brief  Weak Timer4 interrupt handler: clears TMR4IF and invokes the
 *         overflow callback registered via Init.
 */
void TIMER4_IRQHandler(void)
{
    if (!T4_PRESENT) return;
    if (!EPIC_IRQ_GetFlag(PIC18_IRQ_TMR4)) return;
    EPIC_IRQ_ClearFlag(PIC18_IRQ_TMR4);
    if (g_t4_cb)
    {
        g_t4_cb();
    }
}
