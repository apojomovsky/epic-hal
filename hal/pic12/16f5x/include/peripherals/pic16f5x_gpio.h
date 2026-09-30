/* PIC16F5x GPIO port driver (DS41213D §2.0): Cube-style (GPIOx,
 * GPIO_PIN_n). Port direction lives in the control-space TRIS
 * registers, written via the platform's EPIC_TRIS_WRITE (the `tris`
 * instruction on target, the sim shadow on host); port levels live in
 * the PORTA/PORTB/(PORTC/D/E) file registers. This core has NO PORTB
 * weak pull-ups and NO RB change interrupt (no INTEDG/RBPU bits and no
 * INTCON anywhere on the die), so the pull-up and change-callback APIs
 * of the 14-bit families do not exist here. */

#ifndef PIC16F5X_GPIO_H
#define PIC16F5X_GPIO_H

#include "pic16f5x_hal.h"
#include "pic16f5x_sfr.h"

/**
 * @brief GPIO port identifier. Matches the Cube convention where
 *        `GPIOx` selects the port (x = A..E).
 *
 * PORTA (4 output-capable pins, 16F54/57/59 only) and PORTB (8 pins,
 * every part) are the baseline surface. PORTC on the 28-pin parts
 * (16F57/59/505/506); PORTD/E on the 40-pin 16F59. The per-part
 * capability macros gate the enum members.
 */
typedef enum {
    GPIOA = 0,   /**< PORTA, RA0..RA3 (DS41213D §2.0). */
    GPIOB = 1,   /**< PORTB, RB0..RB7. */
#if PIC16F5X_FAMILY_HAS_PORTC
    GPIOC = 2,   /**< PORTC, RC0..RC7. */
#endif
#if PIC16F5X_FAMILY_HAS_PORTD
    GPIOD = 3,   /**< PORTD, RD0..RD7. */
#endif
#if PIC16F5X_FAMILY_HAS_PORTE
    GPIOE = 4,   /**< PORTE, RE4..RE7 (upper nibble, 16F59 only; DS41213D §6.5). */
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
 * On the baseline core the TRIS bit controls direction identically to
 * classic PIC16: TRIS = 1 is input, TRIS = 0 is output (DS41213D
 * §2.0, Table 2-1).
 */
typedef enum {
    GPIO_MODE_INPUT  = 0x1U,   /**< TRIS bit = 1, high-impedance. */
    GPIO_MODE_OUTPUT = 0x2U,   /**< TRIS bit = 0, drives the latch. */
} GPIO_ModeTypeDef;

/* init / deinit. */

/**
 * @brief  Configure one or more pins of a port to the same direction.
 *
 * @param  port   GPIOA..GPIOE
 * @param  pins   Bitmask of @ref GPIO_PIN_0 .. GPIO_PIN_All
 * @param  mode   One of @ref GPIO_ModeTypeDef
 *
 * @note   The 5x core has no analog pins on the parts this driver
 *         covers (the comparator/ADC on the 16F506 are not in the
 *         GPIO tier), so there is no GPIO_MODE_ANALOG.
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
 *         outputs this returns the latch state; for input pins it
 *         returns whatever the pin is being driven to externally.
 * @param port GPIOA..GPIOE.
 * @param pins Bitmask of pins to sample.
 * @return GPIO_PIN_SET if any selected pin reads high, GPIO_PIN_RESET
 *         otherwise.
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

#endif /* PIC16F5X_GPIO_H */
