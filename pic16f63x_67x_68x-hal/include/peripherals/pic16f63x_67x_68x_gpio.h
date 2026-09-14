/* GPIO port driver, Cube-style (GPIOx, GPIO_PIN_n). Writes OR/AND the
 * PORTx latch and never read-modify the pin level first (DS40001262F
 * §4.0). PORTA is RA0..RA5, PORTB is RB4..RB7 only, PORTC is RC0..RC7
 * (DS40001262F Table 1, 18 I/O). Analog-capable pins are selected
 * per-pin through ANSEL in Bank 2 (RA0/RA1/RC0..RC3) and, on ANSELH
 * parts, ANSELH (RB4/RB5/RC6/RC7); there is no ADCON1<PCFG> on this
 * family. */

#ifndef PIC16F63X_67X_68X_GPIO_H
#define PIC16F63X_67X_68X_GPIO_H

#include "pic16f63x_67x_68x_hal.h"
#include "pic16f63x_67x_68x_sfr.h"

/**
 * @brief GPIO port identifier. Matches the Cube convention where
 *        `GPIOx` selects the port (x = A..C).
 */
typedef enum {
    GPIOA = 0,   /**< PORTA, 6 bits (RA0..RA5), DS40001262F §4.1. */
    GPIOB = 1,   /**< PORTB, 4 bits (RB4..RB7), DS40001262F §4.2. */
    GPIOC = 2,   /**< PORTC, 8 bits (RC0..RC7), DS40001262F §4.3. */
} GPIO_TypeDef;

/**
 * @brief Pin identifiers. Ports A and C span 0..7 (unimplemented bits
 *        are masked off per port); PORTB implements RB4..RB7 only.
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
 * On PIC16 the TRIS bit controls direction:
 *   TRIS = 1  → pin is input
 *   TRIS = 0  → pin is output
 * DS40001262F §4.0: "Setting a TRIS bit = 1 will make the corresponding
 * pin an input (i.e., put the corresponding output driver in a
 * High-Impedance mode). Clearing a TRIS bit = 0 will make the
 * corresponding pin an output (i.e., put the contents of the output
 * latch on the selected pin)."
 */
typedef enum {
    GPIO_MODE_INPUT  = 0x1U,   /**< TRIS bit = 1, high-impedance. */
    GPIO_MODE_OUTPUT = 0x2U,   /**< TRIS bit = 0, drives the latch. */
    GPIO_MODE_ANALOG = 0x3U,   /**< Pin released to an analog peripheral (ANSEL = 1). */
} GPIO_ModeTypeDef;

/**
 * @brief  Internal weak-pull-up control (PORTA/PORTB per-pin WPUA/WPUB
 *         bits gated by OPTION_REG<RABPU>, DS40001262F §4.0).
 */
typedef enum {
    GPIO_NOPULL   = 0U,   /**< Weak pull-ups disabled (RABPU = 1). */
    GPIO_PULLUP   = 1U    /**< Weak pull-ups enabled  (RABPU = 0). */
} GPIO_PullTypeDef;

/* init / deinit. */

/**
 * @brief  Configure one or more pins of a port to the same direction.
 *
 * @param  port   GPIOA..GPIOC
 * @param  pins   Bitmask of @ref GPIO_PIN_0 .. GPIO_PIN_All
 * @param  mode   One of @ref GPIO_ModeTypeDef
 *
 * @note   Does not configure alternate-function peripherals (the
 *         comparators own their input pins once enabled); call the
 *         relevant peripheral driver first. For analog pins, the ANSEL
 *         bit must be set (this driver does it for GPIO_MODE_ANALOG).
 */
void EPIC_GPIO_Init(GPIO_TypeDef port, uint16_t pins, GPIO_ModeTypeDef mode);

/**
 * @brief Restore all pins of `port` to input mode and clear the latch.
 * @param port GPIOA..GPIOC.
 */
void EPIC_GPIO_DeInit(GPIO_TypeDef port);

