/**
 * NCO1 driver, implementation (DS40001802D §24). Every access names its
 * register literally; the handler keeps no cstack autos (ISR bank rule,
 * family MANUAL.md). Control, clock and increment writes go through
 * epic_sfr_write8 so the host simulator models the increment double
 * buffer and N1OUT. The callback slot is main-context-written only.
 */
#include <stddef.h>
#include "peripherals/pic16f188xx_nco.h"
#include "core/pic16f188xx_irq.h"

/* ISR callback slot, written from main context only. A scalar, not an
 * array element: the handler references a fixed symbol. */
static void (*s_nco_overflow_cb)(void) = NULL;

/**
 * @brief Check the clock, mode and pulse-width fields and the increment.
 * @param h Handle to check.
 * @return 1 when every field is in range, 0 otherwise.
 */
static uint8_t fields_ok(const NCO_HandleTypeDef *h)
{
    return (((unsigned int)h->ClockSource <= (unsigned int)NCO_CLK_LC4OUT) &&
            ((unsigned int)h->Mode <= (unsigned int)NCO_MODE_PFM) &&
            ((unsigned int)h->PulseWidth <= (unsigned int)NCO_PULSE_128) &&
            (h->Increment <= NCO_VALUE_MAX)) ? 1U : 0U;
}

/**
 * @brief Write the 20-bit increment, upper bytes first.
 * @details NCO1INCL goes last: it loads the double buffer (§24.1.4).
 * @param increment Value already checked against NCO_VALUE_MAX.
 */
static void write_increment(uint32_t increment)
{
    epic_sfr_write8(PIC_REG_NCO1INCU, (uint8_t)((increment >> 16) & 0x0FU));
    epic_sfr_write8(PIC_REG_NCO1INCH, (uint8_t)(increment >> 8));
    epic_sfr_write8(PIC_REG_NCO1INCL, (uint8_t)increment);
}

/**
 * @brief Configure NCO1 from the handle, leaving it disabled.
 * @details NCO1CON is written first with N1EN clear, so no half-written
 * setup reaches the output.
 * @param h Handle with clock, mode, pulse width, polarity, increment and callback.
 * @return EPIC_OK on success, EPIC_INVALID for a NULL or out-of-range handle.
 */
EPIC_StatusTypeDef EPIC_NCO_Init(const NCO_HandleTypeDef *h)
{
    uint8_t con;

    if ((h == NULL) || (fields_ok(h) == 0U))
    {
        return EPIC_INVALID;
    }

    con = (uint8_t)h->Mode;
    if (h->Invert != 0U)
    {
        con |= PIC_NCO1CON_N1POL;
    }
    epic_sfr_write8(PIC_REG_NCO1CON, con);
    epic_sfr_write8(PIC_REG_NCO1CLK, (uint8_t)((((unsigned int)h->PulseWidth <<
        PIC_NCO1CLK_N1PWS_SHIFT) & PIC_NCO1CLK_N1PWS_MASK) |
        ((uint8_t)h->ClockSource & PIC_NCO1CLK_N1CKS_MASK)));
    write_increment(h->Increment);
    EPIC_IRQ_ClearFlag(PIC16F188XX_IRQ_NCO1);
    s_nco_overflow_cb = h->OverflowCallback;
    if (h->OverflowCallback != NULL)
    {
        EPIC_IRQ_Enable(PIC16F188XX_IRQ_NCO1);
    }
    else
    {
        EPIC_IRQ_DisableSrc(PIC16F188XX_IRQ_NCO1);
    }
    return EPIC_OK;
}

/**
 * @brief Disable NCO1 and restore the POR state.
 * @details Every NCO register powers up 0 except NCO1INCL, which is 1
 * (EDC por; DS40001802D Table 24-1).
 * @return EPIC_OK on success.
 */
EPIC_StatusTypeDef EPIC_NCO_DeInit(void)
{
    EPIC_IRQ_DisableSrc(PIC16F188XX_IRQ_NCO1);
    EPIC_IRQ_ClearFlag(PIC16F188XX_IRQ_NCO1);
    epic_sfr_write8(PIC_REG_NCO1CON, 0U);
    epic_sfr_write8(PIC_REG_NCO1CLK, 0U);
    EPIC_REG8(PIC_REG_NCO1ACCU) = 0U;
    EPIC_REG8(PIC_REG_NCO1ACCH) = 0U;
    EPIC_REG8(PIC_REG_NCO1ACCL) = 0U;
    write_increment(1UL);
    s_nco_overflow_cb = NULL;
    return EPIC_OK;
}

