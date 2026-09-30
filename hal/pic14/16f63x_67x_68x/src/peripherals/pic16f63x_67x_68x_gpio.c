/* GPIO driver implementation (DS40001262F §4.0, DS40300 §4.0). The
 * 20-pin parts carry PORTA (RA0..RA5), PORTB (RB4..RB7) and full PORTC;
 * the 14-pin parts carry PORTA/PORTC only with RC0..RC5. ANSEL lives at
 * 0x11E (Bank 2) on 20-pin shapes and 0x91 (Bank 1) on the 14-pin ADC
 * shapes; the 630/639 have no ANSEL at all. */

#include "peripherals/pic16f63x_67x_68x_gpio.h"
#include "core/pic16_irq.h"

/**
 * @brief  Map a GPIO_TypeDef to the address of its TRIS register.
 * @param port GPIOA..GPIOC.
 * @return the TRIS SFR address (0x85..0x87).
 */
static uint8_t tris_addr(GPIO_TypeDef port)
{
    switch (port)
    {
        case GPIOA: return PIC_REG_TRISA;
#if PIC16F63X_67X_68X_FAMILY_HAS_PORTB
        case GPIOB: return PIC_REG_TRISB;
#endif
        case GPIOC: return PIC_REG_TRISC;
        default:    return PIC_REG_TRISA;
    }
}

/**
 * @brief Map a GPIO_TypeDef to the PORTx register address.
 * @param port GPIOA..GPIOC.
 * @return the PORTx SFR address.
 */
static uint8_t port_addr(GPIO_TypeDef port)
{
    switch (port)
    {
        case GPIOA: return PIC_REG_PORTA;
#if PIC16F63X_67X_68X_FAMILY_HAS_PORTB
        case GPIOB: return PIC_REG_PORTB;
#endif
        case GPIOC: return PIC_REG_PORTC;
        default:    return PIC_REG_PORTA;
    }
}

/**
 * @brief Mask off unimplemented bits of a port's TRIS/PORT byte.
 *        PORTB is gapped (RB0..RB3 do not exist), so this is a table,
 *        not a width shift.
 * @param port GPIOA..GPIOC.
 * @return the valid-pin mask.
 */
static uint8_t port_mask(GPIO_TypeDef port)
{
    switch (port)
    {
        case GPIOA: return 0x3FU;
#if PIC16F63X_67X_68X_FAMILY_HAS_PORTB
        case GPIOB: return 0xF0U;
        case GPIOC: return 0xFFU;
#else
        case GPIOC: return 0x3FU;
#endif
        default:    return 0x00U;
    }
}

/**
 * @brief  Map a pin to its ANSEL bit, if it has an analog function.
 *         20-pin map: RA0=ANS0, RA1=ANS1, RC0=ANS4, RC1=ANS5, RC2=ANS6,
 *         RC3=ANS7 (DS40001262F pin summary). 14-pin ADC map:
 *         RA0=ANS0, RA1=ANS1, RA2=ANS2, RA4=ANS3, RC0=ANS4, RC1=ANS5,
 *         RC2=ANS6, RC3=ANS7 (DS40300 pin summary).
 * @param port GPIOA..GPIOC.
 * @param pin the pin number, 0..7.
 * @return the ANSEL bit position, or 0xFF if the pin has no analog
 *         function.
 */
#if PIC16F63X_67X_68X_FAMILY_HAS_ANSEL
static uint8_t ansel_bit(GPIO_TypeDef port, uint8_t pin)
{
#if PIC16F63X_67X_68X_FAMILY_HAS_ANSEL_BANK1
    if (port == GPIOA)
    {
        if (pin <= 2U) return pin;
        if (pin == 4U) return 3U;
        return 0xFFU;
    }
#else
    if (port == GPIOA)
    {
        if (pin == 0U) return 0U;
        if (pin == 1U) return 1U;
        return 0xFFU;
    }
#endif
    if (port == GPIOC)
    {
        if (pin <= 3U) return (uint8_t)(pin + 4U);
        return 0xFFU;
    }
    /* PORTB has no analog functions. */
    return 0xFFU;
}
#endif

