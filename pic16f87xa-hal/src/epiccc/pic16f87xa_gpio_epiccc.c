/* epic-cc GPIO driver (sibling to src/peripherals/pic16f87xa_gpio.c);
 * avoids runtime-computed SFR addresses (`inttoptr i16 %reg`) which irparse
 * does not yet support for variable pointers, and avoids the LLVM switch
 * instruction which irparse also does not yet lower. Each port case uses a
 * literal SFR address via if-else so clang emits `inttoptr (i16 <const> to
 * ptr)` and plain branches. */

#include "peripherals/pic16f87xa_gpio.h"
#include "core/pic16_irq.h"

static uint8_t port_width(GPIO_TypeDef port)
{
#if PIC16F87XA_FAMILY_HAS_PORTE
    if (port == GPIOE) return 3U;
#endif
    if (port == GPIOA) return 6U;
    return 8U;
}

void EPIC_GPIO_Init(GPIO_TypeDef port, uint16_t pins, GPIO_ModeTypeDef mode)
{
    uint8_t mask = (uint8_t)pins & (uint8_t)((1U << port_width(port)) - 1U);
    switch (port) {
    case GPIOA: {
        uint8_t tris = EPIC_REG8(PIC_REG_TRISA);
        __asm__ volatile("" ::: "memory");
        if (mode == GPIO_MODE_INPUT || mode == GPIO_MODE_ANALOG) tris |= mask;
        else if (mode == GPIO_MODE_OUTPUT) tris &= (uint8_t)~mask;
        else break;
        EPIC_REG8(PIC_REG_TRISA) = tris;
        __asm__ volatile("" ::: "memory");
        break;
    }
    case GPIOB: {
        uint8_t tris = EPIC_REG8(PIC_REG_TRISB);
        __asm__ volatile("" ::: "memory");
        if (mode == GPIO_MODE_INPUT || mode == GPIO_MODE_ANALOG) tris |= mask;
        else if (mode == GPIO_MODE_OUTPUT) tris &= (uint8_t)~mask;
        else break;
        EPIC_REG8(PIC_REG_TRISB) = tris;
        __asm__ volatile("" ::: "memory");
        break;
    }
    case GPIOC: {
        uint8_t tris = EPIC_REG8(PIC_REG_TRISC);
        __asm__ volatile("" ::: "memory");
        if (mode == GPIO_MODE_INPUT || mode == GPIO_MODE_ANALOG) tris |= mask;
        else if (mode == GPIO_MODE_OUTPUT) tris &= (uint8_t)~mask;
        else break;
        EPIC_REG8(PIC_REG_TRISC) = tris;
        __asm__ volatile("" ::: "memory");
        break;
    }
#if PIC16F87XA_FAMILY_HAS_PORTD
    case GPIOD: {
        uint8_t tris = EPIC_REG8(PIC_REG_TRISD);
        __asm__ volatile("" ::: "memory");
        if (mode == GPIO_MODE_INPUT || mode == GPIO_MODE_ANALOG) tris |= mask;
        else if (mode == GPIO_MODE_OUTPUT) tris &= (uint8_t)~mask;
        else break;
        EPIC_REG8(PIC_REG_TRISD) = tris;
        __asm__ volatile("" ::: "memory");
        break;
    }
#endif
#if PIC16F87XA_FAMILY_HAS_PORTE
    case GPIOE: {
        uint8_t tris = EPIC_REG8(PIC_REG_TRISE);
        __asm__ volatile("" ::: "memory");
        if (mode == GPIO_MODE_INPUT || mode == GPIO_MODE_ANALOG) tris |= mask;
        else if (mode == GPIO_MODE_OUTPUT) tris &= (uint8_t)~mask;
        else break;
        EPIC_REG8(PIC_REG_TRISE) = tris;
        __asm__ volatile("" ::: "memory");
        break;
    }
#endif
    default: break;
    }
}

void EPIC_GPIO_DeInit(GPIO_TypeDef port)
{
    uint8_t v = (uint8_t)((1U << port_width(port)) - 1U);
    if (port == GPIOA) EPIC_REG8(PIC_REG_TRISA) = v;
    else if (port == GPIOB) EPIC_REG8(PIC_REG_TRISB) = v;
    else if (port == GPIOC) EPIC_REG8(PIC_REG_TRISC) = v;
#if PIC16F87XA_FAMILY_HAS_PORTD
    else if (port == GPIOD) EPIC_REG8(PIC_REG_TRISD) = v;
#endif
#if PIC16F87XA_FAMILY_HAS_PORTE
    else if (port == GPIOE) EPIC_REG8(PIC_REG_TRISE) = v;
#endif
    else EPIC_REG8(PIC_REG_TRISA) = v;
}

