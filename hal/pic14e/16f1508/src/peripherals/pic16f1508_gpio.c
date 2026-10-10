#include "peripherals/pic16f1508_gpio.h"

/* Set by EPIC_GPIO_RegisterChangeCallback, run from IOC_IRQHandler. */
static void (*s_ioc_callback)(uint8_t iocbf, uint8_t portb);

/**
 * @brief Implemented-pin mask for a port.
 * @param port GPIO port.
 * @return Pin mask, or 0 for an unknown port (callers then do nothing).
 */
static uint8_t port_mask(GPIO_TypeDef port)
{
    switch (port)
    {
    case GPIOA: return PIC16F1508_FAMILY_PORTA_MASK;
    case GPIOB: return PIC16F1508_FAMILY_PORTB_MASK;
    case GPIOC: return PIC16F1508_FAMILY_PORTC_MASK;
    default:    return 0U;
    }
}

/**
 * @brief TRIS register address of a port.
 * @param port GPIO port (validated by the caller via port_mask).
 * @return SFR address.
 */
static uint16_t tris_addr(GPIO_TypeDef port)
{
    switch (port)
    {
    case GPIOA: return PIC_REG_TRISA;
    case GPIOB: return PIC_REG_TRISB;
    default:    return PIC_REG_TRISC;
    }
}

/**
 * @brief LAT register address of a port.
 * @param port GPIO port (validated by the caller via port_mask).
 * @return SFR address.
 */
static uint16_t lat_addr(GPIO_TypeDef port)
{
    switch (port)
    {
    case GPIOA: return PIC_REG_LATA;
    case GPIOB: return PIC_REG_LATB;
    default:    return PIC_REG_LATC;
    }
}

/**
 * @brief PORT register address of a port.
 * @param port GPIO port (validated by the caller via port_mask).
 * @return SFR address.
 */
static uint16_t port_addr(GPIO_TypeDef port)
{
    switch (port)
    {
    case GPIOA: return PIC_REG_PORTA;
    case GPIOB: return PIC_REG_PORTB;
    default:    return PIC_REG_PORTC;
    }
}

/**
 * @brief ANSEL register address of a port.
 * @param port GPIO port (validated by the caller via port_mask).
 * @return SFR address.
 */
static uint16_t ansel_addr(GPIO_TypeDef port)
{
    switch (port)
    {
    case GPIOA: return PIC_REG_ANSELA;
    case GPIOB: return PIC_REG_ANSELB;
    default:    return PIC_REG_ANSELC;
    }
}

/**
 * @brief Weak pull-up register address of a port.
 * @param port GPIO port.
 * @return SFR address, or 0 when the port has no WPU register (PORTC).
 */
static uint16_t wpu_addr(GPIO_TypeDef port)
{
    switch (port)
    {
    case GPIOA: return PIC_REG_WPUA;
    case GPIOB: return PIC_REG_WPUB;
    default:    return 0U;
    }
}

/**
 * @brief Set the direction and digital or analog mode of pins on a port.
 * @param port GPIO port.
 * @param pins Pin mask (GPIO_PIN_*), limited to the port's implemented pins.
 * @param mode GPIO_MODE_INPUT, GPIO_MODE_OUTPUT (latch cleared first), or
 *        GPIO_MODE_ANALOG.
 */
void EPIC_GPIO_Init(GPIO_TypeDef port, uint16_t pins, GPIO_ModeTypeDef mode)
{
    uint8_t mask = (uint8_t)(pins & port_mask(port));
    if (mask == 0U)
    {
        return;
    }

    switch (mode)
    {
    case GPIO_MODE_INPUT:
        EPIC_REG8(tris_addr(port)) |= mask;
        EPIC_REG8(ansel_addr(port)) &= (uint8_t)~mask;
        break;
    case GPIO_MODE_OUTPUT:
        EPIC_REG8(tris_addr(port)) &= (uint8_t)~mask;
        EPIC_REG8(ansel_addr(port)) &= (uint8_t)~mask;
        EPIC_REG8(lat_addr(port)) &= (uint8_t)~mask;
        break;
    case GPIO_MODE_ANALOG:
        EPIC_REG8(tris_addr(port)) |= mask;
        EPIC_REG8(ansel_addr(port)) |= mask;
        break;
    default:
        break;
    }
}

