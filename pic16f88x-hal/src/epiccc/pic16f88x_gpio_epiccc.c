/* epic-cc GPIO driver (sibling to src/peripherals/pic16f88x_gpio.c);
 * avoids runtime-computed SFR addresses (`inttoptr i16 %reg`) which irparse
 * does not yet support for variable pointers, and avoids the LLVM switch
 * instruction which irparse also does not yet lower. Each port case uses a
 * literal SFR address via if-else so clang emits `inttoptr (i16 <const> to
 * ptr)` and plain branches. */

#include "peripherals/pic16f88x_gpio.h"
#include "core/pic16_irq.h"

/**
 * @brief Port width in pins.
 * @param port GPIO port.
 * @return number of implemented pins.
 */
static uint8_t port_width(GPIO_TypeDef port)
{
#if PIC16F88X_FAMILY_HAS_PORTE
    if (port == GPIOE) return 2U;
#endif
    (void)port;
    return 7U;
}

/**
 * @brief Port mask of implemented pins.
 * @param port GPIO port.
 * @return bitmask of valid pins.
 */
static uint8_t port_mask(GPIO_TypeDef port)
{
#if PIC16F88X_FAMILY_HAS_PORTE
    if (port == GPIOE) return 0x07U;
#endif
    (void)port;
    return 0xFFU;
}

/**
 * @brief Map a pin to its ANSEL bit.
 * @param port GPIO port.
 * @param pin pin number.
 * @param reg_out ANSEL register address.
 * @return bit position or 0xFF if no analog function.
 */
static uint8_t ansel_bit(GPIO_TypeDef port, uint8_t pin, uint16_t *reg_out)
{
    if (port == GPIOA) {
        if (pin <= 3U) { *reg_out = PIC_REG_ANSEL; return pin; }
        if (pin == 5U) { *reg_out = PIC_REG_ANSEL; return 4U; }
        return 0xFFU;
    }
    if (port == GPIOB) {
        if (pin == 0U) { *reg_out = PIC_REG_ANSELH; return 4U; }
        if (pin == 1U) { *reg_out = PIC_REG_ANSELH; return 2U; }
        if (pin == 2U) { *reg_out = PIC_REG_ANSELH; return 0U; }
        if (pin == 3U) { *reg_out = PIC_REG_ANSELH; return 1U; }
        if (pin == 4U) { *reg_out = PIC_REG_ANSELH; return 3U; }
        if (pin == 5U) { *reg_out = PIC_REG_ANSELH; return 5U; }
        return 0xFFU;
    }
#if PIC16F88X_FAMILY_HAS_PORTE
    if (port == GPIOE) {
        if (pin <= 2U) { *reg_out = PIC_REG_ANSEL; return (uint8_t)(pin + 5U); }
        return 0xFFU;
    }
#endif
    return 0xFFU;
}

/**
 * @brief Configure ANSEL bits for a set of pins.
 * @param port GPIO port.
 * @param pins bitmask.
 * @param analog 1 for analog, 0 for digital.
 */
static void set_ansel_bits(GPIO_TypeDef port, uint16_t pins, uint8_t analog)
{
    uint8_t ansel = EPIC_REG8(PIC_REG_ANSEL);
    uint8_t anselh = EPIC_REG8(PIC_REG_ANSELH);
    uint8_t changed_a = 0U, changed_h = 0U;
    for (uint8_t pin = 0U; pin <= port_width(port); pin++) {
        if (!((uint16_t)EPIC_BIT(pin) & pins)) continue;
        uint16_t reg = 0U;
        uint8_t bit = ansel_bit(port, pin, &reg);
        if (bit == 0xFFU) continue;
        if (reg == PIC_REG_ANSEL) {
            if (analog) ansel |= EPIC_BIT(bit);
            else ansel &= (uint8_t)~EPIC_BIT(bit);
            changed_a = 1U;
        } else {
            if (analog) anselh |= EPIC_BIT(bit);
            else anselh &= (uint8_t)~EPIC_BIT(bit);
            changed_h = 1U;
        }
    }
    if (changed_a) EPIC_REG8(PIC_REG_ANSEL) = ansel;
    if (changed_h) EPIC_REG8(PIC_REG_ANSELH) = anselh;
}

/**
 * @brief Configure one or more pins of a port.
 * @param port GPIO port.
 * @param pins bitmask.
 * @param mode direction.
 */