void EPIC_GPIO_WritePin(GPIO_TypeDef port, uint16_t pins, GPIO_PinState state)
{
    uint8_t mask = (uint8_t)pins & (uint8_t)((1U << port_width(port)) - 1U);
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
#if PIC16F87XA_FAMILY_HAS_PORTD
    if (port == GPIOD) {
        uint8_t cur = EPIC_REG8(PIC_REG_PORTD);
        __asm__ volatile("" ::: "memory");
        if (state == GPIO_PIN_SET) cur |= mask;
        else cur &= (uint8_t)~mask;
        EPIC_REG8(PIC_REG_PORTD) = cur;
        return;
    }
#endif
#if PIC16F87XA_FAMILY_HAS_PORTE
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

void EPIC_GPIO_TogglePin(GPIO_TypeDef port, uint16_t pins)
{
    uint8_t mask = (uint8_t)pins & (uint8_t)((1U << port_width(port)) - 1U);
    if (port == GPIOA) EPIC_REG8(PIC_REG_PORTA) ^= mask;
    else if (port == GPIOB) EPIC_REG8(PIC_REG_PORTB) ^= mask;
    else if (port == GPIOC) EPIC_REG8(PIC_REG_PORTC) ^= mask;
#if PIC16F87XA_FAMILY_HAS_PORTD
    else if (port == GPIOD) EPIC_REG8(PIC_REG_PORTD) ^= mask;
#endif
#if PIC16F87XA_FAMILY_HAS_PORTE
    else if (port == GPIOE) EPIC_REG8(PIC_REG_PORTE) ^= mask;
#endif
    else EPIC_REG8(PIC_REG_PORTA) ^= mask;
}

GPIO_PinState EPIC_GPIO_ReadPin(GPIO_TypeDef port, uint16_t pins)
{
    uint8_t mask = (uint8_t)pins & (uint8_t)((1U << port_width(port)) - 1U);
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
#if PIC16F87XA_FAMILY_HAS_PORTD
    if (port == GPIOD) {
        uint8_t v = EPIC_REG8(PIC_REG_PORTD);
        __asm__ volatile("" ::: "memory");
        return (v & mask) ? GPIO_PIN_SET : GPIO_PIN_RESET;
    }
#endif
#if PIC16F87XA_FAMILY_HAS_PORTE
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

void EPIC_GPIO_WritePort(GPIO_TypeDef port, uint8_t value)
{
    uint8_t mask = (uint8_t)((1U << port_width(port)) - 1U);
    uint8_t v = value & mask;
    if (port == GPIOA) EPIC_REG8(PIC_REG_PORTA) = v;
    else if (port == GPIOB) EPIC_REG8(PIC_REG_PORTB) = v;
    else if (port == GPIOC) EPIC_REG8(PIC_REG_PORTC) = v;
#if PIC16F87XA_FAMILY_HAS_PORTD
    else if (port == GPIOD) EPIC_REG8(PIC_REG_PORTD) = v;
#endif
#if PIC16F87XA_FAMILY_HAS_PORTE
    else if (port == GPIOE) EPIC_REG8(PIC_REG_PORTE) = v;
#endif
    else EPIC_REG8(PIC_REG_PORTA) = v;
}

uint8_t EPIC_GPIO_ReadPort(GPIO_TypeDef port)
{
    if (port == GPIOA) {
        uint8_t v = EPIC_REG8(PIC_REG_PORTA);
        __asm__ volatile("" ::: "memory");
        return v;
    }
    if (port == GPIOB) {
        uint8_t v = EPIC_REG8(PIC_REG_PORTB);
        __asm__ volatile("" ::: "memory");
        return v;
    }
    if (port == GPIOC) {
        uint8_t v = EPIC_REG8(PIC_REG_PORTC);
        __asm__ volatile("" ::: "memory");
        return v;
    }
#if PIC16F87XA_FAMILY_HAS_PORTD
    if (port == GPIOD) {
        uint8_t v = EPIC_REG8(PIC_REG_PORTD);
        __asm__ volatile("" ::: "memory");
        return v;
    }
#endif
#if PIC16F87XA_FAMILY_HAS_PORTE
    if (port == GPIOE) {
        uint8_t v = EPIC_REG8(PIC_REG_PORTE);
        __asm__ volatile("" ::: "memory");
        return v;
    }
#endif
    {
        uint8_t v = EPIC_REG8(PIC_REG_PORTA);
        __asm__ volatile("" ::: "memory");
        return v;
    }
}

void EPIC_GPIO_SetPullups(GPIO_PullTypeDef pull)
{
    uint8_t opt = EPIC_REG8(PIC_REG_OPTION);
    if (pull == GPIO_PULLUP) opt &= (uint8_t)0x7F;
    else opt |= (uint8_t)0x80;
    EPIC_REG8(PIC_REG_OPTION) = opt;
}

static void (*s_rb_change_callback)(uint8_t) = NULL;

void EPIC_GPIO_RegisterChangeCallback(void (*callback)(uint8_t))
{
    s_rb_change_callback = callback;
}

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
