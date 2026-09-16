/* PIC16F5x GPIO implementation (DS41213D §2.0). TRIS direction lives
 * in control space and is WRITE-ONLY on this core: the `tris`
 * instruction has no read path (DS41213D Table 12-1; unlike classic
 * mid-range, where TRISx is a banked file register that reads back),
 * so the driver shadows the direction byte in GPR and re-writes the
 * whole TRIS on every transition. PORTA is RA0..RA3 (4 output-capable
 * pins, 16F54/57/59); PORTB is RB0..RB7 on every part; PORTC/D/E
 * follow the capability macros. No pull-ups, no RB-change interrupt on
 * this die. */

#include "peripherals/pic16f5x_gpio.h"

/**
 * @brief Map a GPIO_TypeDef to the port-select character the TRIS
 *        instruction / platform helper takes.
 * @param port GPIOA..GPIOE.
 * @return 'A'...'E' for a valid port, 'A' for an invalid one.
 */
static char tris_select(GPIO_TypeDef port)
{
    switch (port)
    {
#if PIC16F5X_FAMILY_HAS_PORTA
        case GPIOA: return 'A';
#endif
        case GPIOB: return 'B';
#if PIC16F5X_FAMILY_HAS_PORTC
        case GPIOC: return 'C';
#endif
#if PIC16F5X_FAMILY_HAS_PORTD
        case GPIOD: return 'D';
#endif
#if PIC16F5X_FAMILY_HAS_PORTE
        case GPIOE: return 'E';
#endif
        default:    return 'B';  /* PORTB exists on every part. */
    }
}

/**
 * @brief Map a GPIO_TypeDef to the PORTx register address.
 * @param port GPIOA..GPIOE.
 * @return the PORTx SFR address.
 */
static uint8_t port_addr(GPIO_TypeDef port)
{
    switch (port)
    {
#if PIC16F5X_FAMILY_HAS_PORTA
        case GPIOA: return PIC_REG_PORTA;
#endif
        case GPIOB: return PIC_REG_PORTB;
#if PIC16F5X_FAMILY_HAS_PORTC
        case GPIOC: return PIC_REG_PORTC;
#endif
#if PIC16F5X_FAMILY_HAS_PORTD
        case GPIOD: return PIC_REG_PORTD;
#endif
#if PIC16F5X_FAMILY_HAS_PORTE
        case GPIOE: return PIC_REG_PORTE;
#endif
        default:    return PIC_REG_PORTB;  /* PORTB exists on every part. */
    }
}

/**
 * @brief Implemented-pin mask for a port. PORTA is RA0..RA3
 *        (DS41213D §2.0); PORTE is RE0..RE1 on the 16F59 (2 pins).
 *        Every other port is contiguous from bit 0.
 * @param port GPIOA..GPIOE.
 * @return the bitmask of implemented pins.
 */
static uint8_t port_pin_mask(GPIO_TypeDef port)
{
#if PIC16F5X_FAMILY_HAS_PORTE
    if (port == GPIOE) return 0x03U;
#endif
#if PIC16F5X_FAMILY_HAS_PORTA
    if (port == GPIOA) return 0x0FU;
#endif
    return 0xFFU;
}

/* TRIS shadow: the write-only control register is read-modify-cycled
 * in GPR. Sized to the part's actual port count (A and B always exist;
 * the wider ports follow the capability macros), so the 25 B RAM
 * 16F54 exemplar carries only 2 bytes, not the 5 the 40-pin 16F59's
 * A..E surface needs. Indexing rides the port enum values, which are
 * contiguous from GPIOA once the wider parts are in scope. */
static uint8_t s_tris[2
#if PIC16F5X_FAMILY_HAS_PORTC
                      + 1
#endif
#if PIC16F5X_FAMILY_HAS_PORTD
                      + 1
#endif
#if PIC16F5X_FAMILY_HAS_PORTE
                      + 1
#endif
                      ] = {0xFFU, 0xFFU
#if PIC16F5X_FAMILY_HAS_PORTC
                           , 0xFFU
#endif
#if PIC16F5X_FAMILY_HAS_PORTD
                           , 0xFFU
#endif
#if PIC16F5X_FAMILY_HAS_PORTE
                           , 0xFFU
#endif
                          };