void EPIC_GPIO_Init(GPIO_TypeDef port, uint16_t pins, GPIO_ModeTypeDef mode)
{
    uint8_t mask = (uint8_t)(pins & port_mask(port));
    set_ansel_bits(port, pins, (mode == GPIO_MODE_ANALOG) ? 1U : 0U);
    if (port == GPIOA) {
        uint8_t tris = EPIC_REG8(PIC_REG_TRISA);
        if (mode == GPIO_MODE_OUTPUT) tris &= (uint8_t)~mask;
        else tris |= mask;
        EPIC_REG8(PIC_REG_TRISA) = tris;
    } else if (port == GPIOB) {
        uint8_t tris = EPIC_REG8(PIC_REG_TRISB);
        if (mode == GPIO_MODE_OUTPUT) tris &= (uint8_t)~mask;
        else tris |= mask;
        EPIC_REG8(PIC_REG_TRISB) = tris;
    } else if (port == GPIOC) {
        uint8_t tris = EPIC_REG8(PIC_REG_TRISC);
        if (mode == GPIO_MODE_OUTPUT) tris &= (uint8_t)~mask;
        else tris |= mask;
        EPIC_REG8(PIC_REG_TRISC) = tris;
    }
#if PIC16F88X_FAMILY_HAS_PORTD
    else if (port == GPIOD) {
        uint8_t tris = EPIC_REG8(PIC_REG_TRISD);
        if (mode == GPIO_MODE_OUTPUT) tris &= (uint8_t)~mask;
        else tris |= mask;
        EPIC_REG8(PIC_REG_TRISD) = tris;
    }
#endif
#if PIC16F88X_FAMILY_HAS_PORTE
    else if (port == GPIOE) {
        uint8_t tris = EPIC_REG8(PIC_REG_TRISE);
        if (mode == GPIO_MODE_OUTPUT) tris &= (uint8_t)~mask;
        else tris |= mask;
        EPIC_REG8(PIC_REG_TRISE) = tris;
    }
#endif
    else {
        uint8_t tris = EPIC_REG8(PIC_REG_TRISA);
        if (mode == GPIO_MODE_OUTPUT) tris &= (uint8_t)~mask;
        else tris |= mask;
        EPIC_REG8(PIC_REG_TRISA) = tris;
    }
}

/**
 * @brief Restore all pins of a port to input.
 * @param port GPIO port.
 */
void EPIC_GPIO_DeInit(GPIO_TypeDef port)
{
    uint8_t mask = port_mask(port);
    if (port == GPIOA) {
        uint8_t tris = EPIC_REG8(PIC_REG_TRISA);
        tris |= mask;
        EPIC_REG8(PIC_REG_TRISA) = tris;
        EPIC_REG8(PIC_REG_PORTA) = 0x00U;
    } else if (port == GPIOB) {
        uint8_t tris = EPIC_REG8(PIC_REG_TRISB);
        tris |= mask;
        EPIC_REG8(PIC_REG_TRISB) = tris;
        EPIC_REG8(PIC_REG_PORTB) = 0x00U;
    } else if (port == GPIOC) {
        uint8_t tris = EPIC_REG8(PIC_REG_TRISC);
        tris |= mask;
        EPIC_REG8(PIC_REG_TRISC) = tris;
        EPIC_REG8(PIC_REG_PORTC) = 0x00U;
    }
#if PIC16F88X_FAMILY_HAS_PORTD
    else if (port == GPIOD) {
        uint8_t tris = EPIC_REG8(PIC_REG_TRISD);
        tris |= mask;
        EPIC_REG8(PIC_REG_TRISD) = tris;
        EPIC_REG8(PIC_REG_PORTD) = 0x00U;
    }
#endif
#if PIC16F88X_FAMILY_HAS_PORTE
    else if (port == GPIOE) {
        uint8_t tris = EPIC_REG8(PIC_REG_TRISE);
        tris |= mask;
        EPIC_REG8(PIC_REG_TRISE) = tris;
        EPIC_REG8(PIC_REG_PORTE) = 0x00U;
    }
#endif
    else {
        uint8_t tris = EPIC_REG8(PIC_REG_TRISA);
        tris |= mask;
        EPIC_REG8(PIC_REG_TRISA) = tris;
        EPIC_REG8(PIC_REG_PORTA) = 0x00U;
    }
}

/**
 * @brief Drive a set of pins high or low.
 * @param port GPIO port.
 * @param pins bitmask.
 * @param state pin state.
 */
