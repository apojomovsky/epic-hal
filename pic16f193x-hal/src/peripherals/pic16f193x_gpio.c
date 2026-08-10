/**
 * @file    pic16f193x_gpio.c
 * @brief   GPIO driver, implementation matching datasheet §6.0/§7.0.
 *
 * @details
 *   Enhanced Mid-range I/O model: writes target LATx, reads come from
 *   PORTx (pin level); ANSELx selects analog/digital per pin; WPUB gives
 *   per-pin PORTB weak pull-ups gated by the global WPUEN (OPTION_REG<7>).
 *   All SFR accesses use compile-time-constant `PIC_REG_*` tokens; the
 *   per-port dispatch branches before touching any SFR (the proven
 *   pattern from pic18_irq.c / pic18fxx5x_ccp.c).
 */

#include "peripherals/pic16f193x_gpio.h"
#include "core/pic16f193x_irq.h"

/* ───────────────────────── per-port register addresses ─────────── */

static uint16_t tris_addr(GPIO_TypeDef port)
{
    switch (port) {
        case GPIOA: return PIC_REG_TRISA;
        case GPIOB: return PIC_REG_TRISB;
        case GPIOC: return PIC_REG_TRISC;
#if PIC16F193X_FAMILY_HAS_PORTD
        case GPIOD: return PIC_REG_TRISD;
#endif
#if PIC16F193X_FAMILY_HAS_PORTE
        case GPIOE: return PIC_REG_TRISE;
#endif
        default:    return PIC_REG_TRISA;
    }
}

static uint16_t lat_addr(GPIO_TypeDef port)
{
    switch (port) {
        case GPIOA: return PIC_REG_LATA;
        case GPIOB: return PIC_REG_LATB;
        case GPIOC: return PIC_REG_LATC;
#if PIC16F193X_FAMILY_HAS_PORTD
        case GPIOD: return PIC_REG_LATD;
#endif
#if PIC16F193X_FAMILY_HAS_PORTE
        case GPIOE: return PIC_REG_LATE;
#endif
        default:    return PIC_REG_LATA;
    }
}

static uint16_t port_addr(GPIO_TypeDef port)
{
    switch (port) {
        case GPIOA: return PIC_REG_PORTA;
        case GPIOB: return PIC_REG_PORTB;
        case GPIOC: return PIC_REG_PORTC;
#if PIC16F193X_FAMILY_HAS_PORTD
        case GPIOD: return PIC_REG_PORTD;
#endif
#if PIC16F193X_FAMILY_HAS_PORTE
        case GPIOE: return PIC_REG_PORTE;
#endif
        default:    return PIC_REG_PORTA;
    }
}

static uint16_t ansel_addr(GPIO_TypeDef port)
{
    switch (port) {
        case GPIOA: return PIC_REG_ANSELA;
        case GPIOB: return PIC_REG_ANSELB;
#if PIC16F193X_FAMILY_HAS_PORTD
        case GPIOD: return PIC_REG_ANSELD;
#endif
#if PIC16F193X_FAMILY_HAS_PORTE
        case GPIOE: return PIC_REG_ANSELE;
#endif
        /* PORTC has no ANSEL on this family (all digital); fall back to a
         * harmless sentinel the caller never writes when width gates it. */
        default:    return 0xFFFFU;
    }
}

/** Upper pin bound for a port. PORTA/B/C/D = 8, PORTE = 4. */
static uint8_t port_width(GPIO_TypeDef port)
{
#if PIC16F193X_FAMILY_HAS_PORTE
    if (port == GPIOE) return 4U;
#endif
    return 8U;
}

/* ───────────────────────── init / deinit ────────────────────────── */

void EPIC_GPIO_Init(GPIO_TypeDef port, uint16_t pins, GPIO_ModeTypeDef mode)
{
    uint8_t mask = (uint8_t)pins & (uint8_t)((1U << port_width(port)) - 1U);
    uint16_t ta  = tris_addr(port);
    uint16_t la  = lat_addr(port);
    uint16_t aa  = ansel_addr(port);

    uint8_t tris  = EPIC_REG8(ta);
    uint8_t lat   = EPIC_REG8(la);
    uint8_t ansel = (aa != 0xFFFFU) ? EPIC_REG8(aa) : 0U;

    switch (mode) {
        case GPIO_MODE_INPUT:
            tris  |= mask;
            ansel &= (uint8_t)~mask;   /* digital input. */
            break;
        case GPIO_MODE_OUTPUT:
            tris  &= (uint8_t)~mask;
            ansel &= (uint8_t)~mask;   /* digital output. */
            lat   &= (uint8_t)~mask;   /* start driving low. */
            break;
        case GPIO_MODE_ANALOG:
            tris  |= mask;
            ansel |= mask;
            break;
        default:
            return;
    }
    EPIC_REG8(ta) = tris;
    if (aa != 0xFFFFU) EPIC_REG8(aa) = ansel;
    EPIC_REG8(la) = lat;
}