/* init / deinit. */

/**
 * @brief Configure one or more pins of a port as input or output.
 * @param port GPIOA..GPIOE.
 * @param pins bitmask of pins to configure.
 * @param mode GPIO_MODE_INPUT or GPIO_MODE_OUTPUT.
 */
void EPIC_GPIO_Init(GPIO_TypeDef port, uint16_t pins, GPIO_ModeTypeDef mode)
{
    uint8_t mask = (uint8_t)pins & port_pin_mask(port);
    uint8_t ta = (uint8_t)port;

    switch (mode)
    {
        case GPIO_MODE_INPUT:
            s_tris[ta] |= mask;
            break;
        case GPIO_MODE_OUTPUT:
            s_tris[ta] &= (uint8_t)~mask;
            break;
        default:
            return;
    }
    EPIC_TRIS_WRITE(tris_select(port), s_tris[ta]);
}

/**
 * @brief Restore all pins of a port to input mode and clear the latch.
 * @param port GPIOA..GPIOE.
 */
void EPIC_GPIO_DeInit(GPIO_TypeDef port)
{
    uint8_t ta = (uint8_t)port;
    s_tris[ta] = port_pin_mask(port);
    EPIC_TRIS_WRITE(tris_select(port), s_tris[ta]);
    EPIC_REG8(port_addr(port)) = 0x00U;
}

/* read / write / toggle. */

/**
 * @brief Drive a set of pins high or low.
 * @param port GPIOA..GPIOE.
 * @param pins bitmask of pins to drive.
 * @param state GPIO_PIN_SET or GPIO_PIN_RESET.
 */
void EPIC_GPIO_WritePin(GPIO_TypeDef port, uint16_t pins, GPIO_PinState state)
{
    uint8_t mask = (uint8_t)pins & port_pin_mask(port);
    uint8_t pa = port_addr(port);
    uint8_t cur = EPIC_REG8(pa);
    if (state == GPIO_PIN_SET) cur |= mask;
    else                       cur &= (uint8_t)~mask;
    EPIC_REG8(pa) = cur;
}

/**
 * @brief Invert a set of pins.
 * @param port GPIOA..GPIOE.
 * @param pins bitmask of pins to toggle.
 */
void EPIC_GPIO_TogglePin(GPIO_TypeDef port, uint16_t pins)
{
    uint8_t mask = (uint8_t)pins & port_pin_mask(port);
    uint8_t pa = port_addr(port);
    EPIC_REG8(pa) = EPIC_REG8(pa) ^ mask;
}

/**
 * @brief Read the current level of a set of pins.
 * @param port GPIOA..GPIOE.
 * @param pins bitmask of pins to sample.
 * @return GPIO_PIN_SET if any sampled pin is high, else GPIO_PIN_RESET.
 */
GPIO_PinState EPIC_GPIO_ReadPin(GPIO_TypeDef port, uint16_t pins)
{
    /* TRIS=1 (input) returns the pin state; TRIS=0 (output) the latch.
     * The sim backend implements the same behavior; XC8 lowers this to
     * a single MOVF on the real target. */
    uint8_t mask = (uint8_t)pins & port_pin_mask(port);
    uint8_t pa = port_addr(port);
    return (EPIC_REG8(pa) & mask) ? GPIO_PIN_SET : GPIO_PIN_RESET;
}

/**
 * @brief Write the whole port latch.
 * @param port GPIOA..GPIOE.
 * @param value the byte to write (unimplemented bits are masked off).
 */
void EPIC_GPIO_WritePort(GPIO_TypeDef port, uint8_t value)
{
    uint8_t mask = port_pin_mask(port);
    EPIC_REG8(port_addr(port)) = (uint8_t)(value & mask);
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
