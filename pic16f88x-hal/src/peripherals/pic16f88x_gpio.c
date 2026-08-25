/* GPIO driver implementation (DS40001291H §3.1..§3.7). */

#include "peripherals/pic16f88x_gpio.h"
#include "core/pic16_irq.h"

/**
 * @brief  Map a GPIO_TypeDef to the address of its TRIS register.
 * @param port GPIOA..GPIOE.
 * @return the TRIS SFR address (0x85..0x89).
 */
static uint8_t tris_addr(GPIO_TypeDef port)
{
    switch (port) {
        case GPIOA: return PIC_REG_TRISA;
        case GPIOB: return PIC_REG_TRISB;
        case GPIOC: return PIC_REG_TRISC;
#if PIC16F88X_FAMILY_HAS_PORTD
        case GPIOD: return PIC_REG_TRISD;
#endif
#if PIC16F88X_FAMILY_HAS_PORTE
        case GPIOE: return PIC_REG_TRISE;
#endif
        default:    return PIC_REG_TRISA;
    }
}

/**
 * @brief Map a GPIO_TypeDef to the PORTx register address.
 * @param port GPIOA..GPIOE.
 * @return the PORTx SFR address.
 */
static uint8_t port_addr(GPIO_TypeDef port)
{
    switch (port) {
        case GPIOA: return PIC_REG_PORTA;
        case GPIOB: return PIC_REG_PORTB;
        case GPIOC: return PIC_REG_PORTC;
#if PIC16F88X_FAMILY_HAS_PORTD
        case GPIOD: return PIC_REG_PORTD;
#endif
#if PIC16F88X_FAMILY_HAS_PORTE
        case GPIOE: return PIC_REG_PORTE;
#endif
        default:    return PIC_REG_PORTA;
    }
}

/**
 * @brief  Upper pin bound for a port. PORTA..PORTD = 7, PORTE = 2.
 * @param port GPIOA..GPIOE.
 * @return the highest valid pin number.
 */
static uint8_t port_width(GPIO_TypeDef port)
{
    switch (port) {
#if PIC16F88X_FAMILY_HAS_PORTE
        case GPIOE: return 2U;    /* RE0..RE2. */
#endif
        default:    return 7U;
    }
}

/**
 * @brief Mask off unimplemented bits of a port's TRIS/PORT byte.
 * @param port GPIOA..GPIOE.
 * @return the valid-pin mask.
 */
static uint8_t port_mask(GPIO_TypeDef port)
{
    return (uint8_t)(0xFFU >> (7U - port_width(port)));
}

/**
 * @brief Map a pin to its ANSEL register + bit, if it has an analog
 *        function. ANSEL/ANSELH live in Bank 3 (DS40001291H §3.0,
 *        Registers 3-3/3-4); the mapping is NOT linear in pin number:
 *        RA4/T0CKI has no analog function, and RB0..RB5 map to ANSELH
 *        bits 4,2,0,1,3,5 (AN12,AN10,AN8,AN9,AN11,AN13).
 *
 * @param port GPIOA..GPIOE.
 * @param pin the pin number, 0..7.
 * @param reg_out set to the ANSEL/ANSELH SFR address if the pin is
 *        analog-capable.
 * @return the bit position in that register, or 0xFF if the pin has no
 *         analog function.
 */
static uint8_t ansel_bit(GPIO_TypeDef port, uint8_t pin, uint16_t *reg_out)
{
    switch (port) {
        case GPIOA:
            /* RA0=AN0, RA1=AN1, RA2=AN2, RA3=AN3, RA5=AN4. */
            if (pin <= 3U)      { *reg_out = PIC_REG_ANSEL;  return pin; }
            if (pin == 5U)      { *reg_out = PIC_REG_ANSEL;  return 4U;  }
            return 0xFFU;
        case GPIOB:
            /* RB0=AN12, RB1=AN10, RB2=AN8, RB3=AN9, RB4=AN11,
             * RB5=AN13. */
            switch (pin) {
                case 0U: *reg_out = PIC_REG_ANSELH; return 4U;
                case 1U: *reg_out = PIC_REG_ANSELH; return 2U;
                case 2U: *reg_out = PIC_REG_ANSELH; return 0U;
                case 3U: *reg_out = PIC_REG_ANSELH; return 1U;
                case 4U: *reg_out = PIC_REG_ANSELH; return 3U;
                case 5U: *reg_out = PIC_REG_ANSELH; return 5U;
                default: return 0xFFU;
            }
#if PIC16F88X_FAMILY_HAS_PORTE
        case GPIOE:
            /* RE0=AN5, RE1=AN6, RE2=AN7. */
            if (pin <= 2U)      { *reg_out = PIC_REG_ANSEL;  return (uint8_t)(pin + 5U); }
            return 0xFFU;
#endif
        default:
            /* PORTC/PORTD have no analog functions. */
            return 0xFFU;
    }
}