#if PIC16F63X_67X_68X_FAMILY_HAS_ANSELH
/**
 * @brief  Map a pin to its ANSELH bit, if it has an analog function.
 *         RB4=ANS10, RB5=ANS11, RC6=ANS8, RC7=ANS9 (677 only,
 *         DS40001262F pin summary).
 * @param port GPIOA..GPIOC.
 * @param pin the pin number, 0..7.
 * @return the ANSELH bit position, or 0xFF if the pin has no analog
 *         function.
 */
static uint8_t anselh_bit(GPIO_TypeDef port, uint8_t pin)
{
    if (port == GPIOB)
    {
        if (pin == 4U) return 2U;
        if (pin == 5U) return 3U;
        return 0xFFU;
    }
    if (port == GPIOC)
    {
        if (pin == 6U) return 0U;
        if (pin == 7U) return 1U;
        return 0xFFU;
    }
    /* PORTA has no ANSELH functions. */
    return 0xFFU;
}
#endif

/**
 * @brief  Clear or set the ANSEL (and ANSELH, where present) bits for
 *         a set of pins (Bank 2).
 * @param port GPIOA..GPIOC.
 * @param pins bitmask of pins.
 * @param analog 1 to set the bits (pin to analog), 0 to clear them
 *        (pin to digital).
 */
static void set_ansel_bits(GPIO_TypeDef port, uint16_t pins, uint8_t analog)
{
#if PIC16F63X_67X_68X_FAMILY_HAS_ANSEL
    uint8_t ansel = 0u;
#if PIC16F63X_67X_68X_FAMILY_HAS_ANSEL_BANK1
    /* The BANK1 literal-token macros stringify the token for inline
     * asm, so they need the real SFR name (XC8 resolves ANSEL to
     * 0x91 on these parts); the HAL-side ANSEL_BANK1 alias exists
     * only for the host/sim constant path below. */
#ifdef EPIC_BANK1_READ8
    EPIC_BANK1_READ8(ANSEL, ansel);
#else
    ansel = EPIC_REG8(PIC_REG_ANSEL_BANK1);
#endif
#else
#ifdef EPIC_BANK2_READ8
    EPIC_BANK2_READ8(ANSEL, ansel);
#else
    ansel = EPIC_REG8(PIC_REG_ANSEL);
#endif
#endif
    uint8_t changed = 0U;
    for (uint8_t pin = 0U; pin <= 7U; pin++)
    {
        if (!((uint16_t)EPIC_BIT(pin) & pins)) continue;
        uint8_t bit = ansel_bit(port, pin);
        if (bit == 0xFFU) continue;   /* no analog function. */
        if (analog) ansel |= EPIC_BIT(bit);
        else        ansel &= (uint8_t)~EPIC_BIT(bit);
        changed = 1U;
    }
    if (changed)
    {
#if PIC16F63X_67X_68X_FAMILY_HAS_ANSEL_BANK1
#ifdef EPIC_BANK1_WRITE8
        /* Real SFR name for the stringified asm token (see above). */
        EPIC_BANK1_WRITE8(ANSEL, ansel);
#else
        EPIC_REG8(PIC_REG_ANSEL_BANK1) = ansel;
#endif
#else
#ifdef EPIC_BANK2_WRITE8
        EPIC_BANK2_WRITE8(ANSEL, ansel);
#else
        EPIC_REG8(PIC_REG_ANSEL) = ansel;
#endif
#endif
    }
#endif
#if PIC16F63X_67X_68X_FAMILY_HAS_ANSELH
    uint8_t anselh = 0u;
#ifdef EPIC_BANK2_READ8
    EPIC_BANK2_READ8(ANSELH, anselh);
#else
    anselh = EPIC_REG8(PIC_REG_ANSELH);
#endif
    uint8_t changed_h = 0U;
    for (uint8_t pin = 0U; pin <= 7U; pin++)
    {
        if (!((uint16_t)EPIC_BIT(pin) & pins)) continue;
        uint8_t bit = anselh_bit(port, pin);
        if (bit == 0xFFU) continue;   /* no analog function. */
        if (analog) anselh |= EPIC_BIT(bit);
        else        anselh &= (uint8_t)~EPIC_BIT(bit);
        changed_h = 1U;
    }
    if (changed_h)
    {
#ifdef EPIC_BANK2_WRITE8
        EPIC_BANK2_WRITE8(ANSELH, anselh);
#else
        EPIC_REG8(PIC_REG_ANSELH) = anselh;
#endif
    }
#endif
}

