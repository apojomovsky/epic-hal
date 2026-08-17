/* Oscillator driver implementation (DS40001291H §4.0). OSCCON/OSCTUNE
 * are Bank 1. */

#include "peripherals/pic16f88x_osc.h"
#include "core/pic16_irq.h"

static void (*g_osf_cb)(void) = NULL;

/**
 * @brief Read OSCCON through the banked path.
 * @return the OSCCON value.
 */
static uint8_t osccon_read(void)
{
#ifdef EPIC_BANK1_READ8
    uint8_t v = 0u;
    EPIC_BANK1_READ8(OSCCON, v);
    return v;
#else
    return EPIC_REG8(PIC_REG_OSCCON);
#endif
}

/**
 * @brief Write OSCCON through the banked path.
 * @param v the value to write.
 */
static void osccon_write(uint8_t v)
{
#ifdef EPIC_BANK1_WRITE8
    EPIC_BANK1_WRITE8(OSCCON, v);
#else
    uint8_t prev = (EPIC_REG8(PIC_REG_STATUS) >> 5) & 0x03U;
    pic_select_bank(1);
    EPIC_REG8(PIC_REG_OSCCON) = v;
    pic_select_bank(prev);
#endif
}

/**
 * @brief Set the internal oscillator frequency (OSCCON<IRCF2:IRCF0>).
 * @param freq the frequency select.
 */
void EPIC_OSC_SetInternalFreq(OSC_InternalFreqTypeDef freq)
{
    uint8_t v = osccon_read();
    v = (uint8_t)((v & (uint8_t)~PIC_OSCCON_IRCF_MASK) |
                  (((uint8_t)freq & 0x07U) << PIC_OSCCON_IRCF_POS));
    osccon_write(v);
}

/**
 * @brief Read the current internal-oscillator frequency select.
 * @return the IRCF<2:0> value.
 */
uint8_t EPIC_OSC_GetInternalFreq(void)
{
    return (uint8_t)((osccon_read() & PIC_OSCCON_IRCF_MASK) >>
                     PIC_OSCCON_IRCF_POS);
}

/**
 * @brief Select the system clock source (OSCCON<SCS>).
 * @param internal 1 = internal oscillator, 0 = FOSC<2:0> config bits.
 */
void EPIC_OSC_SetSystemClockSource(uint8_t internal)
{
    uint8_t v = osccon_read();
    if (internal) v |= PIC_OSCCON_SCS;
    else          v &= (uint8_t)~PIC_OSCCON_SCS;
    osccon_write(v);
}

/**
 * @brief Tune the HFINTOSC frequency (OSCTUNE<TUN4:TUN0>).
 * @param tune the tuning value, 0x00 = factory calibration.
 */
void EPIC_OSC_Tune(uint8_t tune)
{
#ifdef EPIC_BANK1_WRITE8
    EPIC_BANK1_WRITE8(OSCTUNE, (uint8_t)(tune & PIC_OSCTUNE_TUN_MASK));
#else
    uint8_t prev = (EPIC_REG8(PIC_REG_STATUS) >> 5) & 0x03U;
    pic_select_bank(1);
    EPIC_REG8(PIC_REG_OSCTUNE) = (uint8_t)(tune & PIC_OSCTUNE_TUN_MASK);
    pic_select_bank(prev);
#endif
}

/**
 * @brief Enable the fail-safe clock monitor interrupt (PIE2<OSFIE>).
 * @param callback optional fail-safe callback, or NULL for polling.
 * @return EPIC_OK on success.
 */
EPIC_StatusTypeDef EPIC_OSC_FailSafeInit(void (*callback)(void))
{
    g_osf_cb = callback;
    EPIC_IRQ_ClearFlag(PIC16_IRQ_OSF);
    if (callback) EPIC_IRQ_Enable(PIC16_IRQ_OSF);
    else          EPIC_IRQ_DisableSrc(PIC16_IRQ_OSF);
    return EPIC_OK;
}

/**
 * @brief De-initialize the fail-safe interrupt.
 * @return EPIC_OK on success.
 */
EPIC_StatusTypeDef EPIC_OSC_FailSafeDeInit(void)
{
    EPIC_IRQ_DisableSrc(PIC16_IRQ_OSF);
    EPIC_IRQ_ClearFlag(PIC16_IRQ_OSF);
    g_osf_cb = NULL;
    return EPIC_OK;
}

/**
 * @brief Returns 1 if an oscillator failure was detected (PIR2<OSFIF>).
 * @return 1 if the fail flag is set, 0 otherwise.
 */
uint8_t EPIC_OSC_IsFailSafe(void)
{
    return EPIC_IRQ_GetFlag(PIC16_IRQ_OSF);
}

/**
 * @brief Clear the oscillator-fail flag (PIR2<OSFIF>).
 */
void EPIC_OSC_ClearFailSafeFlag(void)
{
    EPIC_IRQ_ClearFlag(PIC16_IRQ_OSF);
}

/**
 * @brief Weak oscillator-fail ISR: clears OSFIF and fires the callback.
 */
void OSF_IRQHandler(void)
{
    /* Direct flag ops (class-F). OSFIF is PIR2 bit 7. */
    if (!(EPIC_REG8(PIC_REG_PIR2) & PIC_PIR2_OSFIF)) return;
    EPIC_BIT_CLR(EPIC_REG8(PIC_REG_PIR2), PIC_PIR2_OSFIF);
    if (g_osf_cb) g_osf_cb();
}
