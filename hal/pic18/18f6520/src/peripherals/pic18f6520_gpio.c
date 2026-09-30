/*
 * GPIO driver, implementation (DS39609B §10.0). PIC18 exposes the output
 * latch directly (LATx), so writes are plain stores with no PIC16
 * read-modify-write PORTx hazard; reads come from PORTx, the pin
 * input sample. Register addresses dispatch on a literal `port` value
 * through PIC_REG_* tokens (the §4 runtime-address rule); each branch
 * returns a compile-time-constant address.
 */

#include "peripherals/pic18f6520_gpio.h"
#include "core/pic18_irq.h"

/**
 * @brief  Return the TRISx register address for a port.
 * @param port Port whose direction register is wanted.
 * @return The TRISx SFR address for `port`.
 */
static uint16_t tris_addr(GPIO_TypeDef port)
{
    switch (port)
    {
        case GPIOA: return PIC_REG_TRISA;
        case GPIOB: return PIC_REG_TRISB;
        case GPIOC: return PIC_REG_TRISC;
        case GPIOD: return PIC_REG_TRISD;
        case GPIOE: return PIC_REG_TRISE;
        case GPIOF: return PIC_REG_TRISF;
        case GPIOG: return PIC_REG_TRISG;
#if PIC18F6520_FAMILY_HAS_PORTH
        case GPIOH: return PIC_REG_TRISH;
#endif
#if PIC18F6520_FAMILY_HAS_PORTJ
        case GPIOJ: return PIC_REG_TRISJ;
#endif
        default:    return PIC_REG_TRISA;
    }
}

/**
 * @brief  Return the LATx output-latch register address for a port.
 * @param port Port whose latch register is wanted.
 * @return The LATx SFR address for `port`.
 */
static uint16_t lat_addr(GPIO_TypeDef port)
{
    switch (port)
    {
        case GPIOA: return PIC_REG_LATA;
        case GPIOB: return PIC_REG_LATB;
        case GPIOC: return PIC_REG_LATC;
        case GPIOD: return PIC_REG_LATD;
        case GPIOE: return PIC_REG_LATE;
        case GPIOF: return PIC_REG_LATF;
        case GPIOG: return PIC_REG_LATG;
#if PIC18F6520_FAMILY_HAS_PORTH
        case GPIOH: return PIC_REG_LATH;
#endif
#if PIC18F6520_FAMILY_HAS_PORTJ
        case GPIOJ: return PIC_REG_LATJ;
#endif
        default:    return PIC_REG_LATA;
    }
}

/**
 * @brief  Return the PORTx input register address for a port.
 * @param port Port whose input register is wanted.
 * @return The PORTx SFR address for `port`.
 */
static uint16_t port_addr(GPIO_TypeDef port)
{
    switch (port)
    {
        case GPIOA: return PIC_REG_PORTA;
        case GPIOB: return PIC_REG_PORTB;
        case GPIOC: return PIC_REG_PORTC;
        case GPIOD: return PIC_REG_PORTD;
        case GPIOE: return PIC_REG_PORTE;
        case GPIOF: return PIC_REG_PORTF;
        case GPIOG: return PIC_REG_PORTG;
#if PIC18F6520_FAMILY_HAS_PORTH
        case GPIOH: return PIC_REG_PORTH;
#endif
#if PIC18F6520_FAMILY_HAS_PORTJ
        case GPIOJ: return PIC_REG_PORTJ;
#endif
        default:    return PIC_REG_PORTA;
    }
}

/**
 * @brief  Initialize a set of pins on a port to a given direction.
 * @param port Port to configure (a @ref GPIO_TypeDef).
 * @param pins Bitmask of the pins to configure (GPIO_PIN_*).
 * @param mode A @ref GPIO_ModeTypeDef (input, output, or analog).
 */
void EPIC_GPIO_Init(GPIO_TypeDef port, uint16_t pins, GPIO_ModeTypeDef mode)
{
    uint8_t mask = (uint8_t)pins;
    uint16_t tr = tris_addr(port);

    if (mode == GPIO_MODE_OUTPUT)
    {
        uint8_t t = EPIC_REG8(tr);
        EPIC_REG8(tr) = (uint8_t)(t & (uint8_t)~mask);
    }
    else
    {
        uint8_t t = EPIC_REG8(tr);
        EPIC_REG8(tr) = (uint8_t)(t | mask);
    }
}

/**
 * @brief  Reset a set of pins to their default state (all inputs).
 * @param port Port to reset (a @ref GPIO_TypeDef).
 * @param pins Bitmask of the pins to reset (GPIO_PIN_*).
 */
void EPIC_GPIO_DeInit(GPIO_TypeDef port, uint16_t pins)
{
    uint8_t mask = (uint8_t)pins;
    uint8_t t = EPIC_REG8(tris_addr(port));
    EPIC_REG8(tris_addr(port)) = (uint8_t)(t | mask);
}

