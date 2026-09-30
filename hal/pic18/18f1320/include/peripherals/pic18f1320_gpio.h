/*
 * GPIO driver for the PIC18F1320 (DS39605F §5.0/§6.0): Cube-style
 * `(GPIOx, GPIO_PIN_n)` API, writes through LATx, reads PORTx. Only
 * PORTA and PORTB exist, both full 8-bit; no PORTC/D/E (confirmed
 * absent from the DFP header, unlike the sibling family's 4550/2455).
 */

#ifndef PIC18F1320_GPIO_H
#define PIC18F1320_GPIO_H

#include "pic18f1320.h"
#include "pic18f1320_sfr.h"

/**
 * @brief GPIO port identifier. Matches the Cube convention where
 *        `GPIOx` selects the port (x = A, B).
 *
 * PIC18F1320 has only PORTA and PORTB (18-pin package, DS39605F §5.0).
 */
typedef enum {
    GPIOA = 0,   /**< PORTA, 8 bits (RA0..RA7), DS39605F §5.0. */
    GPIOB = 1,   /**< PORTB, 8 bits (RB0..RB7), DS39605F §6.0. */
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
 * On PIC18 the TRIS bit controls direction (DS39605F §5.0/§6.0):
 *   TRIS = 1  → pin is input (driver high-impedance)
 *   TRIS = 0  → pin is output (drives the LATx value)
 */
typedef enum {
    GPIO_MODE_INPUT  = 0x1U,   /**< TRIS bit = 1, high-impedance. */
    GPIO_MODE_OUTPUT = 0x2U,   /**< TRIS bit = 0, drives LATx.      */
    GPIO_MODE_ANALOG = 0x3U,   /**< Pin released to an analog peripheral. */
} GPIO_ModeTypeDef;

/**
 * @brief  Internal weak-pull-up control (PORTB only, DS39605F §6.2,
 *         RBPU bit in INTCON2<7>).
 */
typedef enum {
    GPIO_NOPULL   = 0U,   /**< Weak pull-ups disabled (RBPU = 1). */
    GPIO_PULLUP   = 1U    /**< Weak pull-ups enabled  (RBPU = 0). */
} GPIO_PullTypeDef;

/**
 * @brief  Configure one or more pins of a port to the same direction.
 *
 * @param  port   GPIOA or GPIOB
 * @param  pins   Bitmask of @ref GPIO_PIN_0 .. GPIO_PIN_All
 * @param  mode   One of @ref GPIO_ModeTypeDef
 *
 * @note   Does not configure alternate-function peripherals (e.g. ADC,
 *         USART); call the relevant peripheral driver first.
 */
void EPIC_GPIO_Init(GPIO_TypeDef port, uint16_t pins, GPIO_ModeTypeDef mode);

/**
 * @brief Restore all pins of `port` to input mode and clear the latch.
 * @param port the port to deinitialize (GPIOA or GPIOB).
 */
void EPIC_GPIO_DeInit(GPIO_TypeDef port);

/**
 * @brief  Drive a pin high or low. Writes the LATx latch directly
 *         (DS39605F §5.0/§6.0), the PIC18-native way (no read-modify-write
 *         of PORTx).
 * @param port the port containing the pins.
 * @param pins bitmask of pins to drive (@ref GPIO_PIN_0 .. GPIO_PIN_All).
 * @param state the level to drive (GPIO_PIN_RESET or GPIO_PIN_SET).
 */
void EPIC_GPIO_WritePin(GPIO_TypeDef port, uint16_t pins, GPIO_PinState state);

/**
 * @brief Toggle a set of pins (LATx ^= mask).
 * @param port the port containing the pins.
 * @param pins bitmask of pins to toggle (@ref GPIO_PIN_0 .. GPIO_PIN_All).
 */
void EPIC_GPIO_TogglePin(GPIO_TypeDef port, uint16_t pins);

/**
 * @brief  Read the current level seen on `pins` from PORTx. For pins
 *         configured as outputs this returns the latched value; for input
 *         pins it returns whatever the pin is being driven to externally.
 * @param port the port to read.
 * @param pins bitmask of pins to sample (@ref GPIO_PIN_0 .. GPIO_PIN_All).
 * @return GPIO_PIN_SET if any sampled pin reads high, else GPIO_PIN_RESET.
 */
GPIO_PinState EPIC_GPIO_ReadPin(GPIO_TypeDef port, uint16_t pins);

/**
 * @brief Atomically write the entire 8-bit port latch (LATx).
 * @param port the port to write.
 * @param value the 8-bit value to latch.
 */
void EPIC_GPIO_WritePort(GPIO_TypeDef port, uint8_t value);

/**
 * @brief Read the entire port (PORTx).
 * @param port the port to read.
 * @return the current 8-bit PORTx value.
 */
uint8_t EPIC_GPIO_ReadPort(GPIO_TypeDef port);

/**
 * @brief  Enable or disable PORTB internal weak pull-ups.
 *         Maps to INTCON2<RBPU> (DS39605F §6.2).
 *
 * @note   INTCON2<7> is inverted: RBPU = 1 disables pull-ups.
 * @param pull GPIO_PULLUP to enable the pull-ups, GPIO_NOPULL to disable.
 */
void EPIC_GPIO_SetPullups(GPIO_PullTypeDef pull);

/**
 * @brief  Register a single whole-port callback fired from the RB<7:4>
 *         change interrupt (DS39605F §9.0/§6.2, INTCON<RBIF>/<RBIE>).
 *
 * @param  callback  function called once per RB-change interrupt with the
 *                   freshly-read PORTB byte, or NULL to unregister.
 *
 * One callback slot (only one PORTB exists). The handler must read PORTB
 * *before* clearing RBIF (DS39605F §9.0): reading PORTB is what re-arms
 * the mismatch comparator; clearing first risks a missed or spurious
 * interrupt. See @ref RB_IRQHandler.
 */
void EPIC_GPIO_RegisterChangeCallback(void (*callback)(uint8_t portb_value));

/**
 * @brief  Weak RB<7:4> change-interrupt ISR (DS39605F §9.0/§6.2).
 *
 * Default body reads PORTB into a local, clears RBIF, then forwards that
 * value to the callback from @ref EPIC_GPIO_RegisterChangeCallback. The
 * read-before-clear order is mandatory (datasheet "read PORTB to end the
 * mismatch condition"), not stylistic.
 */
void RB_IRQHandler(void) EPIC_WEAK;

#endif /* PIC18F1320_GPIO_H */
