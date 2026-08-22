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
    uint8_t tris;
    if (port == GPIOA) tris = EPIC_REG8(PIC_REG_TRISA);
    else if (port == GPIOB) tris = EPIC_REG8(PIC_REG_TRISB);
    else if (port == GPIOC) tris = EPIC_REG8(PIC_REG_TRISC);
#if PIC16F87XA_FAMILY_HAS_PORTD
    else if (port == GPIOD) tris = EPIC_REG8(PIC_REG_TRISD);
#endif
#if PIC16F87XA_FAMILY_HAS_PORTE
    else if (port == GPIOE) tris = EPIC_REG8(PIC_REG_TRISE);
#endif
    else tris = EPIC_REG8(PIC_REG_TRISA);
    if (mode == GPIO_MODE_INPUT || mode == GPIO_MODE_ANALOG) tris |= mask;
    else if (mode == GPIO_MODE_OUTPUT) tris &= (uint8_t)~mask;
    else return;
    if (port == GPIOA) EPIC_REG8(PIC_REG_TRISA) = tris;
    else if (port == GPIOB) EPIC_REG8(PIC_REG_TRISB) = tris;
    else if (port == GPIOC) EPIC_REG8(PIC_REG_TRISC) = tris;
#if PIC16F87XA_FAMILY_HAS_PORTD
    else if (port == GPIOD) EPIC_REG8(PIC_REG_TRISD) = tris;
#endif
#if PIC16F87XA_FAMILY_HAS_PORTE
    else if (port == GPIOE) EPIC_REG8(PIC_REG_TRISE) = tris;
#endif
    else EPIC_REG8(PIC_REG_TRISA) = tris;
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
    uint8_t cur;
    if (port == GPIOA) cur = EPIC_REG8(PIC_REG_PORTA);
    else if (port == GPIOB) cur = EPIC_REG8(PIC_REG_PORTB);
    else if (port == GPIOC) cur = EPIC_REG8(PIC_REG_PORTC);
#if PIC16F87XA_FAMILY_HAS_PORTD
    else if (port == GPIOD) cur = EPIC_REG8(PIC_REG_PORTD);
#endif
#if PIC16F87XA_FAMILY_HAS_PORTE
    else if (port == GPIOE) cur = EPIC_REG8(PIC_REG_PORTE);
#endif
    else cur = EPIC_REG8(PIC_REG_PORTA);
    if (state == GPIO_PIN_SET) cur |= mask;
    else cur &= (uint8_t)~mask;
    if (port == GPIOA) EPIC_REG8(PIC_REG_PORTA) = cur;
    else if (port == GPIOB) EPIC_REG8(PIC_REG_PORTB) = cur;
    else if (port == GPIOC) EPIC_REG8(PIC_REG_PORTC) = cur;
#if PIC16F87XA_FAMILY_HAS_PORTD
    else if (port == GPIOD) EPIC_REG8(PIC_REG_PORTD) = cur;
#endif
#if PIC16F87XA_FAMILY_HAS_PORTE
    else if (port == GPIOE) EPIC_REG8(PIC_REG_PORTE) = cur;
#endif
    else EPIC_REG8(PIC_REG_PORTA) = cur;
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
    uint8_t v;
    if (port == GPIOA) v = EPIC_REG8(PIC_REG_PORTA);
    else if (port == GPIOB) v = EPIC_REG8(PIC_REG_PORTB);
    else if (port == GPIOC) v = EPIC_REG8(PIC_REG_PORTC);
#if PIC16F87XA_FAMILY_HAS_PORTD
    else if (port == GPIOD) v = EPIC_REG8(PIC_REG_PORTD);
#endif
#if PIC16F87XA_FAMILY_HAS_PORTE
    else if (port == GPIOE) v = EPIC_REG8(PIC_REG_PORTE);
#endif
    else v = EPIC_REG8(PIC_REG_PORTA);
    return (v & mask) ? GPIO_PIN_SET : GPIO_PIN_RESET;
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
    if (port == GPIOA) return EPIC_REG8(PIC_REG_PORTA);
    if (port == GPIOB) return EPIC_REG8(PIC_REG_PORTB);
    if (port == GPIOC) return EPIC_REG8(PIC_REG_PORTC);
#if PIC16F87XA_FAMILY_HAS_PORTD
    if (port == GPIOD) return EPIC_REG8(PIC_REG_PORTD);
#endif
#if PIC16F87XA_FAMILY_HAS_PORTE
    if (port == GPIOE) return EPIC_REG8(PIC_REG_PORTE);
#endif
    return EPIC_REG8(PIC_REG_PORTA);
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
