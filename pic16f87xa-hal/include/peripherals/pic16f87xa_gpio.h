/**
 * @file    peripherals/pic16f87xa_gpio.h
 * @brief   General-Purpose I/O port driver for the PIC16F87XA family.
 *
 * @details
 *   Cube-style API: every pin is (GPIOx, GPIO_PIN_n). Writes only
 *   OR/AND a mask onto the PORTx latch, never read-modify the pin level
 *   first (DS39582B §4.x). PORTA is 6-bit, PORTB/C/D are 8-bit, PORTE
 *   is 3-bit (40/44-pin only); pin names match the datasheet pinout
 *   tables across every part in the family.
 */

#ifndef PIC16F87XA_GPIO_H
#define PIC16F87XA_GPIO_H

#include "pic16f87xa.h"
#include "pic16f87xa_sfr.h"

/**
 * @brief GPIO port identifier. Matches the Cube convention where
 *        `GPIOx` selects the port (x = A..E).
 *
 * PORTA..PORTC are present on every device. PORTD and PORTE exist only on
 * 40/44-pin parts (PIC16F874A / 877A, DS39582B §4.4, §4.5).
 */
typedef enum {
    GPIOA = 0,   /**< PORTA, 6 bits (RA0..RA5), DS39582B §4.1, Table 4-1. */
    GPIOB = 1,   /**< PORTB, 8 bits (RB0..RB7), DS39582B §4.2, Table 4-3. */
    GPIOC = 2,   /**< PORTC, 8 bits (RC0..RC7), DS39582B §4.3, Table 4-5. */
#if PIC16F87XA_FAMILY_HAS_PORTD
    GPIOD = 3,   /**< PORTD, 8 bits (RD0..RD7), DS39582B §4.4, Table 4-7. */
#endif
#if PIC16F87XA_FAMILY_HAS_PORTE
    GPIOE = 4,   /**< PORTE, 3 bits (RE0..RE2), DS39582B §4.5, Table 4-9. */
#endif
} GPIO_TypeDef;

/**
 * @brief Pin identifiers. Each port has up to 8 pins.
 *        Use @ref GPIO_PIN_All for whole-port operations.
 */
#define GPIO_PIN_0    PIC8_BIT(0)
#define GPIO_PIN_1    PIC8_BIT(1)
#define GPIO_PIN_2    PIC8_BIT(2)
#define GPIO_PIN_3    PIC8_BIT(3)
#define GPIO_PIN_4    PIC8_BIT(4)
#define GPIO_PIN_5    PIC8_BIT(5)
#define GPIO_PIN_6    PIC8_BIT(6)
#define GPIO_PIN_7    PIC8_BIT(7)
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
 * DS39582B §4.x: "Setting a TRIS bit = 1 will make the corresponding pin
 * an input (i.e., put the corresponding output driver in a High-Impedance
 * mode). Clearing a TRIS bit = 0 will make the corresponding pin an
 * output (i.e., put the contents of the output latch on the selected
 * pin)."
 */
typedef enum {
    GPIO_MODE_INPUT  = 0x1U,   /**< TRIS bit = 1, high-impedance. */
    GPIO_MODE_OUTPUT = 0x2U,   /**< TRIS bit = 0, drives the latch. */
    GPIO_MODE_ANALOG = 0x3U,   /**< Pin released to an analog peripheral. */
} GPIO_ModeTypeDef;

/**
 * @brief  Internal weak-pull-up control (PORTB only, DS39582B §4.2,
 *         RBPU bit in OPTION_REG<7>).
 */
typedef enum {
    GPIO_NOPULL   = 0U,   /**< Weak pull-ups disabled (RBPU = 1). */
    GPIO_PULLUP   = 1U    /**< Weak pull-ups enabled  (RBPU = 0). */
} GPIO_PullTypeDef;

/* ───────────────────────── init / deinit ────────────────────────── */

/**
 * @brief  Configure one or more pins of a port to the same direction.
 *
 * @param  port   GPIOA..GPIOE
 * @param  pins   Bitmask of @ref GPIO_PIN_0 .. GPIO_PIN_All
 * @param  mode   One of @ref GPIO_ModeTypeDef
 *
 * @note   Does not configure alternate-function peripherals (e.g. ADC,
 *         USART), call the relevant peripheral driver first. Specifically
 *         for PORTA analog pins, set ADCON1<PCFG3:PCFG0> before configuring
 *         the pin as analog (DS39582B §4.1, §11.x).
 */
void HAL_GPIO_Init(GPIO_TypeDef port, uint16_t pins, GPIO_ModeTypeDef mode);

/** Restore all pins of `port` to input mode and clear the latch. */
void HAL_GPIO_DeInit(GPIO_TypeDef port);

/* ───────────────────────── read / write / toggle ────────────────── */

/**
 * @brief  Drive a pin high or low; ORs/ANDs the mask onto the PORTx
 *         latch directly, never reads back the pin level first.
 */
void HAL_GPIO_WritePin(GPIO_TypeDef port, uint16_t pins, GPIO_PinState state);

/** Toggle a set of pins (latch ^= mask). */
void HAL_GPIO_TogglePin(GPIO_TypeDef port, uint16_t pins);

/**
 * @brief  Read the current level seen on `pins`. For pins configured as
 *         outputs this returns the latch state; for input pins it returns
 *         whatever the pin is being driven to externally.
 */
GPIO_PinState HAL_GPIO_ReadPin(GPIO_TypeDef port, uint16_t pins);

/** Atomically write the entire 8-bit port latch. */
void HAL_GPIO_WritePort(GPIO_TypeDef port, uint8_t value);

/** Read the entire port latch. */
uint8_t HAL_GPIO_ReadPort(GPIO_TypeDef port);

/* ───────────────────────── PORTB pull-ups ───────────────────────── */

/**
 * @brief  Enable or disable PORTB internal weak pull-ups.
 *         Maps to OPTION_REG<RBPU> (DS39582B §4.2, §14 Register 14-1).
 *
 * @note   OPTION_REG<7> is inverted: RBPU = 1 disables pull-ups.
 */
void HAL_GPIO_SetPullups(GPIO_PullTypeDef pull);

/* ───────────────────────── PORTB change interrupt ─────────────────── */

/**
 * @brief  Register a single whole-port callback fired from the RB<7:4>
 *         change interrupt (DS39582B §14.11.3, INTCON<RBIF>/<RBIE>).
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
void HAL_GPIO_RegisterChangeCallback(void (*callback)(uint8_t portb_value));

/**
 * @brief  Weak RB<7:4> change-interrupt ISR (DS39582B §14.11.3).
 *
 * @details
 *   Default body clears RBIF and forwards the already-read PORTB byte
 *   to the registered callback. Read-before-clear is mandatory, not
 *   stylistic: the mismatch comparator only re-arms once PORTB is
 *   read, so reading it after clearing RBIF risks a spurious
 *   re-interrupt or a silently-missed change.
 */
void RB_IRQHandler(void) PIC8_WEAK;

#endif /* PIC16F87XA_GPIO_H */