void EPIC_GPIO_WritePin(GPIO_TypeDef port, uint16_t pins, GPIO_PinState state)
{
    uint8_t mask = (uint8_t)(pins & port_mask(port));
    if (port == GPIOA) {
        uint8_t cur = EPIC_REG8(PIC_REG_PORTA);
        __asm__ volatile("" ::: "memory");
        if (state == GPIO_PIN_SET) cur |= mask;
        else cur &= (uint8_t)~mask;
        EPIC_REG8(PIC_REG_PORTA) = cur;
        return;
    }
    if (port == GPIOB) {
        uint8_t cur = EPIC_REG8(PIC_REG_PORTB);
        __asm__ volatile("" ::: "memory");
        if (state == GPIO_PIN_SET) cur |= mask;
        else cur &= (uint8_t)~mask;
        EPIC_REG8(PIC_REG_PORTB) = cur;
        return;
    }
    if (port == GPIOC) {
        uint8_t cur = EPIC_REG8(PIC_REG_PORTC);
        __asm__ volatile("" ::: "memory");
        if (state == GPIO_PIN_SET) cur |= mask;
        else cur &= (uint8_t)~mask;
        EPIC_REG8(PIC_REG_PORTC) = cur;
        return;
    }
#if PIC16F88X_FAMILY_HAS_PORTD
    if (port == GPIOD) {
        uint8_t cur = EPIC_REG8(PIC_REG_PORTD);
        __asm__ volatile("" ::: "memory");
        if (state == GPIO_PIN_SET) cur |= mask;
        else cur &= (uint8_t)~mask;
        EPIC_REG8(PIC_REG_PORTD) = cur;
        return;
    }
#endif
#if PIC16F88X_FAMILY_HAS_PORTE
    if (port == GPIOE) {
        uint8_t cur = EPIC_REG8(PIC_REG_PORTE);
        __asm__ volatile("" ::: "memory");
        if (state == GPIO_PIN_SET) cur |= mask;
        else cur &= (uint8_t)~mask;
        EPIC_REG8(PIC_REG_PORTE) = cur;
        return;
    }
#endif
    {
        uint8_t cur = EPIC_REG8(PIC_REG_PORTA);
        __asm__ volatile("" ::: "memory");
        if (state == GPIO_PIN_SET) cur |= mask;
        else cur &= (uint8_t)~mask;
        EPIC_REG8(PIC_REG_PORTA) = cur;
    }
}

/**
 * @brief Toggle a set of pins.
 * @param port GPIO port.
 * @param pins bitmask.
 */
void EPIC_GPIO_TogglePin(GPIO_TypeDef port, uint16_t pins)
{
    uint8_t mask = (uint8_t)(pins & port_mask(port));
    if (port == GPIOA) EPIC_REG8(PIC_REG_PORTA) ^= mask;
    else if (port == GPIOB) EPIC_REG8(PIC_REG_PORTB) ^= mask;
    else if (port == GPIOC) EPIC_REG8(PIC_REG_PORTC) ^= mask;
#if PIC16F88X_FAMILY_HAS_PORTD
    else if (port == GPIOD) EPIC_REG8(PIC_REG_PORTD) ^= mask;
#endif
#if PIC16F88X_FAMILY_HAS_PORTE
    else if (port == GPIOE) EPIC_REG8(PIC_REG_PORTE) ^= mask;
#endif
    else EPIC_REG8(PIC_REG_PORTA) ^= mask;
}

/**
 * @brief Read the current level of a set of pins.
 * @param port GPIO port.
 * @param pins bitmask.
 * @return pin state.
 */
GPIO_PinState EPIC_GPIO_ReadPin(GPIO_TypeDef port, uint16_t pins)
{
    uint8_t mask = (uint8_t)(pins & port_mask(port));
    if (port == GPIOA) {
        uint8_t v = EPIC_REG8(PIC_REG_PORTA);
        __asm__ volatile("" ::: "memory");
        return (v & mask) ? GPIO_PIN_SET : GPIO_PIN_RESET;
    }
    if (port == GPIOB) {
        uint8_t v = EPIC_REG8(PIC_REG_PORTB);
        __asm__ volatile("" ::: "memory");
        return (v & mask) ? GPIO_PIN_SET : GPIO_PIN_RESET;
    }
    if (port == GPIOC) {
        uint8_t v = EPIC_REG8(PIC_REG_PORTC);
        __asm__ volatile("" ::: "memory");
        return (v & mask) ? GPIO_PIN_SET : GPIO_PIN_RESET;
    }
#if PIC16F88X_FAMILY_HAS_PORTD
    if (port == GPIOD) {
        uint8_t v = EPIC_REG8(PIC_REG_PORTD);
        __asm__ volatile("" ::: "memory");
        return (v & mask) ? GPIO_PIN_SET : GPIO_PIN_RESET;
    }
#endif
#if PIC16F88X_FAMILY_HAS_PORTE
    if (port == GPIOE) {
        uint8_t v = EPIC_REG8(PIC_REG_PORTE);
        __asm__ volatile("" ::: "memory");
        return (v & mask) ? GPIO_PIN_SET : GPIO_PIN_RESET;
    }
#endif
    {
        uint8_t v = EPIC_REG8(PIC_REG_PORTA);
        __asm__ volatile("" ::: "memory");
        return (v & mask) ? GPIO_PIN_SET : GPIO_PIN_RESET;
    }
}