/**
 * @brief  Clear or set the ANSEL/ANSELH bits for a set of pins (Bank 3).
 * @param port GPIOA..GPIOE.
 * @param pins bitmask of pins.
 * @param analog 1 to set the bits (pin to analog), 0 to clear them
 *        (pin to digital).
 */
static void set_ansel_bits(GPIO_TypeDef port, uint16_t pins, uint8_t analog)
{
    uint8_t ansel = 0u, anselh = 0u;
#ifdef EPIC_BANK3_READ8
    EPIC_BANK3_READ8(ANSEL, ansel);
    EPIC_BANK3_READ8(ANSELH, anselh);
#else
    ansel  = EPIC_REG8(PIC_REG_ANSEL);
    anselh = EPIC_REG8(PIC_REG_ANSELH);
#endif
    uint8_t changed_a = 0U, changed_h = 0U;
    for (uint8_t pin = 0U; pin <= port_width(port); pin++) {
        if (!((uint16_t)EPIC_BIT(pin) & pins)) continue;
        uint16_t reg = 0U;
        uint8_t bit = ansel_bit(port, pin, &reg);
        if (bit == 0xFFU) continue;   /* no analog function. */
        if (reg == PIC_REG_ANSEL) {
            if (analog) ansel |= EPIC_BIT(bit);
            else        ansel &= (uint8_t)~EPIC_BIT(bit);
            changed_a = 1U;
        } else {
            if (analog) anselh |= EPIC_BIT(bit);
            else        anselh &= (uint8_t)~EPIC_BIT(bit);
            changed_h = 1U;
        }
    }
    if (changed_a) {
#ifdef EPIC_BANK3_WRITE8
        EPIC_BANK3_WRITE8(ANSEL, ansel);
#else
        EPIC_REG8(PIC_REG_ANSEL) = ansel;
#endif
    }
    if (changed_h) {
#ifdef EPIC_BANK3_WRITE8
        EPIC_BANK3_WRITE8(ANSELH, anselh);
#else
        EPIC_REG8(PIC_REG_ANSELH) = anselh;
#endif
    }
}

/* init / deinit. */

/**
 * @brief Read the TRIS register of a port through the safe Bank-1 path.
 * @param port GPIOA..GPIOE.
 * @return the TRIS byte.
 */
static uint8_t tris_read(GPIO_TypeDef port)
{
#ifdef EPIC_BANK1_READ8
    /* Plain EPIC_REG8 RMW on Bank-1 TRISx (0x85..0x89) misdirects to
     * the Bank-0 alias under XC8 v4.00 (see target/pic16f88x_platform.h).
     * The banked macro needs a literal SFR name, so dispatch per-port
     * before any SFR access. */
    uint8_t v = 0u;
    switch (port) {
        case GPIOA: EPIC_BANK1_READ8(TRISA, v); break;
        case GPIOB: EPIC_BANK1_READ8(TRISB, v); break;
        case GPIOC: EPIC_BANK1_READ8(TRISC, v); break;
#if PIC16F88X_FAMILY_HAS_PORTD
        case GPIOD: EPIC_BANK1_READ8(TRISD, v); break;
#endif
#if PIC16F88X_FAMILY_HAS_PORTE
        case GPIOE: EPIC_BANK1_READ8(TRISE, v); break;
#endif
        default: break;
    }
    return v;
#else
    return EPIC_REG8(tris_addr(port));
#endif
}