void EPIC_GPIO_DeInit(GPIO_TypeDef port)
{
    uint16_t ta = tris_addr(port);
    uint16_t la = lat_addr(port);
    uint16_t aa = ansel_addr(port);
    /* Reset to POR: input, analog, latch clear. */
    EPIC_REG8(ta) = (uint8_t)((1U << port_width(port)) - 1U);
    if (aa != 0xFFFFU) EPIC_REG8(aa) = (uint8_t)((1U << port_width(port)) - 1U);
    EPIC_REG8(la) = 0x00U;
}

/* ───────────────────────── read / write / toggle ────────────────── */

void EPIC_GPIO_WritePin(GPIO_TypeDef port, uint16_t pins, GPIO_PinState state)
{
    uint8_t mask = (uint8_t)pins & (uint8_t)((1U << port_width(port)) - 1U);
    uint16_t la = lat_addr(port);
    uint8_t cur = EPIC_REG8(la);
    if (state == GPIO_PIN_SET) cur |= mask;
    else                       cur &= (uint8_t)~mask;
    EPIC_REG8(la) = cur;
}

void EPIC_GPIO_TogglePin(GPIO_TypeDef port, uint16_t pins)
{
    uint8_t mask = (uint8_t)pins & (uint8_t)((1U << port_width(port)) - 1U);
    uint16_t la = lat_addr(port);
    EPIC_REG8(la) = EPIC_REG8(la) ^ mask;
}

GPIO_PinState EPIC_GPIO_ReadPin(GPIO_TypeDef port, uint16_t pins)
{
    uint8_t mask = (uint8_t)pins & (uint8_t)((1U << port_width(port)) - 1U);
    uint16_t pa = port_addr(port);
    return (EPIC_REG8(pa) & mask) ? GPIO_PIN_SET : GPIO_PIN_RESET;
}

void EPIC_GPIO_WritePort(GPIO_TypeDef port, uint8_t value)
{
    uint8_t mask = (uint8_t)((1U << port_width(port)) - 1U);
    EPIC_REG8(lat_addr(port)) = (uint8_t)(value & mask);
}

uint8_t EPIC_GPIO_ReadPort(GPIO_TypeDef port)
{
    return EPIC_REG8(port_addr(port));
}

/* ───────────────────────── PORTB weak pull-ups ─────────────────── */

void EPIC_GPIO_SetPullups(GPIO_TypeDef port, uint16_t pins, GPIO_PinState state)
{
    /* Weak pull-ups on this family are PORTB-only via WPUB (DS41364B
     * §6.0), gated by the global WPUEN (OPTION_REG<7>, active-low). */
    if (port != GPIOB) return;
    uint8_t mask = (uint8_t)pins & (uint8_t)((1U << port_width(port)) - 1U);

    if (state == GPIO_PIN_SET) {
        EPIC_REG8(PIC_REG_WPUB) |= mask;
        /* WPUEN = 0 enables the per-pin pull-ups. */
        EPIC_BIT_CLR(EPIC_REG8(PIC_REG_OPTION), PIC_OPTION_WPUEN);
    } else {
        EPIC_REG8(PIC_REG_WPUB) &= (uint8_t)~mask;
        /* If no PORTB pin still wants a pull-up, disable globally. */
        if (EPIC_REG8(PIC_REG_WPUB) == 0U) {
            EPIC_BIT_SET(EPIC_REG8(PIC_REG_OPTION), PIC_OPTION_WPUEN);
        }
    }
}

/* ───────────────────────── PORTB change interrupt ───────────────────── */

static void (*s_ioc_callback)(uint8_t iocbf, uint8_t portb) = NULL;

void EPIC_GPIO_RegisterChangeCallback(void (*callback)(uint8_t, uint8_t))
{
    s_ioc_callback = callback;
}

void EPIC_GPIO_EnableChangeDetect(uint8_t pos_mask, uint8_t neg_mask)
{
    EPIC_REG8(PIC_REG_IOCBP) = pos_mask;
    EPIC_REG8(PIC_REG_IOCBN) = neg_mask;
}

void IOC_IRQHandler(void)
{
    /* Direct flag ops (class-F: the table route clobbers PCLATH in ISR
     * context; the ccp_irq_common switch is the reference). IOCIF is
     * INTCON bit 0. */
    if (!(EPIC_REG8(PIC_REG_INTCON) & PIC_INTCON_IOCIF)) return;

    /* Read IOCBF (which pins changed) and PORTB before clearing, the
     * mismatch comparator only re-arms once PORTB is read (DS41364B
     * §7.0), matching classic PIC16's read-before-clear requirement. */
    uint8_t iocbf = EPIC_REG8(PIC_REG_IOCBF);
    uint8_t portb = EPIC_REG8(PIC_REG_PORTB);
    EPIC_REG8(PIC_REG_IOCBF) = 0x00U;
    EPIC_BIT_CLR(EPIC_REG8(PIC_REG_INTCON), PIC_INTCON_IOCIF);
    if (s_ioc_callback) s_ioc_callback(iocbf, portb);
}