/**
 * @brief Write the whole port latch.
 * @param port GPIO port.
 * @param value byte to write.
 */
void EPIC_GPIO_WritePort(GPIO_TypeDef port, uint8_t value)
{
    uint8_t v = (uint8_t)(value & port_mask(port));
    if (port == GPIOA) EPIC_REG8(PIC_REG_PORTA) = v;
    else if (port == GPIOB) EPIC_REG8(PIC_REG_PORTB) = v;
    else if (port == GPIOC) EPIC_REG8(PIC_REG_PORTC) = v;
#if PIC16F88X_FAMILY_HAS_PORTD
    else if (port == GPIOD) EPIC_REG8(PIC_REG_PORTD) = v;
#endif
#if PIC16F88X_FAMILY_HAS_PORTE
    else if (port == GPIOE) EPIC_REG8(PIC_REG_PORTE) = v;
#endif
    else EPIC_REG8(PIC_REG_PORTA) = v;
}

/**
 * @brief Read the whole port latch.
 * @param port GPIO port.
 * @return port byte.
 */
uint8_t EPIC_GPIO_ReadPort(GPIO_TypeDef port)
{
    if (port == GPIOA) { uint8_t v = EPIC_REG8(PIC_REG_PORTA); __asm__ volatile("" ::: "memory"); return v; }
    if (port == GPIOB) { uint8_t v = EPIC_REG8(PIC_REG_PORTB); __asm__ volatile("" ::: "memory"); return v; }
    if (port == GPIOC) { uint8_t v = EPIC_REG8(PIC_REG_PORTC); __asm__ volatile("" ::: "memory"); return v; }
#if PIC16F88X_FAMILY_HAS_PORTD
    if (port == GPIOD) { uint8_t v = EPIC_REG8(PIC_REG_PORTD); __asm__ volatile("" ::: "memory"); return v; }
#endif
#if PIC16F88X_FAMILY_HAS_PORTE
    if (port == GPIOE) { uint8_t v = EPIC_REG8(PIC_REG_PORTE); __asm__ volatile("" ::: "memory"); return v; }
#endif
    { uint8_t v = EPIC_REG8(PIC_REG_PORTA); __asm__ volatile("" ::: "memory"); return v; }
}

/**
 * @brief Enable or disable PORTB pull-ups.
 * @param pull pull-up control.
 */
void EPIC_GPIO_SetPullups(GPIO_PullTypeDef pull)
{
    uint8_t option = EPIC_REG8(PIC_REG_OPTION);
    if (pull == GPIO_PULLUP) option &= (uint8_t)~PIC_OPTION_RBPU;
    else option |= PIC_OPTION_RBPU;
    EPIC_REG8(PIC_REG_OPTION) = option;
}

/**
 * @brief Enable or disable pull-up on one pin.
 * @param pin pin number.
 * @param enable 1 to enable.
 */
void EPIC_GPIO_SetPinPullup(uint8_t pin, uint8_t enable)
{
    uint8_t wpub = EPIC_REG8(PIC_REG_WPUB);
    uint8_t mask = (uint8_t)(1U << (pin & 0x07U));
    if (enable) wpub |= mask;
    else wpub &= (uint8_t)~mask;
    EPIC_REG8(PIC_REG_WPUB) = wpub;
}

static void (*s_rb_change_callback)(uint8_t) = NULL;

/**
 * @brief Register PORTB change callback.
 * @param callback function called with PORTB byte.
 */
void EPIC_GPIO_RegisterChangeCallback(void (*callback)(uint8_t))
{
    s_rb_change_callback = callback;
}

/**
 * @brief Enable or disable interrupt-on-change for one pin.
 * @param pin pin number.
 * @param enable 1 to enable.
 */
void EPIC_GPIO_SetPinIOC(uint8_t pin, uint8_t enable)
{
    uint8_t iocb = EPIC_REG8(PIC_REG_IOCB);
    uint8_t mask = (uint8_t)(1U << (pin & 0x07U));
    if (enable) iocb |= mask;
    else iocb &= (uint8_t)~mask;
    EPIC_REG8(PIC_REG_IOCB) = iocb;
}

/**
 * @brief RB change ISR.
 */
void RB_IRQHandler(void)
{
    if (!(EPIC_REG8(PIC_REG_INTCON) & PIC_INTCON_RBIF)) return;
    uint8_t portb = EPIC_REG8(PIC_REG_PORTB);
    EPIC_BIT_CLR(EPIC_REG8(PIC_REG_INTCON), PIC_INTCON_RBIF);
#ifndef EPIC_AT
    if (s_rb_change_callback) s_rb_change_callback(portb);
#else
    (void)s_rb_change_callback;
    (void)portb;
#endif
}