/* init / deinit. */

/**
 * @brief Read the TRIS register of a port through the safe Bank-1 path.
 * @param port GPIOA..GPIOC.
 * @return the TRIS byte.
 */
static uint8_t tris_read(GPIO_TypeDef port)
{
#ifdef EPIC_BANK1_READ8
    /* Plain EPIC_REG8 RMW on Bank-1 TRISx (0x85..0x87) misdirects to
     * the Bank-0 alias under XC8 v4.00 (see the target platform
     * header). The banked macro needs a literal SFR name, so dispatch
     * per-port before any SFR access. */
    uint8_t v = 0u;
    switch (port)
    {
        case GPIOA: EPIC_BANK1_READ8(TRISA, v); break;
#if PIC16F63X_67X_68X_FAMILY_HAS_PORTB
        case GPIOB: EPIC_BANK1_READ8(TRISB, v); break;
#endif
        case GPIOC: EPIC_BANK1_READ8(TRISC, v); break;
        default: break;
    }
    return v;
#else
    return EPIC_REG8(tris_addr(port));
#endif
}

/**
 * @brief Write the TRIS register of a port through the safe Bank-1 path.
 * @param port GPIOA..GPIOC.
 * @param value the TRIS byte to write.
 */
static void tris_write(GPIO_TypeDef port, uint8_t value)
{
#ifdef EPIC_BANK1_WRITE8
    switch (port)
    {
        case GPIOA: EPIC_BANK1_WRITE8(TRISA, value); break;
#if PIC16F63X_67X_68X_FAMILY_HAS_PORTB
        case GPIOB: EPIC_BANK1_WRITE8(TRISB, value); break;
#endif
        case GPIOC: EPIC_BANK1_WRITE8(TRISC, value); break;
        default: break;
    }
#else
    EPIC_REG8(tris_addr(port)) = value;
#endif
}

/**
 * @brief  Configure one or more pins of a port as input, output or analog.
 * @param port GPIOA..GPIOC.
 * @param pins Bitmask of pins.
 * @param mode GPIO_MODE_INPUT / OUTPUT / ANALOG.
 *
 * @note   TRIS is Bank 1; a plain C RMW misdirects to the Bank-0 alias
 *         under XC8 v4.00 (see the target platform header). ANSEL lives
 *         in Bank 2 on this family.
 *
 * @details
 *   ANSEL resets to all-analog on the implemented bits (0xF3,
 *   DS40001262F memory map), so digital pins read '0' until their
 *   ANSEL bit is cleared. This driver clears the bit for
 *   INPUT/OUTPUT modes and sets it for ANALOG.
 */
void EPIC_GPIO_Init(GPIO_TypeDef port, uint16_t pins, GPIO_ModeTypeDef mode)
{
    uint8_t mask = (uint8_t)(pins & port_mask(port));

    /* Manage the ANSEL bits for every mode. */
    set_ansel_bits(port, pins, (mode == GPIO_MODE_ANALOG) ? 1U : 0U);

    /* Set/clear the TRIS bits (Bank 1). INPUT and ANALOG both keep
     * TRIS = 1 (high-impedance). */
    uint8_t tris = tris_read(port);
    if (mode == GPIO_MODE_OUTPUT)
    {
        tris &= (uint8_t)~mask;
    }
    else
    {
        tris |= mask;
    }
    tris_write(port, tris);
}

