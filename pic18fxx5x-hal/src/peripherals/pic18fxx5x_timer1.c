/*
 * Timer1 driver, implementation (DS39632E §12.0, Register 12-1).
 */

#include "peripherals/pic18fxx5x_timer1.h"
#include "core/pic18_irq.h"

/* T1CON prescaler ratios, DS39632E Register 12-1:
 *   00 -> 1:1, 01 -> 1:2, 10 -> 1:4, 11 -> 1:8 */
static const uint16_t ps_ratio[4] = { 1, 2, 4, 8 };

/** Per-handle storage. COPIES the caller's handle (typically a stack-local
 *  out of scope by the time the ISR reads it back; storing a pointer would
 *  dangle). The weak ISR reads from this owned copy. */
static TIMER1_HandleTypeDef g_t1_storage;
static const TIMER1_HandleTypeDef *g_t1_handle = NULL;

/**
 * @brief  Read the 16-bit Timer1 counter. With RD16 set, reading TMR1L
 *         latches TMR1H into a shadow, so reading low then high yields a
 *         consistent 16-bit value.
 * @return The current TMR1H:TMR1L value.
 */
uint16_t EPIC_TIMER1_ReadCounter(void)
{
    /* With RD16 set, reading TMR1L latches TMR1H into a shadow (DS39632E
     * §12.0); read low then high for a consistent 16-bit value. */
    uint8_t lo = epic_sfr_read8(PIC_REG_TMR1L);
    uint8_t hi = epic_sfr_read8(PIC_REG_TMR1H);
    return (uint16_t)(((uint16_t)hi << 8) | lo);
}

/**
 * @brief  Write the 16-bit Timer1 counter. With RD16 set, writing TMR1L
 *         latches TMR1H, so the high byte is written first via the shadow
 *         and the low byte commits both.
 * @param value 16-bit value to load into TMR1H:TMR1L.
 */
void EPIC_TIMER1_WriteCounter(uint16_t value)
{
    /* With RD16 set, writing TMR1L latches TMR1H (DS39632E §12.0); write
     * high byte first via the shadow, then low to commit both. */
    epic_sfr_write8(PIC_REG_TMR1H, (uint8_t)(value >> 8));
    epic_sfr_write8(PIC_REG_TMR1L, (uint8_t)(value & 0xFFU));
}

/**
 * @brief  Map a Timer1 prescaler enum to its ratio (DS39632E Register
 *         12-1: 00 -> 1:1, 01 -> 1:2, 10 -> 1:4, 11 -> 1:8).
 * @param p Prescaler selection (T1CKPS<1:0> value).
 * @return The divide ratio (1..8), or 1 for an out-of-range value.
 */
uint16_t EPIC_TIMER1_PrescalerToRatio(TIMER1_PrescalerTypeDef p)
{
    if ((unsigned)p > 3U) return 1U;
    return ps_ratio[p];
}

/**
 * @brief  Initialize Timer1 from a handle: stop the timer, clear TMR1IF,
 *         enable the overflow interrupt if a callback is given, enable
 *         16-bit read/write mode (RD16) and store an owned copy of the
 *         handle.
 * @param h Handle describing the Timer1 configuration.
 * @return EPIC_OK on success, EPIC_INVALID if `h` is NULL.
 */
EPIC_StatusTypeDef EPIC_TIMER1_Init(const TIMER1_HandleTypeDef *h)
{
    if (!h) return EPIC_INVALID;

    /* Stop the timer before reconfiguring. */
    EPIC_BIT_CLR(EPIC_REG8(PIC_REG_T1CON), PIC_T1CON_TMR1ON);

    /* Configure the overflow interrupt. */
    EPIC_IRQ_ClearFlag(PIC18_IRQ_TMR1);
    if (h->OverflowCallback) {
        EPIC_IRQ_Enable(PIC18_IRQ_TMR1);
    } else {
        EPIC_IRQ_DisableSrc(PIC18_IRQ_TMR1);
    }

    /* Enable 16-bit read/write mode (RD16) so the atomic 16-bit idiom works
     * the same way it always does on PIC16. */
    EPIC_BIT_SET(EPIC_REG8(PIC_REG_T1CON), PIC_T1CON_RD16);

    g_t1_storage = *h;
    g_t1_handle = &g_t1_storage;
    return EPIC_OK;
}

/**
 * @brief  De-initialize Timer1: disable its interrupt, clear the flag,
 *         restore T1CON to its power-on value, zero TMR1H/TMR1L and drop
 *         the stored handle.
 * @return EPIC_OK.
 */
EPIC_StatusTypeDef EPIC_TIMER1_DeInit(void)
{
    EPIC_IRQ_DisableSrc(PIC18_IRQ_TMR1);
    EPIC_IRQ_ClearFlag(PIC18_IRQ_TMR1);
    EPIC_REG8(PIC_REG_T1CON) = PIC_T1CON_POR_VALUE;
    EPIC_REG8(PIC_REG_TMR1H) = 0x00U;
    EPIC_REG8(PIC_REG_TMR1L) = 0x00U;
    g_t1_handle = NULL;
    return EPIC_OK;
}

/**
 * @brief  Start Timer1: load the reload value and program T1CON
 *         (prescaler, oscillator enable, sync, clock source) in one
 *         write, setting TMR1ON last.
 * @param h Handle whose configuration is applied.
 * @return EPIC_OK on success, EPIC_INVALID if `h` is NULL.
 */
EPIC_StatusTypeDef EPIC_TIMER1_Start(const TIMER1_HandleTypeDef *h)
{
    if (!h) return EPIC_INVALID;

    EPIC_TIMER1_WriteCounter(h->ReloadValue);

    /* Program T1CON in one write. RD16 stays set (from Init), T1RUN is
     * read-only and left clear.
     *   T1CKPS1:T1CKPS0 -> bits 5:4
     *   T1OSCEN          -> bit 3
     *   T1SYNC           -> bit 2
     *   TMR1CS           -> bit 1
     *   TMR1ON           -> bit 0 (set last) */
    uint8_t v = PIC_T1CON_RD16;     /* keep 16-bit mode */
    v |= (uint8_t)((h->Prescaler & 0x3U) << 4);
    if (h->Oscillator  == TIMER1_OSCILLATOR_ON)   v |= PIC_T1CON_T1OSCEN;
    if (h->ClockSync   == TIMER1_ASYNC_EXTERNAL) v |= PIC_T1CON_T1SYNC;
    if (h->ClockSource == TIMER1_CLOCK_EXTERNAL)  v |= PIC_T1CON_TMR1CS;
    v |= PIC_T1CON_TMR1ON;
    EPIC_REG8(PIC_REG_T1CON) = v;

    return EPIC_OK;
}

/**
 * @brief  Stop Timer1 by clearing TMR1ON.
 * @return EPIC_OK.
 */
EPIC_StatusTypeDef EPIC_TIMER1_Stop(void)
{
    EPIC_BIT_CLR(EPIC_REG8(PIC_REG_T1CON), PIC_T1CON_TMR1ON);
    return EPIC_OK;
}

/**
 * @brief  Weak Timer1 interrupt handler: clears TMR1IF and invokes the
 *         overflow callback registered via Init.
 */
void TIMER1_IRQHandler(void)
{
    if (!EPIC_IRQ_GetFlag(PIC18_IRQ_TMR1)) return;
    EPIC_IRQ_ClearFlag(PIC18_IRQ_TMR1);
    if (g_t1_handle && g_t1_handle->OverflowCallback) {
        g_t1_handle->OverflowCallback();
    }
}