/**
 * @brief  Write all pins of a port from a bitmask (writes LATx).
 * @param port Port to write (a @ref GPIO_TypeDef).
 * @param value Bitmask of the pin levels to drive (1 = high).
 */
void EPIC_GPIO_WritePort(GPIO_TypeDef port, uint16_t value)
{
    EPIC_REG8(lat_addr(port)) = (uint8_t)value;
}

/**
 * @brief  Write one or more pins on a port to a level (writes LATx).
 * @param port Port to write (a @ref GPIO_TypeDef).
 * @param pins Bitmask of the pins to change (GPIO_PIN_*).
 * @param state @ref GPIO_PIN_SET or @ref GPIO_PIN_RESET.
 */
void EPIC_GPIO_WritePin(GPIO_TypeDef port, uint16_t pins, GPIO_PinState state)
{
    uint8_t mask = (uint8_t)pins;
    uint16_t la = lat_addr(port);
    uint8_t v = EPIC_REG8(la);
    EPIC_REG8(la) = (state == GPIO_PIN_SET)
                        ? (uint8_t)(v | mask)
                        : (uint8_t)(v & (uint8_t)~mask);
}

/**
 * @brief  Toggle one or more pins on a port (XOR into LATx).
 * @param port Port whose pins to toggle (a @ref GPIO_TypeDef).
 * @param pins Bitmask of the pins to toggle (GPIO_PIN_*).
 */
void EPIC_GPIO_TogglePin(GPIO_TypeDef port, uint16_t pins)
{
    uint16_t la = lat_addr(port);
    uint8_t v = EPIC_REG8(la);
    EPIC_REG8(la) = (uint8_t)(v ^ (uint8_t)pins);
}

/**
 * @brief  Read one pin's logical level from PORTx.
 * @param port Port to read (a @ref GPIO_TypeDef).
 * @param pins Single pin to read (a GPIO_PIN_* value).
 * @return @ref GPIO_PIN_SET if the pin reads high, @ref GPIO_PIN_RESET if low.
 */
GPIO_PinState EPIC_GPIO_ReadPin(GPIO_TypeDef port, uint16_t pins)
{
    return (EPIC_REG8(port_addr(port)) & (uint8_t)pins)
               ? GPIO_PIN_SET : GPIO_PIN_RESET;
}

/**
 * @brief  Read the whole port byte from PORTx.
 * @param port Port to read (a @ref GPIO_TypeDef).
 * @return the raw PORTx byte.
 */
uint8_t EPIC_GPIO_ReadPort(GPIO_TypeDef port)
{
    return EPIC_REG8(port_addr(port));
}

/**
 * @brief  Enable or disable the PORTB weak pull-ups (INTCON2<RBPU>).
 * @param pull @ref GPIO_PULLUP to enable, @ref GPIO_NOPULL to disable.
 */
void EPIC_GPIO_SetPullups(GPIO_PullTypeDef pull)
{
    if (pull == GPIO_PULLUP)
    {
        EPIC_BIT_CLR(EPIC_REG8(PIC_REG_INTCON2), PIC_INTCON2_RBPU);
    }
    else
    {
        EPIC_BIT_SET(EPIC_REG8(PIC_REG_INTCON2), PIC_INTCON2_RBPU);
    }
}

/* Slot for the single PORTB change-interrupt callback. */
static void (*s_rb_change_callback)(uint8_t) = 0;

/**
 * @brief  Register the single whole-port callback fired from the RB<7:4>
 *         change interrupt. NULL unregisters.
 * @param callback Function called once per RB-change interrupt with the
 *                 PORTB byte, or NULL.
 */
void EPIC_GPIO_RegisterChangeCallback(void (*callback)(uint8_t))
{
    s_rb_change_callback = callback;
}

/**
 * @brief  Weak RB<7:4> change-interrupt ISR. Reads PORTB into a local,
 *         clears RBIF, then forwards the value to the callback from
 *         RegisterChangeCallback. The read-before-clear order is
 *         mandatory (DS39609B §9.0): the mismatch comparator latches the
 *         value at the last CPU read of PORTB, so the read ends the
 *         mismatch condition and re-arms the next change.
 */
void RB_IRQHandler(void)
{
    if (!EPIC_IRQ_GetFlag(PIC18_IRQ_RB)) return;

    /* MUST read PORTB before clearing RBIF (DS39609B §9.0): the mismatch
     * comparator latches the value at the last CPU read of PORTB, so the
     * read is what ends the mismatch condition and re-arms the next one.
     * Clearing first risks a spurious re-interrupt or a missed change. */
    uint8_t portb = EPIC_REG8(PIC_REG_PORTB);
    EPIC_IRQ_ClearFlag(PIC18_IRQ_RB);
    if (s_rb_change_callback) s_rb_change_callback(portb);
}
