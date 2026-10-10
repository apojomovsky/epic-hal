/**
 * PIC16F1508/1509 GPIO contract. Names and signatures match the other PIC14E
 * families so consumers build unchanged. Port set, pull-up registers, and the
 * IOC layout are family-specific.
 */
#ifndef PIC16F1508_GPIO_H
#define PIC16F1508_GPIO_H

#include <stdint.h>
#include "pic16f1508.h"

typedef enum
{
    GPIOA = 0,
    GPIOB = 1,
    GPIOC = 2
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
 * @brief Enable or disable weak pull-ups on pins of PORTA or PORTB.
 * @details PORTC has no weak pull-up register, so the call is a no-op there.
 * The global pull-up enable (OPTION_REG nWPUEN, active low) is cleared on the
 * first set and set again when both WPUA and WPUB are empty.
 * @param port Port (GPIOA or GPIOB).
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
 * @brief Select the RB4..RB7 edges that set IOCBF.
 * @details Writes IOCBP and IOCBN in full, masked to RB4..RB7.
 * @param pos_mask Rising-edge pins (bit n = RBn).
 * @param neg_mask Falling-edge pins (bit n = RBn).
 */
void EPIC_GPIO_EnableChangeDetect(uint8_t pos_mask, uint8_t neg_mask);

/**
 * @brief PORTB interrupt-on-change handler: clears the captured IOCBF bits and
 * IOCIF, then calls the registered callback.
 * @details Weak, so an application can supply its own. Does nothing unless IOCIF is set.
 */
void IOC_IRQHandler(void) EPIC_WEAK;

#endif
