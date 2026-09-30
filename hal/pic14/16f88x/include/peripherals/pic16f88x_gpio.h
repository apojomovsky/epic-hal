/* GPIO port driver, Cube-style (GPIOx, GPIO_PIN_n). Writes OR/AND the
 * PORTx latch and never read-modify the pin level first (DS40001291H
 * §3.x). PORTA..PORTC 8-bit on all devices; PORTD/PORTE RE<2:0>
 * 40/44-pin only. Analog pins are selected per-pin through ANSEL/
 * ANSELH (not ADCON1<PCFG> like the 87XA). */

#ifndef PIC16F88X_GPIO_H
#define PIC16F88X_GPIO_H

#include "pic16f88x.h"
#include "pic16f88x_sfr.h"

/**
 * @brief GPIO port identifier. Matches the Cube convention where
 *        `GPIOx` selects the port (x = A..E).
 *
 * PORTA..PORTC are present on every device. PORTD and PORTE (RE<2:0>)
 * exist only on 40/44-pin parts (PIC16F884 / 887, DS40001291H §3.6,
 * §3.7).
 */
typedef enum {
    GPIOA = 0,   /**< PORTA, 8 bits (RA0..RA7), DS40001291H §3.1. */
    GPIOB = 1,   /**< PORTB, 8 bits (RB0..RB7), DS40001291H §3.4. */
    GPIOC = 2,   /**< PORTC, 8 bits (RC0..RC7), DS40001291H §3.5. */
#if PIC16F88X_FAMILY_HAS_PORTD
    GPIOD = 3,   /**< PORTD, 8 bits (RD0..RD7), DS40001291H §3.6. */
#endif
#if PIC16F88X_FAMILY_HAS_PORTE
    GPIOE = 4,   /**< PORTE, 3 bits (RE0..RE2), DS40001291H §3.7. */
#endif
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
 * On PIC16 the TRIS bit controls direction:
 *   TRIS = 1  → pin is input
 *   TRIS = 0  → pin is output
 * DS40001291H §3.x: "Setting a TRIS bit = 1 will make the corresponding
 * pin an input (i.e., put the corresponding output driver in a
 * High-Impedance mode). Clearing a TRIS bit = 0 will make the
 * corresponding pin an output (i.e., put the contents of the output
 * latch on the selected pin)."
 */
typedef enum {
    GPIO_MODE_INPUT  = 0x1U,   /**< TRIS bit = 1, high-impedance. */
    GPIO_MODE_OUTPUT = 0x2U,   /**< TRIS bit = 0, drives the latch. */
    GPIO_MODE_ANALOG = 0x3U,   /**< Pin released to an analog peripheral (ANSEL/ANSELH = 1). */
} GPIO_ModeTypeDef;

/**
 * @brief  Internal weak-pull-up control (PORTB only, DS40001291H §3.4.2:
 *         per-pin WPUB bits gated by OPTION_REG<RBPU>).
 */
typedef enum {
    GPIO_NOPULL   = 0U,   /**< Weak pull-ups disabled (RBPU = 1). */
    GPIO_PULLUP   = 1U    /**< Weak pull-ups enabled  (RBPU = 0). */
} GPIO_PullTypeDef;

/* init / deinit. */

/**
 * @brief  Configure one or more pins of a port to the same direction.
 *
 * @param  port   GPIOA..GPIOE
 * @param  pins   Bitmask of @ref GPIO_PIN_0 .. GPIO_PIN_All
 * @param  mode   One of @ref GPIO_ModeTypeDef
 *
 * @note   Does not configure alternate-function peripherals (e.g. ADC,
 *         EUSART), call the relevant peripheral driver first. For
 *         analog pins, the ANSEL/ANSELH bit must be set (this driver
 *         does it for GPIO_MODE_ANALOG; DS40001291H §3.1 note).
 */
void EPIC_GPIO_Init(GPIO_TypeDef port, uint16_t pins, GPIO_ModeTypeDef mode);

/**
 * @brief Restore all pins of `port` to input mode and clear the latch.
 * @param port GPIOA..GPIOE.
 */
void EPIC_GPIO_DeInit(GPIO_TypeDef port);

/* read / write / toggle. */

/**
 * @brief  Drive a pin high or low; ORs/ANDs the mask onto the PORTx
 *         latch directly, never reads back the pin level first.
 * @param port GPIOA..GPIOE.
 * @param pins Bitmask of @ref GPIO_PIN_0 .. GPIO_PIN_All.
 * @param state GPIO_PIN_SET to drive high, GPIO_PIN_RESET for low.
 */
void EPIC_GPIO_WritePin(GPIO_TypeDef port, uint16_t pins, GPIO_PinState state);

/**
 * @brief Toggle a set of pins (latch ^= mask).
 * @param port GPIOA..GPIOE.
 * @param pins Bitmask of pins to invert.
 */
void EPIC_GPIO_TogglePin(GPIO_TypeDef port, uint16_t pins);

/**
 * @brief  Read the current level seen on `pins`. For pins configured as
 *         outputs this returns the latch state; for input pins it returns
 *         whatever the pin is being driven to externally.
 * @param port GPIOA..GPIOE.
 * @param pins Bitmask of pins to sample.
 * @return GPIO_PIN_SET if any selected pin reads high, GPIO_PIN_RESET otherwise.
 */
GPIO_PinState EPIC_GPIO_ReadPin(GPIO_TypeDef port, uint16_t pins);

/**
 * @brief Atomically write the entire 8-bit port latch.
 * @param port GPIOA..GPIOE.
 * @param value the byte to write to the port latch.
 */
void EPIC_GPIO_WritePort(GPIO_TypeDef port, uint8_t value);

/**
 * @brief Read the entire port latch.
 * @param port GPIOA..GPIOE.
 * @return the current 8-bit port latch value.
 */
uint8_t EPIC_GPIO_ReadPort(GPIO_TypeDef port);

/* PORTB pull-ups (per-pin WPUB). */

/**
 * @brief  Enable or disable the per-pin PORTB weak pull-ups.
 *         Maps to the WPUB register bits plus the global RBPU gate
 *         (OPTION_REG<7>, inverted, DS40001291H §3.4.2 Register 3-7).
 *
 * @note   RBPU = 1 disables all pull-ups; WPUB<n> = 1 enables the
 *         individual pull-up on RB<n> (only when RBPU = 0).
 * @param pull GPIO_PULLUP to enable (RBPU = 0), GPIO_NOPULL to disable.
 */
void EPIC_GPIO_SetPullups(GPIO_PullTypeDef pull);

/**
 * @brief  Enable or disable the weak pull-up on a single PORTB pin.
 * @param pin the RB pin number, 0..7.
 * @param enable 1 to enable the pull-up, 0 to disable it.
 */
void EPIC_GPIO_SetPinPullup(uint8_t pin, uint8_t enable);

/* PORTB interrupt-on-change (per-pin IOCB). */

/**
 * @brief  Register a single whole-port callback fired from the PORTB
 *         change interrupt (DS40001291H §3.4.3, INTCON<RBIF>/<RBIE>,
 *         IOCB<7:0>). Unlike the 87XA's RB<7:4>-only change, every
 *         PORTB pin is individually enabled through IOCB.
 *
 * @param  callback  function called once per RB-change interrupt with the
 *                   freshly-read PORTB byte, or NULL to unregister.
 *
 * @details
 *   One callback slot (there's only one PORTB); fanning one received
 *   byte out to N consumers is application-level composition, not a
 *   HAL registry. NULL is safe. @ref RB_IRQHandler reads PORTB before
 *   clearing RBIF, see its own doc for why that order is mandatory.
 */
void EPIC_GPIO_RegisterChangeCallback(void (*callback)(uint8_t portb_value));

/**
 * @brief  Enable or disable interrupt-on-change for one PORTB pin.
 * @param pin the RB pin number, 0..7.
 * @param enable 1 to enable IOC on the pin, 0 to disable it.
 */
void EPIC_GPIO_SetPinIOC(uint8_t pin, uint8_t enable);

/**
 * @brief  Weak RB<7:0> change-interrupt ISR (DS40001291H §3.4.3).
 *
 * @details
 *   Default body clears RBIF and forwards the already-read PORTB byte
 *   to the registered callback. Read-before-clear is mandatory, not
 *   stylistic: the mismatch comparator only re-arms once PORTB is
 *   read, so reading it after clearing RBIF risks a spurious
 *   re-interrupt or a silently-missed change.
 */
void RB_IRQHandler(void) EPIC_WEAK;

#endif /* PIC16F88X_GPIO_H */