/**
 * @brief Restore all pins of `port` to input mode.
 * @param port GPIOA..GPIOC.
 */
void EPIC_GPIO_DeInit(GPIO_TypeDef port)
{
    uint8_t mask = port_mask(port);
    uint8_t tris = tris_read(port);
    tris |= mask;
    tris_write(port, tris);
    EPIC_REG8(port_addr(port)) = 0x00U;
}

/* read / write / toggle. */

/**
 * @brief Drive a set of pins high or low.
 * @param port GPIOA..GPIOC.
 * @param pins Bitmask of pins.
 * @param state GPIO_PIN_SET or GPIO_PIN_RESET.
 */
void EPIC_GPIO_WritePin(GPIO_TypeDef port, uint16_t pins, GPIO_PinState state)
{
    uint8_t p_addr = port_addr(port);
    uint8_t mask = (uint8_t)(pins & port_mask(port));
    uint8_t portval = EPIC_REG8(p_addr);
    if (state == GPIO_PIN_SET)
    {
        portval |= mask;
    }
    else
    {
        portval &= (uint8_t)~mask;
    }
    EPIC_REG8(p_addr) = portval;
}

/**
 * @brief Invert a set of pins.
 * @param port GPIOA..GPIOC.
 * @param pins Bitmask of pins to toggle.
 */
void EPIC_GPIO_TogglePin(GPIO_TypeDef port, uint16_t pins)
{
    uint8_t p_addr = port_addr(port);
    uint8_t mask = (uint8_t)(pins & port_mask(port));
    uint8_t portval = EPIC_REG8(p_addr);
    portval ^= mask;
    EPIC_REG8(p_addr) = portval;
}

/**
 * @brief Read the current level of a set of pins.
 * @param port GPIOA..GPIOC.
 * @param pins Bitmask of pins to sample.
 * @return GPIO_PIN_SET if any selected pin reads high, GPIO_PIN_RESET otherwise.
 */
GPIO_PinState EPIC_GPIO_ReadPin(GPIO_TypeDef port, uint16_t pins)
{
    uint8_t p_addr = port_addr(port);
    uint8_t mask = (uint8_t)(pins & port_mask(port));
    return (EPIC_REG8(p_addr) & mask) ? GPIO_PIN_SET : GPIO_PIN_RESET;
}

/**
 * @brief Write the whole port latch.
 * @param port GPIOA..GPIOC.
 * @param value the byte to write (unimplemented bits are masked off).
 */
void EPIC_GPIO_WritePort(GPIO_TypeDef port, uint8_t value)
{
    uint8_t p_addr = port_addr(port);
    EPIC_REG8(p_addr) = (uint8_t)(value & port_mask(port));
}

/**
 * @brief Read the whole port latch.
 * @param port GPIOA..GPIOC.
 * @return the port byte.
 */
uint8_t EPIC_GPIO_ReadPort(GPIO_TypeDef port)
{
    return EPIC_REG8(port_addr(port));
}

/* PORTA/PORTB pull-ups. */

/**
 * @brief Enable or disable the internal weak pull-ups (global gate).
 * @param pull GPIO_PULLUP (RABPU = 0) or GPIO_NOPULL (RABPU = 1).
 */