/**
 * @brief Restore the implemented pins of a port to reset: input, digital,
 *        latch low.
 * @param port GPIO port.
 */
void EPIC_GPIO_DeInit(GPIO_TypeDef port)
{
    uint8_t mask = port_mask(port);
    if (mask == 0U)
    {
        return;
    }

    EPIC_REG8(tris_addr(port)) = mask;
    EPIC_REG8(ansel_addr(port)) = mask;
    EPIC_REG8(lat_addr(port)) = 0U;
}

/**
 * @brief Drive pins high or low by setting or clearing their LATx bits.
 * @param port GPIO port.
 * @param pins Pin mask (GPIO_PIN_*).
 * @param state GPIO_PIN_SET drives high, GPIO_PIN_RESET drives low.
 */
void EPIC_GPIO_WritePin(GPIO_TypeDef port, uint16_t pins, GPIO_PinState state)
{
    uint8_t mask = (uint8_t)(pins & port_mask(port));
    if (mask == 0U)
    {
        return;
    }

    if (state == GPIO_PIN_SET)
    {
        EPIC_REG8(lat_addr(port)) |= mask;
    }
    else
    {
        EPIC_REG8(lat_addr(port)) &= (uint8_t)~mask;
    }
}

/**
 * @brief Toggle the LATx latch bits of the given pins.
 * @param port GPIO port.
 * @param pins Pin mask (GPIO_PIN_*).
 */
void EPIC_GPIO_TogglePin(GPIO_TypeDef port, uint16_t pins)
{
    uint8_t mask = (uint8_t)(pins & port_mask(port));
    if (mask == 0U)
    {
        return;
    }

    EPIC_REG8(lat_addr(port)) ^= mask;
}

/**
 * @brief Read the level of the given pins from PORTx.
 * @param port GPIO port.
 * @param pins Pin mask (GPIO_PIN_*).
 * @return GPIO_PIN_SET if any masked pin reads high, else GPIO_PIN_RESET.
 */
GPIO_PinState EPIC_GPIO_ReadPin(GPIO_TypeDef port, uint16_t pins)
{
    uint8_t mask = (uint8_t)(pins & port_mask(port));
    if (mask == 0U)
    {
        return GPIO_PIN_RESET;
    }

    return ((EPIC_REG8(port_addr(port)) & mask) != 0U) ? GPIO_PIN_SET : GPIO_PIN_RESET;
}

/**
 * @brief Write the implemented bits of a port's LATx latch in one access.
 * @param port GPIO port.
 * @param value Byte to write. Bits outside the port's implemented mask
 *        are ignored and the unimplemented LAT bits are kept.
 */
void EPIC_GPIO_WritePort(GPIO_TypeDef port, uint8_t value)
{
    uint8_t mask = port_mask(port);
    if (mask == 0U)
    {
        return;
    }

    EPIC_REG8(lat_addr(port)) = (uint8_t)((EPIC_REG8(lat_addr(port)) & (uint8_t)~mask) |
                                          (value & mask));
}

/**
 * @brief Read the pin levels of a port, masked to its implemented pins.
 * @param port GPIO port.
 * @return PORTx register value masked to the port's implemented pins.
 */
uint8_t EPIC_GPIO_ReadPort(GPIO_TypeDef port)
{
    uint8_t mask = port_mask(port);
    if (mask == 0U)
    {
        return 0U;
    }

    return (uint8_t)(EPIC_REG8(port_addr(port)) & mask);
}

/**
 * @brief Enable or disable the weak pull-up on pins of PORTA or PORTB.
 * @details Setting a pull-up clears the global nWPUEN bit. Clearing one sets
 *          nWPUEN again only when WPUA and WPUB are both zero. PORTC has no
 *          WPU register in this HAL, so the call is a no-op for it.
 * @param port GPIO port.
 * @param pins Pin mask (GPIO_PIN_*).
 * @param state GPIO_PIN_SET enables the pull-ups, GPIO_PIN_RESET disables.
 */
