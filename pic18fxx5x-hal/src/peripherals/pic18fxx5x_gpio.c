/**
 * @file    pic18fxx5x_gpio.c
 * @brief   GPIO driver, implementation matching DS39632E §10.0.
 *
 * @details
 *   PIC18 exposes the output latch as its own register, LATx, so this
 *   driver writes LATx (not PORTx) and reads PORTx, per §10.0. Direction
 *   is programmed in TRISx. PORTB pull-ups live in INTCON2<RBPU>.
 */

#include "peripherals/pic18fxx5x_gpio.h"
#include "core/pic18_irq.h"

/** Map a GPIO_TypeDef to the address of its TRIS register. */
static uint16_t tris_addr(GPIO_TypeDef port)
{
    switch (port) {
        case GPIOA: return PIC_REG_TRISA;
        case GPIOB: return PIC_REG_TRISB;
        case GPIOC: return PIC_REG_TRISC;
#if PIC18FXX5X_FAMILY_HAS_PORTD
        case GPIOD: return PIC_REG_TRISD;
#endif
#if PIC18FXX5X_FAMILY_HAS_PORTE
        case GPIOE: return PIC_REG_TRISE;
#endif
        default:    return PIC_REG_TRISA;
    }
}

/** Map a GPIO_TypeDef to the address of its LAT (output latch) register. */
static uint16_t lat_addr(GPIO_TypeDef port)
{
    switch (port) {
        case GPIOA: return PIC_REG_LATA;
        case GPIOB: return PIC_REG_LATB;
        case GPIOC: return PIC_REG_LATC;
#if PIC18FXX5X_FAMILY_HAS_PORTD
        case GPIOD: return PIC_REG_LATD;
#endif
#if PIC18FXX5X_FAMILY_HAS_PORTE
        case GPIOE: return PIC_REG_LATE;
#endif
        default:    return PIC_REG_LATA;
    }
}

/** Map a GPIO_TypeDef to the address of its PORT (pin sample) register. */
static uint16_t port_addr(GPIO_TypeDef port)
{
    switch (port) {
        case GPIOA: return PIC_REG_PORTA;
        case GPIOB: return PIC_REG_PORTB;
        case GPIOC: return PIC_REG_PORTC;
#if PIC18FXX5X_FAMILY_HAS_PORTD
        case GPIOD: return PIC_REG_PORTD;
#endif
#if PIC18FXX5X_FAMILY_HAS_PORTE
        case GPIOE: return PIC_REG_PORTE;
#endif
        default:    return PIC_REG_PORTA;
    }
}

/**
 * @brief  Upper pin bound for a port. PORTA = 6, PORTE = 3, others = 8.
 *         Anything above this is unimplemented (DS39632E Table 10-1..10-5).
 */
static uint8_t port_width(GPIO_TypeDef port)
{
#if PIC18FXX5X_FAMILY_HAS_PORTE
    if (port == GPIOE) return 3U;
#endif
    if (port == GPIOA) return 6U;
    return 8U;
}

/* ───────────────────────── init / deinit ────────────────────────── */

void EPIC_GPIO_Init(GPIO_TypeDef port, uint16_t pins, GPIO_ModeTypeDef mode)
{
    uint16_t ta = tris_addr(port);
    uint8_t mask = (uint8_t)pins & (uint8_t)((1U << port_width(port)) - 1U);
    uint8_t tris = PIC8_REG8(ta);

    switch (mode) {
        case GPIO_MODE_INPUT:
        case GPIO_MODE_ANALOG:
            /* Both modes set TRIS=1 (input). Analog mode additionally
             * requires ADC configuration, which the user does separately. */
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
    uint16_t ta = tris_addr(port);
    /* Reset all implemented bits of TRISx to 1 = input, clear the latch. */
    PIC8_REG8(ta) = (uint8_t)((1U << port_width(port)) - 1U);
    PIC8_REG8(lat_addr(port)) = 0x00U;
}

/* ───────────────────────── read / write / toggle ────────────────── */

void EPIC_GPIO_WritePin(GPIO_TypeDef port, uint16_t pins, GPIO_PinState state)
{
    uint8_t mask = (uint8_t)pins & (uint8_t)((1U << port_width(port)) - 1U);
    uint16_t la = lat_addr(port);
    uint8_t cur = PIC8_REG8(la);
    if (state == GPIO_PIN_SET) cur |= mask;
    else                       cur &= (uint8_t)~mask;
    PIC8_REG8(la) = cur;          /* write the latch, DS39632E §10.0 */
}

void EPIC_GPIO_TogglePin(GPIO_TypeDef port, uint16_t pins)
{
    uint8_t mask = (uint8_t)pins & (uint8_t)((1U << port_width(port)) - 1U);
    uint16_t la = lat_addr(port);
    PIC8_REG8(la) = (uint8_t)(PIC8_REG8(la) ^ mask);
}

GPIO_PinState EPIC_GPIO_ReadPin(GPIO_TypeDef port, uint16_t pins)
{
    uint8_t mask = (uint8_t)pins & (uint8_t)((1U << port_width(port)) - 1U);
    /* Read PORTx: pin state for inputs, latched value for outputs. The sim
     * backend models the same; on a real XC8 target this lowers to one
     * MOVF PORTx. */
    return (PIC8_REG8(port_addr(port)) & mask) ? GPIO_PIN_SET : GPIO_PIN_RESET;
}

void EPIC_GPIO_WritePort(GPIO_TypeDef port, uint8_t value)
{
    uint8_t mask = (uint8_t)((1U << port_width(port)) - 1U);
    PIC8_REG8(lat_addr(port)) = (uint8_t)(value & mask);
}

uint8_t EPIC_GPIO_ReadPort(GPIO_TypeDef port)
{
    return PIC8_REG8(port_addr(port));
}

/* ───────────────────────── PORTB pull-ups ───────────────────────── */

void EPIC_GPIO_SetPullups(GPIO_PullTypeDef pull)
{
    /* INTCON2<RBPU> (bit 7), active-low: 1 = disabled, 0 = enabled
     * (DS39632E §10.2, Register 9-2). */
    if (pull == GPIO_PULLUP) {
        PIC8_BIT_CLR(PIC8_REG8(PIC_REG_INTCON2), PIC_INTCON2_RBPU);
    } else {
        PIC8_BIT_SET(PIC8_REG8(PIC_REG_INTCON2), PIC_INTCON2_RBPU);
    }
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
    if (!EPIC_IRQ_GetFlag(PIC18_IRQ_RB)) return;

    /* MUST read PORTB before clearing RBIF, DS39632E §9.0: the mismatch
     * comparator latches the value at the last CPU read of PORTB, so the
     * read is what ends the current mismatch condition and re-arms the next
     * one. Clearing the flag first (or not reading at all) risks an
     * immediate spurious re-interrupt or a silently-missed change. The
     * callback gets this already-read byte, never a second later read. */
    uint8_t portb = PIC8_REG8(PIC_REG_PORTB);
    EPIC_IRQ_ClearFlag(PIC18_IRQ_RB);
    if (s_rb_change_callback) s_rb_change_callback(portb);
}