/* read / write / toggle. */

/**
 * @brief  Drive a pin high or low; ORs/ANDs the mask onto the PORTx
 *         latch directly, never reads back the pin level first.
 * @param port GPIOA..GPIOC.
 * @param pins Bitmask of @ref GPIO_PIN_0 .. GPIO_PIN_All.
 * @param state GPIO_PIN_SET to drive high, GPIO_PIN_RESET for low.
 */
void EPIC_GPIO_WritePin(GPIO_TypeDef port, uint16_t pins, GPIO_PinState state);

/**
 * @brief Toggle a set of pins (latch ^= mask).
 * @param port GPIOA..GPIOC.
 * @param pins Bitmask of pins to invert.
 */
void EPIC_GPIO_TogglePin(GPIO_TypeDef port, uint16_t pins);

/**
 * @brief  Read the current level seen on `pins`. For pins configured as
 *         outputs this returns the latch state; for input pins it returns
 *         whatever the pin is being driven to externally.
 * @param port GPIOA..GPIOC.
 * @param pins Bitmask of pins to sample.
 * @return GPIO_PIN_SET if any selected pin reads high, GPIO_PIN_RESET otherwise.
 */
GPIO_PinState EPIC_GPIO_ReadPin(GPIO_TypeDef port, uint16_t pins);

/**
 * @brief Write the whole port latch.
 * @param port GPIOA..GPIOC.
 * @param value the byte to write (unimplemented bits are masked off).
 */
void EPIC_GPIO_WritePort(GPIO_TypeDef port, uint8_t value);

/**
 * @brief Read the whole port latch.
 * @param port GPIOA..GPIOC.
 * @return the port byte.
 */
uint8_t EPIC_GPIO_ReadPort(GPIO_TypeDef port);

/* PORTA/PORTB pull-ups. */

/**
 * @brief Enable or disable the internal weak pull-ups (global gate).
 * @param pull GPIO_PULLUP (RABPU = 0) or GPIO_NOPULL (RABPU = 1).
 */
void EPIC_GPIO_SetPullups(GPIO_PullTypeDef pull);

/**
 * @brief Enable or disable the weak pull-up on a single PORTB pin.
 * @param pin the RB pin number, 4..7 (RB0..RB3 do not exist).
 * @param enable 1 to enable the pull-up, 0 to disable it.
 */
void EPIC_GPIO_SetPinPullup(uint8_t pin, uint8_t enable);

/* PORTB change interrupt. */

/**
 * @brief Install or remove the PORTB change callback.
 * @param callback function called with the freshly-read PORTB byte on
 *                 an RB<7:4> change, or NULL to unregister.
 *
 * @details
 *   One callback slot (there's only one PORTB); fanning one received
 *   byte out to N consumers is application-level composition, not a
 *   HAL registry. NULL is safe. @ref RB_IRQHandler reads PORTB before
 *   clearing RABIF, see its own doc for why that order is mandatory.
 */
void EPIC_GPIO_RegisterChangeCallback(void (*callback)(uint8_t portb_value));

/**
 * @brief  Enable or disable interrupt-on-change for one PORTB pin.
 * @param pin the RB pin number, 4..7 (RB0..RB3 do not exist).
 * @param enable 1 to enable IOC on the pin, 0 to disable it.
 */
void EPIC_GPIO_SetPinIOC(uint8_t pin, uint8_t enable);

/**
 * @brief  Weak RB<7:4> change-interrupt ISR (DS40001262F §4.0, §14.0).
 *
 * @details
 *   Default body clears RABIF and forwards the already-read PORTB byte
 *   to the registered callback. Read-before-clear is mandatory, not
 *   stylistic: the mismatch comparator only re-arms once PORTB is
 *   read, so reading it after clearing RABIF risks a spurious
 *   re-interrupt or a silently-missed change.
 */
void RB_IRQHandler(void) EPIC_WEAK;

#endif /* PIC16F63X_67X_68X_GPIO_H */
