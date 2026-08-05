/**
 * @file    pic16f87xa_gpio.c
 * @brief   GPIO driver, implementation matching datasheet §4.1..§4.5.
 */

#include "peripherals/pic16f87xa_gpio.h"
#include "core/pic16_irq.h"

/**
 * @brief  Map a GPIO_TypeDef to the address of its TRIS register.
 *         TRISx is always at PORTx | 0x80 in the banked register file
 *         (DS39582B Figure 2-3 / 2-4).
 */
static uint8_t tris_addr(GPIO_TypeDef port)
{
    switch (port) {
        case GPIOA: return PIC_REG_TRISA;
        case GPIOB: return PIC_REG_TRISB;
        case GPIOC: return PIC_REG_TRISC;
#if PIC16F87XA_FAMILY_HAS_PORTD
        case GPIOD: return PIC_REG_TRISD;
#endif
#if PIC16F87XA_FAMILY_HAS_PORTE
        case GPIOE: return PIC_REG_TRISE;
#endif
        default:    return PIC_REG_TRISA;
    }
}

/** Map a GPIO_TypeDef to the PORTx register address. */
static uint8_t port_addr(GPIO_TypeDef port)
{
    switch (port) {
        case GPIOA: return PIC_REG_PORTA;
        case GPIOB: return PIC_REG_PORTB;
        case GPIOC: return PIC_REG_PORTC;
#if PIC16F87XA_FAMILY_HAS_PORTD
        case GPIOD: return PIC_REG_PORTD;
#endif
#if PIC16F87XA_FAMILY_HAS_PORTE
        case GPIOE: return PIC_REG_PORTE;
#endif
        default:    return PIC_REG_PORTA;
    }
}

/**
 * @brief  Upper pin bound for a port. PORTA = 6, PORTE = 3, others = 8.
 *         Anything above this is unimplemented (DS39582B Table 4-1..4-9,
 *         marked with a dash in the datasheet).
 */
static uint8_t port_width(GPIO_TypeDef port)
{
#if PIC16F87XA_FAMILY_HAS_PORTE
    if (port == GPIOE) return 3U;
#endif
    if (port == GPIOA) return 6U;
    return 8U;
}

/* ───────────────────────── init / deinit ────────────────────────── */

void EPIC_GPIO_Init(GPIO_TypeDef port, uint16_t pins, GPIO_ModeTypeDef mode)
{
    uint8_t ta = tris_addr(port);
    uint8_t mask   = (uint8_t)pins & (uint8_t)((1U << port_width(port)) - 1U);

    uint8_t tris = PIC8_REG8(ta);

    switch (mode) {
        case GPIO_MODE_INPUT:
        case GPIO_MODE_ANALOG:
            /* Both modes set TRIS=1 (input). Analog mode additionally
             * requires ADCON1 configuration, which the user does separately
             * via EPIC_ADC_ConfigChannels(). */
            tris |= mask;
            break;
        case GPIO_MODE_OUTPUT:
            tris &= (uint8_t)~mask;
            break;
        default:
            return;
    }
    PIC8_REG8(ta) = tris;
}

void EPIC_GPIO_DeInit(GPIO_TypeDef port)
{
    uint8_t ta = tris_addr(port);
    /* Reset all implemented bits of TRISx to 1 = input. */
    PIC8_REG8(ta) = (uint8_t)((1U << port_width(port)) - 1U);
}

/* ───────────────────────── read / write / toggle ────────────────── */

void EPIC_GPIO_WritePin(GPIO_TypeDef port, uint16_t pins, GPIO_PinState state)
{
    uint8_t mask = (uint8_t)pins & (uint8_t)((1U << port_width(port)) - 1U);
    uint8_t pa = port_addr(port);
    uint8_t cur = PIC8_REG8(pa);
    if (state == GPIO_PIN_SET) cur |= mask;
    else                       cur &= (uint8_t)~mask;
    PIC8_REG8(pa) = cur;
}

void EPIC_GPIO_TogglePin(GPIO_TypeDef port, uint16_t pins)
{
    uint8_t mask = (uint8_t)pins & (uint8_t)((1U << port_width(port)) - 1U);
    uint8_t pa = port_addr(port);
    PIC8_REG8(pa) = PIC8_REG8(pa) ^ mask;
}

GPIO_PinState EPIC_GPIO_ReadPin(GPIO_TypeDef port, uint16_t pins)
{
    /* Reading the PIN level semantics:
     *   - If TRIS bit is 1 (input) → returns pin state.
     *   - If TRIS bit is 0 (output) → returns the latch.
     * The sim backend implements the same behavior; on a real XC8
     * target the compiler lowers this to a single MOVF. */
    uint8_t mask = (uint8_t)pins & (uint8_t)((1U << port_width(port)) - 1U);
    uint8_t pa = port_addr(port);
    return (PIC8_REG8(pa) & mask) ? GPIO_PIN_SET : GPIO_PIN_RESET;
}

void EPIC_GPIO_WritePort(GPIO_TypeDef port, uint8_t value)
{
    uint8_t mask = (uint8_t)((1U << port_width(port)) - 1U);
    PIC8_REG8(port_addr(port)) = (uint8_t)(value & mask);
}

uint8_t EPIC_GPIO_ReadPort(GPIO_TypeDef port)
{
    return PIC8_REG8(port_addr(port));
}

/* ───────────────────────── PORTB pull-ups ───────────────────────── */

void EPIC_GPIO_SetPullups(GPIO_PullTypeDef pull)
{
    uint8_t opt = PIC8_REG8(PIC_REG_OPTION);
    if (pull == GPIO_PULLUP) {
        PIC8_BIT_CLR(opt, (uint8_t)0x80);    /* RBPU = 0 → enabled */
    } else {
        PIC8_BIT_SET(opt, (uint8_t)0x80);    /* RBPU = 1 → disabled */
    }
    PIC8_REG8(PIC_REG_OPTION) = opt;
}

/* ───────────────────────── PORTB change interrupt ───────────────────── */

/* One callback slot for the whole-port RB<7:4> change interrupt. There is
 * only one PORTB, so there is no handle struct (mirrors Timer2's
 * one-callback-per-handle shape but simpler). NULL = unregistered/no-op. */
static void (*s_rb_change_callback)(uint8_t) = NULL;

void EPIC_GPIO_RegisterChangeCallback(void (*callback)(uint8_t))
{
    s_rb_change_callback = callback;
}

void RB_IRQHandler(void)
{
    if (!EPIC_IRQ_GetFlag(PIC16_IRQ_RB)) return;

    /* MUST read PORTB before clearing RBIF (DS39582B §14.11.3): the
     * mismatch comparator only re-arms once PORTB is read, so reading
     * after clearing risks a spurious re-interrupt or a missed change. */
    uint8_t portb = PIC8_REG8(PIC_REG_PORTB);
    EPIC_IRQ_ClearFlag(PIC16_IRQ_RB);
    if (s_rb_change_callback) s_rb_change_callback(portb);
}
