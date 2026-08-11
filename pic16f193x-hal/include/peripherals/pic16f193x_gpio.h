/**
 * PIC16F193X GPIO driver (DS41364B §6.0/§7.0). Cube-style API: every pin
 * is (GPIOx, GPIO_PIN_n). The Enhanced Mid-range I/O model adds a LATx
 * output latch (writes go to LATx, never RMW the pin level) and an ANSELx
 * analog-select register; ANSEL defaults to 1 (analog) at POR on
 * analog-capable pins, so digital Init clears the bit. PORTA/B/C are on
 * every device; PORTD/E only on 40/44-pin parts. PORTA/B/C/D are 8-bit,
 * PORTE is 4-bit (RE0-RE3).
 */

#ifndef PIC16F193X_GPIO_H
#define PIC16F193X_GPIO_H

#include "pic16f193x.h"
#include "pic16f193x_sfr.h"

/**
 * @brief GPIO port identifier. Matches the Cube convention where
 *        `GPIOx` selects the port (x = A..E).
 *
 * PORTA..PORTC are present on every device. PORTD and PORTE exist only on
 * 40/44-pin parts (DS41364B §6.0).
 */
typedef enum {
    GPIOA = 0,   /**< PORTA, 8 bits (RA0..RA7).       */
    GPIOB = 1,   /**< PORTB, 8 bits (RB0..RB7).       */
    GPIOC = 2,   /**< PORTC, 8 bits (RC0..RC7).       */
#if PIC16F193X_FAMILY_HAS_PORTD
    GPIOD = 3,   /**< PORTD, 8 bits (RD0..RD7).       */
#endif
#if PIC16F193X_FAMILY_HAS_PORTE
    GPIOE = 4,   /**< PORTE, 4 bits (RE0..RE3).       */
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

/** Pin logical state. */
typedef enum {
    GPIO_PIN_RESET = 0U,   /**< Logic low. */
    GPIO_PIN_SET   = 1U    /**< Logic high. */
} GPIO_PinState;

/**
 * @brief Pin direction / operating mode.
 *
 * On this core TRIS sets direction and ANSEL selects analog/digital
 * (DS41364B §6.0):
 *   TRIS = 1  -> pin is input
 *   TRIS = 0  -> pin is output (drives LATx)
 *   ANSEL = 1 -> pin is analog (disables the digital input buffer)
 *   ANSEL = 0 -> pin is digital
 */
typedef enum {
    GPIO_MODE_INPUT  = 0x1U,   /**< TRIS=1, ANSEL=0 (digital input).   */
    GPIO_MODE_OUTPUT = 0x2U,   /**< TRIS=0, ANSEL=0 (digital output). */
    GPIO_MODE_ANALOG = 0x3U,   /**< TRIS=1, ANSEL=1 (analog).          */
} GPIO_ModeTypeDef;

/**
 * @brief  Configure one or more pins of a port to the same mode.
 *
 * @param  port   GPIOA..GPIOE
 * @param  pins   Bitmask of @ref GPIO_PIN_0 .. GPIO_PIN_All
 * @param  mode   One of @ref GPIO_ModeTypeDef
 *
 * @note   For OUTPUT mode the matching LATx bits are cleared so the pin
 *         starts driving low. For ANALOG mode the relevant peripheral
 *         (ADC, comparator) is configured separately by its own driver.
 */
void EPIC_GPIO_Init(GPIO_TypeDef port, uint16_t pins, GPIO_ModeTypeDef mode);

/** Restore all pins of `port` to reset (input, analog, latch clear). */
void EPIC_GPIO_DeInit(GPIO_TypeDef port);

/**
 * @brief  Drive a pin high or low; ORs/ANDs the mask onto the LATx latch
 *         (DS41364B §6.0), never reads back the pin level first.
 */
void EPIC_GPIO_WritePin(GPIO_TypeDef port, uint16_t pins, GPIO_PinState state);

/** Toggle a set of pins (LATx ^= mask). */
void EPIC_GPIO_TogglePin(GPIO_TypeDef port, uint16_t pins);

/**
 * @brief  Read the current level seen on `pins` from PORTx (the actual
 *         pin level, DS41364B §6.0). For outputs this returns the driven
 *         level; for inputs it returns whatever the pin is driven to.
 */
GPIO_PinState EPIC_GPIO_ReadPin(GPIO_TypeDef port, uint16_t pins);

/** Atomically write the entire port LATx latch. */
void EPIC_GPIO_WritePort(GPIO_TypeDef port, uint8_t value);

/** Read the entire port (PORTx, pin level). */
uint8_t EPIC_GPIO_ReadPort(GPIO_TypeDef port);

/**
 * @brief  Enable or disable the PORTB per-pin weak pull-ups via WPUB
 *         (DS41364B §6.0), and the global WPUEN enable in OPTION_REG<7>
 *         (active-low). `pins` selects which PORTB pins get pull-ups.
 *         GPIO_NOPULL disables all (WPUB &= ~pins, WPUEN=1); GPIO_PULLUP
 *         enables the selected pins (WPUB |= pins, WPUEN=0).
 */
void EPIC_GPIO_SetPullups(GPIO_TypeDef port, uint16_t pins, GPIO_PinState state);

/**
 * @brief  Register a callback fired from the PORTB interrupt-on-change
 *         (DS41364B §7.0, INTCON<IOCIF>/<IOCIE>). Edge detection is
 *         per-pin via IOCBP (positive) / IOCBN (negative).
 *
 * @param  callback  function called once per IOC interrupt with the
 *                   IOCBF mask (which pins changed) and the freshly-read
 *                   PORTB byte, or NULL to unregister.
 */
void EPIC_GPIO_RegisterChangeCallback(void (*callback)(uint8_t iocbf, uint8_t portb));

/**
 * @brief  Enable per-pin positive/negative edge detection on PORTB
 *         (IOCBP / IOCBN, DS41364B §7.0). Call @ref EPIC_IRQ_Enable with
 *         @ref PIC16F193X_IRQ_IOC to arm the interrupt itself.
 */
void EPIC_GPIO_EnableChangeDetect(uint8_t pos_mask, uint8_t neg_mask);

/**
 * @brief  Weak PORTB change-interrupt ISR (DS41364B §7.0).
 *
 * @details
 *   Default body reads IOCBF (which pins changed), reads PORTB, clears
 *   IOCBF and IOCIF, then forwards (iocbf, portb) to the registered
 *   callback. Read-before-clear order matches the datasheet's re-arm
 *   requirement.
 */
void IOC_IRQHandler(void) EPIC_WEAK;

#endif /* PIC16F193X_GPIO_H */
