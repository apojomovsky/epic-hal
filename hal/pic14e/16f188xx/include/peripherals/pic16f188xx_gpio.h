/**
 * PIC16F188xx GPIO contract. Names and signatures match the other PIC14E
 * families so consumers build unchanged, except EPIC_GPIO_EnableChangeDetect
 * takes a leading port: every pin of PORTA..PORTC is IOC-capable here,
 * while the siblings arm PORTB only. PORTD has no IOC registers and PORTE
 * IOC is RE3 only, so neither arms edges. Port set, pull-up registers,
 * and the IOC layout are family-specific.
 */
#ifndef PIC16F188XX_GPIO_H
#define PIC16F188XX_GPIO_H

#include <stdint.h>
#include "pic16f188xx.h"

typedef enum
{
    GPIOA = 0,
    GPIOB = 1,
    GPIOC = 2,
#if PIC16F188XX_FAMILY_HAS_PORTD
    GPIOD = 3,
#endif
#if PIC16F188XX_FAMILY_HAS_PORTE
    GPIOE = 4,
#endif
} GPIO_TypeDef;

#define GPIO_PIN_0   EPIC_BIT(0)
#define GPIO_PIN_1   EPIC_BIT(1)
#define GPIO_PIN_2   EPIC_BIT(2)
#define GPIO_PIN_3   EPIC_BIT(3)
#define GPIO_PIN_4   EPIC_BIT(4)
#define GPIO_PIN_5   EPIC_BIT(5)
#define GPIO_PIN_6   EPIC_BIT(6)
#define GPIO_PIN_7   EPIC_BIT(7)
#define GPIO_PIN_All 0xFFU

typedef enum
{
    GPIO_PIN_RESET = 0U,
    GPIO_PIN_SET   = 1U
} GPIO_PinState;

typedef enum
{
    GPIO_MODE_INPUT  = 0x1U,
    GPIO_MODE_OUTPUT = 0x2U,
    GPIO_MODE_ANALOG = 0x3U
} GPIO_ModeTypeDef;

/**
 * @brief Set the direction and digital/analog mode of pins on a port.
 * @param port Port to configure.
 * @param pins Pin mask (GPIO_PIN_*), limited to the port's implemented pins.
 * @param mode Input, output (latch cleared), or analog.
 */
void EPIC_GPIO_Init(GPIO_TypeDef port, uint16_t pins, GPIO_ModeTypeDef mode);

/**
 * @brief Reset a port to its power-up direction: all inputs, analog, latch cleared.
 * @param port Port to reset.
 */
void EPIC_GPIO_DeInit(GPIO_TypeDef port);

/**
 * @brief Drive pins high or low through the output latch (read-modify-write).
 * @param port Port to write.
 * @param pins Pin mask (GPIO_PIN_*).
 * @param state Level to drive.
 */
void EPIC_GPIO_WritePin(GPIO_TypeDef port, uint16_t pins, GPIO_PinState state);

/**
 * @brief Invert the output latch of pins on a port.
 * @param port Port to toggle.
 * @param pins Pin mask (GPIO_PIN_*).
 */
void EPIC_GPIO_TogglePin(GPIO_TypeDef port, uint16_t pins);

/**
 * @brief Read the level on pins of a port.
 * @param port Port to read.
 * @param pins Pin mask (GPIO_PIN_*).
 * @return GPIO_PIN_SET if any masked pin is high, GPIO_PIN_RESET otherwise.
 */
GPIO_PinState EPIC_GPIO_ReadPin(GPIO_TypeDef port, uint16_t pins);

/**
 * @brief Write the output latch of a whole port, limited to implemented pins.
 * @param port Port to write.
 * @param value Latch value; bits outside the port's implemented pins are ignored.
 */
void EPIC_GPIO_WritePort(GPIO_TypeDef port, uint8_t value);

/**
 * @brief Read the pin levels of a whole port.
 * @param port Port to read.
 * @return PORTx value masked to the port's implemented pins.
 */
uint8_t EPIC_GPIO_ReadPort(GPIO_TypeDef port);

/**
 * @brief Enable or disable weak pull-ups on pins of a port.
 * @details Every port has its own WPUx register and there is no global
 * enable (the pack ships no OPTION_REG for this part), so the call only
 * touches WPUx.
 * @param port Port (GPIOA, GPIOB, or GPIOC; plus GPIOD/GPIOE on 40-pin parts).
 * @param pins Pin mask (GPIO_PIN_*).
 * @param state GPIO_PIN_SET enables, GPIO_PIN_RESET disables.
 */
void EPIC_GPIO_SetPullups(GPIO_TypeDef port, uint16_t pins, GPIO_PinState state);

/**
 * @brief Register the callback run from the interrupt-on-change handler.
 * @param callback Receives the captured IOCBF mask and the PORTB value read
 * with it. Pass NULL to clear.
 */
void EPIC_GPIO_RegisterChangeCallback(void (*callback)(uint8_t iocbf, uint8_t portb));

/**
 * @brief Select the edges that set the IOC flags on one port.
 * @details Writes that port's IOCxP and IOCxN in full, masked to its implemented pins.
 * @param port Port to arm (GPIOA, GPIOB, or GPIOC; GPIOD/GPIOE arm
 *        nothing, PORTD has no IOC registers and PORTE IOC is RE3 only).
 * @param pos_mask Rising-edge pins (bit n = Pn).
 * @param neg_mask Falling-edge pins (bit n = Pn).
 */
void EPIC_GPIO_EnableChangeDetect(GPIO_TypeDef port, uint8_t pos_mask, uint8_t neg_mask);

/**
 * @brief PORTB interrupt-on-change handler: clears every port's captured IOCxF
 * bits and IOCIF, then calls the registered callback with the PORTB capture.
 * @details Weak, so an application can supply its own. Does nothing unless IOCIF is set.
 */
void IOC_IRQHandler(void) EPIC_WEAK;

#endif