/**
 * @brief Set N1EN.
 * @return EPIC_OK on success.
 */
EPIC_StatusTypeDef EPIC_NCO_Start(void)
{
    epic_sfr_write8(PIC_REG_NCO1CON,
        (uint8_t)(EPIC_REG8(PIC_REG_NCO1CON) | PIC_NCO1CON_N1EN));
    return EPIC_OK;
}

/**
 * @brief Clear N1EN.
 * @return EPIC_OK on success.
 */
EPIC_StatusTypeDef EPIC_NCO_Stop(void)
{
    epic_sfr_write8(PIC_REG_NCO1CON,
        (uint8_t)(EPIC_REG8(PIC_REG_NCO1CON) & (uint8_t)~PIC_NCO1CON_N1EN));
    return EPIC_OK;
}

/**
 * @brief Change the increment, also while running.
 * @param increment New increment, 0..NCO_VALUE_MAX.
 * @return EPIC_OK on success, EPIC_INVALID when above NCO_VALUE_MAX.
 */
EPIC_StatusTypeDef EPIC_NCO_SetIncrement(uint32_t increment)
{
    if (increment > NCO_VALUE_MAX)
    {
        return EPIC_INVALID;
    }
    write_increment(increment);
    return EPIC_OK;
}

/**
 * @brief Read the 20-bit increment.
 * @return NCO1INCU:NCO1INCH:NCO1INCL.
 */
uint32_t EPIC_NCO_GetIncrement(void)
{
    uint32_t value = EPIC_REG8(PIC_REG_NCO1INCL);

    value |= (uint32_t)EPIC_REG8(PIC_REG_NCO1INCH) << 8;
    value |= (uint32_t)(EPIC_REG8(PIC_REG_NCO1INCU) & 0x0FU) << 16;
    return value;
}

/**
 * @brief Read the 20-bit accumulator, low byte first.
 * @return NCO1ACCU:NCO1ACCH:NCO1ACCL.
 */
uint32_t EPIC_NCO_ReadAccumulator(void)
{
    uint32_t value = EPIC_REG8(PIC_REG_NCO1ACCL);

    value |= (uint32_t)EPIC_REG8(PIC_REG_NCO1ACCH) << 8;
    value |= (uint32_t)(EPIC_REG8(PIC_REG_NCO1ACCU) & 0x0FU) << 16;
    return value;
}

/**
 * @brief Load the accumulator while the NCO is stopped.
 * @param value New accumulator value, 0..NCO_VALUE_MAX.
 * @return EPIC_OK on success, EPIC_BUSY while N1EN is set, EPIC_INVALID
 *         when above NCO_VALUE_MAX.
 */
EPIC_StatusTypeDef EPIC_NCO_WriteAccumulator(uint32_t value)
{
    if (value > NCO_VALUE_MAX)
    {
        return EPIC_INVALID;
    }
    if ((EPIC_REG8(PIC_REG_NCO1CON) & PIC_NCO1CON_N1EN) != 0U)
    {
        return EPIC_BUSY;
    }
    EPIC_REG8(PIC_REG_NCO1ACCU) = (uint8_t)((value >> 16) & 0x0FU);
    EPIC_REG8(PIC_REG_NCO1ACCH) = (uint8_t)(value >> 8);
    EPIC_REG8(PIC_REG_NCO1ACCL) = (uint8_t)value;
    return EPIC_OK;
}

/**
 * @brief Read N1OUT.
 * @return 1 when the output is high, 0 otherwise.
 */
uint8_t EPIC_NCO_ReadOutput(void)
{
    return (EPIC_REG8(PIC_REG_NCO1CON) & PIC_NCO1CON_N1OUT) ? 1U : 0U;
}

/**
 * @brief Acknowledge NCO1IF, then run the overflow callback if one is set.
 * @details Called from epic_dispatch_all_irqs only when NCO1IF and NCO1IE
 * are both set. Clears NCO1IF before the callback so the vector does not
 * re-enter. No cstack autos: every access names its register literally.
 */
void NCO_IRQHandler(void)
{
    EPIC_IRQ_ClearFlag(PIC16F188XX_IRQ_NCO1);
    if (s_nco_overflow_cb != NULL)
    {
        s_nco_overflow_cb();
    }
}