/**
 * @brief Write the TRIS register of a port through the safe Bank-1 path.
 * @param port GPIOA..GPIOE.
 * @param value the TRIS byte to write.
 */
static void tris_write(GPIO_TypeDef port, uint8_t value)
{
#ifdef EPIC_BANK1_WRITE8
    switch (port) {
        case GPIOA: EPIC_BANK1_WRITE8(TRISA, value); break;
        case GPIOB: EPIC_BANK1_WRITE8(TRISB, value); break;
        case GPIOC: EPIC_BANK1_WRITE8(TRISC, value); break;
#if PIC16F88X_FAMILY_HAS_PORTD
        case GPIOD: EPIC_BANK1_WRITE8(TRISD, value); break;
#endif
#if PIC16F88X_FAMILY_HAS_PORTE
        case GPIOE: EPIC_BANK1_WRITE8(TRISE, value); break;
#endif
        default: break;
    }
#else
    EPIC_REG8(tris_addr(port)) = value;
#endif
}

/**
 * @brief  Configure one or more pins of a port as input, output or analog.
 * @param port GPIOA..GPIOE.
 * @param pins Bitmask of pins.
 * @param mode GPIO_MODE_INPUT / OUTPUT / ANALOG.
 *
 * @note   TRIS is Bank 1; a plain C RMW misdirects to the Bank-0 alias
 *         under XC8 v4.00 (see target/pic16f88x_platform.h). ANSEL/
 *         ANSELH live in Bank 3.
 *
 * @details
 *   On the 88X, ANSEL/ANSELH reset to all-analog (0xFF/0x3F,
 *   DS40001291H Table 2-4), so digital pins read '0' until their ANSEL
 *   bit is cleared. This driver clears the bit for INPUT/OUTPUT modes
 *   and sets it for ANALOG. The ADC driver owns the channel->ANSEL
 *   mapping separately via EPIC_ADC_ConfigChannel(); GPIO_MODE_ANALOG
 *   here only releases the pin (TRIS=1 + ANSEL=1) without touching
 *   ADCON0.
 */
void EPIC_GPIO_Init(GPIO_TypeDef port, uint16_t pins, GPIO_ModeTypeDef mode)
{
    uint8_t mask = (uint8_t)(pins & port_mask(port));

    /* Manage the ANSEL/ANSELH bits for every mode. */
    set_ansel_bits(port, pins, (mode == GPIO_MODE_ANALOG) ? 1U : 0U);

    /* Set/clear the TRIS bits (Bank 1). INPUT and ANALOG both keep
     * TRIS = 1 (high-impedance). */
    uint8_t tris = tris_read(port);
    if (mode == GPIO_MODE_OUTPUT) {
        tris &= (uint8_t)~mask;
    } else {
        tris |= mask;
    }
    tris_write(port, tris);
}

/**
 * @brief Restore all pins of a port to input mode.
 * @param port GPIOA..GPIOE.
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
 * @param port GPIOA..GPIOE.
 * @param pins Bitmask of pins.
 * @param state GPIO_PIN_SET or GPIO_PIN_RESET.
 */
void EPIC_GPIO_WritePin(GPIO_TypeDef port, uint16_t pins, GPIO_PinState state)
{
    uint8_t p_addr = port_addr(port);
    uint8_t mask = (uint8_t)(pins & port_mask(port));
    uint8_t portval = EPIC_REG8(p_addr);
    if (state == GPIO_PIN_SET) {
        portval |= mask;
    } else {
        portval &= (uint8_t)~mask;
    }
    EPIC_REG8(p_addr) = portval;
}

/**
 * @brief Invert a set of pins.
 * @param port GPIOA..GPIOE.
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
 * @param port GPIOA..GPIOE.
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
 * @param port GPIOA..GPIOE.
 * @param value the byte to write (unimplemented bits are masked off).
 */
void EPIC_GPIO_WritePort(GPIO_TypeDef port, uint8_t value)
{
    uint8_t p_addr = port_addr(port);
    EPIC_REG8(p_addr) = (uint8_t)(value & port_mask(port));
}

/**
 * @brief Read the whole port latch.
 * @param port GPIOA..GPIOE.
 * @return the port byte.
 */