void EPIC_GPIO_SetPullups(GPIO_PullTypeDef pull)
{
    uint8_t option = 0u;
#ifdef EPIC_BANK1_READ8
    EPIC_BANK1_READ8(OPTION_REG, option);
#else
    option = EPIC_REG8(PIC_REG_OPTION);
#endif
    if (pull == GPIO_PULLUP) option &= (uint8_t)~PIC_OPTION_RABPU;
    else                     option |= PIC_OPTION_RABPU;
#ifdef EPIC_BANK1_WRITE8
    EPIC_BANK1_WRITE8(OPTION_REG, option);
#else
    EPIC_REG8(PIC_REG_OPTION) = option;
#endif
}

#if PIC16F63X_67X_68X_FAMILY_HAS_PORTB
/**
 * @brief Enable or disable the weak pull-up on a single PORTB pin.
 * @param pin the RB pin number, 4..7 (RB0..RB3 do not exist; masked).
 * @param enable 1 to enable the pull-up, 0 to disable it.
 */
void EPIC_GPIO_SetPinPullup(uint8_t pin, uint8_t enable)
{
    uint8_t wpub = 0u;
#ifdef EPIC_BANK2_READ8
    EPIC_BANK2_READ8(WPUB, wpub);
#else
    wpub = EPIC_REG8(PIC_REG_WPUB);
#endif
    uint8_t mask = (uint8_t)((1U << (pin & 0x07U)) & 0xF0U);
    if (enable) wpub |= mask;
    else        wpub &= (uint8_t)~mask;
#ifdef EPIC_BANK2_WRITE8
    EPIC_BANK2_WRITE8(WPUB, wpub);
#else
    EPIC_REG8(PIC_REG_WPUB) = wpub;
#endif
}
#endif

/* PORTA/B change interrupt. */

/* One callback slot for the whole-port RB<7:4> change interrupt. There is
 * only one PORTB, so there is no handle struct. NULL = unregistered. */
static void (*s_rb_change_callback)(uint8_t) = NULL;

/**
 * @brief Install or remove the PORTB change callback.
 * @param callback function called with the PORTB byte on an RB<7:4>
 *        change, or NULL to unregister.
 */
void EPIC_GPIO_RegisterChangeCallback(void (*callback)(uint8_t))
{
    s_rb_change_callback = callback;
}

#if PIC16F63X_67X_68X_FAMILY_HAS_PORTB
/**
 * @brief Enable or disable interrupt-on-change for one PORTB pin.
 * @param pin the RB pin number, 4..7 (RB0..RB3 do not exist; masked).
 * @param enable 1 to enable IOC on the pin, 0 to disable it.
 */
void EPIC_GPIO_SetPinIOC(uint8_t pin, uint8_t enable)
{
    uint8_t iocb = 0u;
#ifdef EPIC_BANK2_READ8
    EPIC_BANK2_READ8(IOCB, iocb);
#else
    iocb = EPIC_REG8(PIC_REG_IOCB);
#endif
    uint8_t mask = (uint8_t)((1U << (pin & 0x07U)) & 0xF0U);
    if (enable) iocb |= mask;
    else        iocb &= (uint8_t)~mask;
#ifdef EPIC_BANK2_WRITE8
    EPIC_BANK2_WRITE8(IOCB, iocb);
#else
    EPIC_REG8(PIC_REG_IOCB) = iocb;
#endif
}
#endif

/**
 * @brief Weak RB<7:4> change ISR: reads PORTB first, clears RABIF, then
 *        fires the registered callback with the byte.
 */
void RB_IRQHandler(void)
{
    /* Direct flag ops (class-F). RABIF is INTCON bit 0. */
    if (!(EPIC_REG8(PIC_REG_INTCON) & PIC_INTCON_RBIF)) return;
#if PIC16F63X_67X_68X_FAMILY_HAS_PORTB
    uint8_t portb = EPIC_REG8(PIC_REG_PORTB);
#else
    uint8_t portb = EPIC_REG8(PIC_REG_PORTA);
#endif
    EPIC_BIT_CLR(EPIC_REG8(PIC_REG_INTCON), PIC_INTCON_RBIF);
    if (s_rb_change_callback) s_rb_change_callback(portb);
}
