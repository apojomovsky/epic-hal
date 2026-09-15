/*
 * GPIO driver, implementation (DS39605F §5.0/§6.0). PIC18 exposes the
 * output latch as its own register, LATx, so this driver writes LATx
 * (not PORTx) and reads PORTx. Direction is programmed in TRISx. PORTB
 * pull-ups live in INTCON2<RBPU>. Both ports are full 8-bit on this
 * part; no per-port width table is needed (unlike pic18fxx5x-hal's
 * 6-bit PORTA / 3-bit PORTE).
 */

#include "peripherals/pic18f1320_gpio.h"
#include "core/pic18_irq.h"

/**
 * @brief  Return the TRISx register address for a port.
 * @param port Port whose direction register is wanted.
 * @return The TRISx SFR address for `port`.
 */
static uint16_t tris_addr(GPIO_TypeDef port)
{
    return (port == GPIOB) ? PIC_REG_TRISB : PIC_REG_TRISA;
}

/**
 * @brief  Return the LATx output-latch register address for a port.
 * @param port Port whose latch register is wanted.
 * @return The LATx SFR address for `port`.
 */
static uint16_t lat_addr(GPIO_TypeDef port)
{
    return (port == GPIOB) ? PIC_REG_LATB : PIC_REG_LATA;
}

/**
 * @brief  Return the PORTx input register address for a port.
 * @param port Port whose input register is wanted.
 * @return The PORTx SFR address for `port`.
 */
static uint16_t port_addr(GPIO_TypeDef port)
{
    return (port == GPIOB) ? PIC_REG_PORTB : PIC_REG_PORTA;
}

/**
 * @brief  Initialize a set of pins on a port to the given mode by
 *         programming the TRISx register (input/analog set the pin as
 *         input; output clears the direction bit).
 * @param port Port to configure.
 * @param pins Bitmask of pins to configure.
 * @param mode Desired pin mode (input, analog or output).
 */
void EPIC_GPIO_Init(GPIO_TypeDef port, uint16_t pins, GPIO_ModeTypeDef mode)
{
    uint16_t ta = tris_addr(port);
    uint8_t mask = (uint8_t)pins;
    uint8_t tris = EPIC_REG8(ta);

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
    EPIC_REG8(ta) = tris;
}

/**
 * @brief  Restore all pins of `port` to input mode and clear the output
 *         latch.
 * @param port Port to reset.
 */
void EPIC_GPIO_DeInit(GPIO_TypeDef port)
{
    EPIC_REG8(tris_addr(port)) = 0xFFU;
    EPIC_REG8(lat_addr(port))  = 0x00U;
}

/**
 * @brief  Drive a set of pins high or low. Writes the LATx latch directly
 *         (DS39605F §5.0/§6.0), the PIC18-native way (no read-modify-write
 *         of PORTx).
 * @param port Port whose pins are driven.
 * @param pins Bitmask of pins to drive.
 * @param state GPIO_PIN_SET or GPIO_PIN_RESET.
 */
void EPIC_GPIO_WritePin(GPIO_TypeDef port, uint16_t pins, GPIO_PinState state)
{
    uint8_t mask = (uint8_t)pins;
    uint16_t la = lat_addr(port);
    uint8_t cur = EPIC_REG8(la);
    if (state == GPIO_PIN_SET) cur |= mask;
    else                       cur &= (uint8_t)~mask;
    EPIC_REG8(la) = cur;          /* write the latch, DS39605F §5.0/§6.0 */
}

/**
 * @brief  Toggle a set of pins by XORing the output latch (LATx ^= mask).
 * @param port Port whose pins are toggled.
 * @param pins Bitmask of pins to toggle.
 */
void EPIC_GPIO_TogglePin(GPIO_TypeDef port, uint16_t pins)
{
    uint8_t mask = (uint8_t)pins;
    uint16_t la = lat_addr(port);
    EPIC_REG8(la) = (uint8_t)(EPIC_REG8(la) ^ mask);
}

/**
 * @brief  Read the current level seen on `pins` from PORTx. For pins
 *         configured as outputs this returns the latched value; for input
 *         pins it returns whatever the pin is being driven to externally.
 * @param port Port to read.
 * @param pins Bitmask of pins to sample.
 * @return GPIO_PIN_SET if any sampled pin is high, else GPIO_PIN_RESET.
 */
GPIO_PinState EPIC_GPIO_ReadPin(GPIO_TypeDef port, uint16_t pins)
{
    uint8_t mask = (uint8_t)pins;
    /* Read PORTx: pin state for inputs, latched value for outputs. The sim
     * backend models the same; on a real XC8 target this lowers to one
     * MOVF PORTx. */
    return (EPIC_REG8(port_addr(port)) & mask) ? GPIO_PIN_SET : GPIO_PIN_RESET;
}

/**
 * @brief  Write the entire 8-bit port latch (LATx).
 * @param port Port whose latch is written.
 * @param value Byte value to write to the latch.
 */
void EPIC_GPIO_WritePort(GPIO_TypeDef port, uint8_t value)
{
    EPIC_REG8(lat_addr(port)) = value;
}

/**
 * @brief  Read the entire port input register (PORTx).
 * @param port Port to read.
 * @return The current PORTx byte.
 */
uint8_t EPIC_GPIO_ReadPort(GPIO_TypeDef port)
{
    return EPIC_REG8(port_addr(port));
}

/**
 * @brief  Enable or disable the PORTB internal weak pull-ups, mapped to
 *         INTCON2<RBPU> (DS39605F §6.2). RBPU is active-low: 1 disables
 *         the pull-ups, 0 enables them.
 * @param pull GPIO_PULLUP to enable, GPIO_NOPULL to disable.
 */
void EPIC_GPIO_SetPullups(GPIO_PullTypeDef pull)
{
    /* INTCON2<RBPU> (bit 7), active-low: 1 = disabled, 0 = enabled
     * (DS39605F §6.2, Register 9-2). */
    if (pull == GPIO_PULLUP) {
        EPIC_BIT_CLR(EPIC_REG8(PIC_REG_INTCON2), PIC_INTCON2_RBPU);
    } else {
        EPIC_BIT_SET(EPIC_REG8(PIC_REG_INTCON2), PIC_INTCON2_RBPU);
    }
}

/* One callback slot for the whole-port RB<7:4> change interrupt. There is
 * only one PORTB, so there is no handle struct (mirrors Timer2's
 * one-callback-per-handle shape but simpler). NULL = unregistered/no-op. */
static void (*s_rb_change_callback)(uint8_t) = NULL;

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
 *         mandatory (DS39605F §9.0): the mismatch comparator latches the
 *         value at the last CPU read of PORTB, so the read ends the
 *         mismatch condition and re-arms the next change.
 */
void RB_IRQHandler(void)
{
    if (!EPIC_IRQ_GetFlag(PIC18_IRQ_RB)) return;

    /* MUST read PORTB before clearing RBIF (DS39605F §9.0): the mismatch
     * comparator latches the value at the last CPU read of PORTB, so the
     * read is what ends the mismatch condition and re-arms the next one.
     * Clearing first risks a spurious re-interrupt or a missed change. */
    uint8_t portb = EPIC_REG8(PIC_REG_PORTB);
    EPIC_IRQ_ClearFlag(PIC18_IRQ_RB);
    if (s_rb_change_callback) s_rb_change_callback(portb);
}