uint8_t EPIC_GPIO_ReadPort(GPIO_TypeDef port)
{
    return EPIC_REG8(port_addr(port));
}

/* PORTB pull-ups. */

/**
 * @brief Enable or disable the PORTB internal weak pull-ups (global gate).
 * @param pull GPIO_PULLUP (RBPU = 0) or GPIO_NOPULL (RBPU = 1).
 */
void EPIC_GPIO_SetPullups(GPIO_PullTypeDef pull)
{
    uint8_t option = 0u;
#ifdef EPIC_BANK1_READ8
    EPIC_BANK1_READ8(OPTION_REG, option);
#else
    option = EPIC_REG8(PIC_REG_OPTION);
#endif
    if (pull == GPIO_PULLUP) option &= (uint8_t)~PIC_OPTION_RBPU;
    else                     option |= PIC_OPTION_RBPU;
#ifdef EPIC_BANK1_WRITE8
    EPIC_BANK1_WRITE8(OPTION_REG, option);
#else
    EPIC_REG8(PIC_REG_OPTION) = option;
#endif
}

/**
 * @brief Enable or disable the weak pull-up on a single PORTB pin.
 * @param pin the RB pin number, 0..7.
 * @param enable 1 to enable the pull-up, 0 to disable it.
 */
void EPIC_GPIO_SetPinPullup(uint8_t pin, uint8_t enable)
{
    uint8_t wpub = 0u;
#ifdef EPIC_BANK1_READ8
    EPIC_BANK1_READ8(WPUB, wpub);
#else
    wpub = EPIC_REG8(PIC_REG_WPUB);
#endif
    uint8_t mask = (uint8_t)(1U << (pin & 0x07U));
    if (enable) wpub |= mask;
    else        wpub &= (uint8_t)~mask;
#ifdef EPIC_BANK1_WRITE8
    EPIC_BANK1_WRITE8(WPUB, wpub);
#else
    EPIC_REG8(PIC_REG_WPUB) = wpub;
#endif
}

/* PORTB change interrupt. */

/* One callback slot for the whole-port RB<7:0> change interrupt. There is
 * only one PORTB, so there is no handle struct (mirrors Timer2's
 * one-callback-per-handle shape but simpler). NULL = unregistered/no-op. */
static void (*s_rb_change_callback)(uint8_t) = NULL;

/**
 * @brief Install or remove the PORTB change callback.
 * @param callback function called with the PORTB byte on an RB<7:0>
 *        change, or NULL to unregister.
 */
void EPIC_GPIO_RegisterChangeCallback(void (*callback)(uint8_t))
{
    s_rb_change_callback = callback;
}

/**
 * @brief Enable or disable interrupt-on-change for one PORTB pin.
 * @param pin the RB pin number, 0..7.
 * @param enable 1 to enable IOC on the pin, 0 to disable it.
 */
void EPIC_GPIO_SetPinIOC(uint8_t pin, uint8_t enable)
{
    uint8_t iocb = 0u;
#ifdef EPIC_BANK1_READ8
    EPIC_BANK1_READ8(IOCB, iocb);
#else
    iocb = EPIC_REG8(PIC_REG_IOCB);
#endif
    uint8_t mask = (uint8_t)(1U << (pin & 0x07U));
    if (enable) iocb |= mask;
    else        iocb &= (uint8_t)~mask;
#ifdef EPIC_BANK1_WRITE8
    EPIC_BANK1_WRITE8(IOCB, iocb);
#else
    EPIC_REG8(PIC_REG_IOCB) = iocb;
#endif
}

/**
 * @brief Weak RB<7:0> change ISR: reads PORTB first, clears RBIF, then
 *        fires the registered callback with the byte.
 */
void RB_IRQHandler(void)
{
    /* Direct flag ops (class-F). */
    if (!(EPIC_REG8(PIC_REG_INTCON) & PIC_INTCON_RBIF)) return;
    uint8_t portb = EPIC_REG8(PIC_REG_PORTB);
    EPIC_BIT_CLR(EPIC_REG8(PIC_REG_INTCON), PIC_INTCON_RBIF);
#ifndef EPIC_AT
    if (s_rb_change_callback) s_rb_change_callback(portb);
#else
    (void)portb;
#endif
}
