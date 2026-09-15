/*
 * GPIO driver (DS39631E §10.0): Cube-style (GPIOx, GPIO_PIN_n) API.
 * Writes go through LATx (no PIC16 read-modify-write PORTx hazard),
 * reads come from PORTx. 28-pin part: PORTA/B/C only; RE3/MCLR input
 * has no LAT/TRIS, so no GPIOE. PORTB pull-ups are INTCON2<RBPU>.
 */

#ifndef PIC18F2520_GPIO_H
#define PIC18F2520_GPIO_H

#include "pic18f2520_hal.h"
#include "pic18f2520_sfr.h"

/**
 * @brief GPIO port identifier. Matches the Cube convention where
 *        `GPIOx` selects the port (x = A..C).
 *
 * PORTA/B/C are the only GPIO ports on the 28-pin PIC18F2520
 * (DS39631E Table 1-1); PORTD does not exist and PORTE is a single
 * RE3/MCLR input with no LAT/TRIS registers, so neither is listed here.
 */
typedef enum {
    GPIOA = 0,   /**< PORTA, 8 bits (RA0..RA7), DS39631E §10.0. */
    GPIOB = 1,   /**< PORTB, 8 bits (RB0..RB7), DS39631E §10.0. */
    GPIOC = 2,   /**< PORTC, 8 bits (RC0..RC7), DS39631E §10.0. */
} GPIO_TypeDef;

/**
 * @brief Pin identifiers. Each port has up to 8 pins.
 *        Use @ref GPIO_PIN_All for whole-port operations.
 */
#define GPIO_PIN_0    EPIC_BIT(0)
#define GPIO_PIN_1    EPIC_BIT(1)
#define GPIO_PIN_2    EPIC_BIT(2)
#define GPIO_PIN_3    EPIC_BIT(3)
#define GPIO_PIN_4    EPIC_BIT(4)
#define GPIO_PIN_5    EPIC_BIT(5)
#define GPIO_PIN_6    EPIC_BIT(6)
#define GPIO_PIN_7    EPIC_BIT(7)
#define GPIO_PIN_All  0xFFU

/**
 * @brief Pin logical state.
 */
typedef enum {
    GPIO_PIN_RESET = 0U,   /**< Logic low. */
    GPIO_PIN_SET   = 1U    /**< Logic high. */
} GPIO_PinState;

/**
 * @brief Pin direction / operating mode.
 *
 * On PIC18 the TRIS bit controls direction (DS39631E §10.0):
 *   TRIS = 1  → pin is input (driver high-impedance)
 *   TRIS = 0  → pin is output (drives the LATx value)
 */
typedef enum {
    GPIO_MODE_INPUT  = 0x1U,   /**< TRIS bit = 1, high-impedance. */
    GPIO_MODE_OUTPUT = 0x2U,   /**< TRIS bit = 0, drives LATx.      */
    GPIO_MODE_ANALOG = 0x3U,   /**< Pin released to an analog peripheral. */
} GPIO_ModeTypeDef;

/**
 * @brief  Internal weak-pull-up control (PORTB only, DS39631E §10.2).
 */
typedef enum {
    GPIO_NOPULL = 0U,   /**< Pull-ups disabled. */
    GPIO_PULLUP = 1U,   /**< Pull-ups enabled.  */
} GPIO_PullTypeDef;

/**
 * @brief  Initialize a set of pins on a port to a given direction.
 *
 *         Configures the pins as outputs (clear TRISx) or inputs (set
 *         TRISx). Input pins are left as digital inputs (an analog
 *         peripheral would be configured separately). Writes go through
 *         the TRISx register (DS39631E §10.0).
 * @param port port to configure (a @ref GPIO_TypeDef).
 * @param pins bitmask of the pins to configure (GPIO_PIN_*).
 * @param mode a @ref GPIO_ModeTypeDef (input, output, or analog).
 */
void EPIC_GPIO_Init(GPIO_TypeDef port, uint16_t pins, GPIO_ModeTypeDef mode);

/**
 * @brief  Reset a set of pins to their default state (all inputs).
 * @param port port to reset (a @ref GPIO_TypeDef).
 * @param pins bitmask of the pins to reset (GPIO_PIN_*).
 */
void EPIC_GPIO_DeInit(GPIO_TypeDef port, uint16_t pins);

/**
 * @brief  Write all pins of a port from a bitmask (writes LATx).
 * @param port port to write (a @ref GPIO_TypeDef).
 * @param value bitmask of the pin levels to drive (1 = high).
 */
void EPIC_GPIO_WritePort(GPIO_TypeDef port, uint16_t value);

/**
 * @brief  Write one or more pins on a port to a level (writes LATx).
 * @param port port to write (a @ref GPIO_TypeDef).
 * @param pins bitmask of the pins to change (GPIO_PIN_*).
 * @param state @ref GPIO_PIN_SET or @ref GPIO_PIN_RESET.
 */
void EPIC_GPIO_WritePin(GPIO_TypeDef port, uint16_t pins, GPIO_PinState state);

/**
 * @brief  Toggle one or more pins on a port (XOR into LATx).
 * @param port port whose pins to toggle (a @ref GPIO_TypeDef).
 * @param pins bitmask of the pins to toggle (GPIO_PIN_*).
 */
void EPIC_GPIO_TogglePin(GPIO_TypeDef port, uint16_t pins);

/**
 * @brief  Read one pin's logical level from PORTx.
 * @param port port to read (a @ref GPIO_TypeDef).
 * @param pins single pin to read (a GPIO_PIN_* value).
 * @return @ref GPIO_PIN_SET if the pin reads high, @ref GPIO_PIN_RESET if low.
 */
GPIO_PinState EPIC_GPIO_ReadPin(GPIO_TypeDef port, uint16_t pins);

/**
 * @brief  Read the whole port byte from PORTx.
 * @param port port to read (a @ref GPIO_TypeDef).
 * @return the raw PORTx byte.
 */
uint8_t EPIC_GPIO_ReadPort(GPIO_TypeDef port);

/**
 * @brief  Enable or disable the PORTB weak pull-ups (DS39631E §10.2,
 *         INTCON2<RBPU>, active-low).
 * @param pull @ref GPIO_PULLUP to enable, @ref GPIO_NOPULL to disable.
 */
void EPIC_GPIO_SetPullups(GPIO_PullTypeDef pull);

/**
 * @brief  Register a single whole-port callback fired from the RB<7:4>
 *         change interrupt (DS39631E §9.0/§10.2, INTCON<RBIF>/<RBIE>).
 *
 * @param  callback  function called once per RB-change interrupt with the
 *                   freshly-read PORTB byte, or NULL to unregister.
 *
 * One callback slot (only one PORTB exists). The handler must read PORTB
 * *before* clearing RBIF (DS39631E §9.0): reading PORTB is what re-arms
 * the mismatch comparator; clearing first risks a missed or spurious
 * interrupt. See @ref RB_IRQHandler.
 */
void EPIC_GPIO_RegisterChangeCallback(void (*callback)(uint8_t portb_value));

/**
 * @brief  Weak RB<7:4> change-interrupt ISR (DS39631E §9.0/§10.2).
 *
 * Default body reads PORTB into a local, clears RBIF, then forwards that
 * value to the callback from @ref EPIC_GPIO_RegisterChangeCallback. The
 * read-before-clear order is mandatory (datasheet "read PORTB to end the
 * mismatch condition"), not stylistic.
 */
void RB_IRQHandler(void) EPIC_WEAK;

#endif /* PIC18F2520_GPIO_H */