void EPIC_GPIO_SetPullups(GPIO_TypeDef port, uint16_t pins, GPIO_PinState state)
{
    uint8_t mask = (uint8_t)(pins & port_mask(port));
    uint16_t wpu = wpu_addr(port);
    if ((mask == 0U) || (wpu == 0U))
    {
        return;
    }

    if (state == GPIO_PIN_SET)
    {
        EPIC_REG8(wpu) |= mask;
        EPIC_BIT_CLR(EPIC_REG8(PIC_REG_OPTION), PIC_OPTION_nWPUEN);
    }
    else
    {
        EPIC_REG8(wpu) &= (uint8_t)~mask;
        if ((EPIC_REG8(PIC_REG_WPUA) | EPIC_REG8(PIC_REG_WPUB)) == 0U)
        {
            EPIC_BIT_SET(EPIC_REG8(PIC_REG_OPTION), PIC_OPTION_nWPUEN);
        }
    }
}

/**
 * @brief Register the callback run from IOC_IRQHandler, or NULL to
 *        unregister it.
 * @param callback Called with the IOCBF mask and the PORTB byte read in the
 *        handler.
 */
void EPIC_GPIO_RegisterChangeCallback(void (*callback)(uint8_t iocbf, uint8_t portb))
{
    s_ioc_callback = callback;
}

/**
 * @brief Set the rising and falling edge masks for PORTB interrupt-on-change.
 * @param pos_mask Pins that trigger on a rising edge (IOCBP). RB4..RB7 only.
 * @param neg_mask Pins that trigger on a falling edge (IOCBN). RB4..RB7 only.
 */
void EPIC_GPIO_EnableChangeDetect(uint8_t pos_mask, uint8_t neg_mask)
{
    EPIC_REG8(PIC_REG_IOCBP) = (uint8_t)(pos_mask & PIC16F1508_FAMILY_IOCB_MASK);
    EPIC_REG8(PIC_REG_IOCBN) = (uint8_t)(neg_mask & PIC16F1508_FAMILY_IOCB_MASK);
}

/**
 * @brief Interrupt-on-change ISR: capture and clear the PORTB IOCBF flags,
 *        then forward them to the registered callback.
 * @details Returns unless INTCON.IOCIF is set. Each captured bit is cleared
 *          by a single-bit clear, so a flag set by hardware meanwhile is kept
 *          and re-raises IOCIF. A second edge on a captured bit merges into
 *          that capture; the callback's PORTB byte is read after the clear.
 */
void IOC_IRQHandler(void)
{
    uint8_t iocbf;
    uint8_t portb;

    if ((EPIC_REG8(PIC_REG_INTCON) & PIC_INTCON_IOCIF) == 0U)
    {
        return;
    }

    /* Each captured bit gets its own single-bit clear: a masked write of the
     * whole register can overwrite a flag hardware sets in its load/store
     * window. PORTB is read after the clears, so its byte is never older
     * than the flags being consumed. */
    iocbf = EPIC_REG8(PIC_REG_IOCBF);
    if ((iocbf & EPIC_BIT(4)) != 0U)
    {
        EPIC_BIT_CLR(EPIC_REG8(PIC_REG_IOCBF), EPIC_BIT(4));
    }
    if ((iocbf & EPIC_BIT(5)) != 0U)
    {
        EPIC_BIT_CLR(EPIC_REG8(PIC_REG_IOCBF), EPIC_BIT(5));
    }
    if ((iocbf & EPIC_BIT(6)) != 0U)
    {
        EPIC_BIT_CLR(EPIC_REG8(PIC_REG_IOCBF), EPIC_BIT(6));
    }
    if ((iocbf & EPIC_BIT(7)) != 0U)
    {
        EPIC_BIT_CLR(EPIC_REG8(PIC_REG_IOCBF), EPIC_BIT(7));
    }
    portb = EPIC_REG8(PIC_REG_PORTB);

    /* The sim models IOCIF as RAM, so this clear is required there. */
    EPIC_BIT_CLR(EPIC_REG8(PIC_REG_INTCON), PIC_INTCON_IOCIF);

    if (s_ioc_callback != 0)
    {
        s_ioc_callback(iocbf, portb);
    }
}
