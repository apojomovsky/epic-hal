/**
 * PIC16F188xx peripheral pin select (PPS). Input peripherals read their
 * pin through an xxxPPS register holding (port << 3) | pin; output pins
 * pick a source through their RxyPPS register (0 = the port latch, i.e.
 * plain GPIO). Facts come from the pinned DFP EDC (register addresses,
 * POR values, the virtual-pin ppsval table); see MANUAL.md. PPS does not
 * touch TRIS or ANSEL: configure the pin with EPIC_GPIO_Init as well.
 */
#ifndef PIC16F188XX_PPS_H
#define PIC16F188XX_PPS_H

#include "pic16f188xx.h"

/** @brief Port a pin belongs to; the value is the xxxPPS port field (bits 4:3). */
typedef enum {
    PPS_PORTA = 0x0U,
    PPS_PORTB = 0x1U,
    PPS_PORTC = 0x2U,
    PPS_PORTD = 0x3U,   /**< 40-pin parts only. */
} PPS_PortTypeDef;

/**
 * @brief Remappable inputs this HAL drives. T0CKI picks from PORTA and
 *        PORTB only (T0CKIPPS is a 4-bit field); the others reach PORTA
 *        to PORTC, plus PORTD on the 40-pin parts.
 */
typedef enum {
    PPS_INPUT_T0CKI      = 0U,   /**< Timer0 external clock (T0CKIPPS). */
    PPS_INPUT_SSP1CLK    = 1U,   /**< MSSP1 SCK/SCL input (SSP1CLKPPS). */
    PPS_INPUT_SSP1DAT    = 2U,   /**< MSSP1 SDI/SDA input (SSP1DATPPS). */
    PPS_INPUT_SSP1SS     = 3U,   /**< MSSP1 slave-select input (SSP1SSPPS). */
    PPS_INPUT_EUSART_RX  = 4U,   /**< EUSART RX input (RXPPS). */
    PPS_INPUT_CCP1       = 5U,   /**< CCP1 capture input (CCP1PPS). */
    PPS_INPUT_CCP2       = 6U,   /**< CCP2 capture input (CCP2PPS). */
    PPS_INPUT_CCP3       = 7U,   /**< CCP3 capture input (CCP3PPS). */
    PPS_INPUT_CCP4       = 8U,   /**< CCP4 capture input (CCP4PPS). */
    PPS_INPUT_CCP5       = 9U,   /**< CCP5 capture input (CCP5PPS). */
} PPS_InputTypeDef;

/**
 * @brief Output sources for RxyPPS (EDC virtual-pin ppsval). In I2C mode
 *        SCK1 drives SCL and SDO1 drives SDA.
 */
typedef enum {
    PPS_OUTPUT_LAT        = 0x00U,   /**< Port latch: the pin is plain GPIO. */
    PPS_OUTPUT_CCP1       = 0x09U,   /**< CCP1 output (compare or PWM). */
    PPS_OUTPUT_CCP2       = 0x0AU,   /**< CCP2 output. */
    PPS_OUTPUT_CCP3       = 0x0BU,   /**< CCP3 output. */
    PPS_OUTPUT_CCP4       = 0x0CU,   /**< CCP4 output. */
    PPS_OUTPUT_CCP5       = 0x0DU,   /**< CCP5 output. */
    PPS_OUTPUT_PWM6       = 0x0EU,   /**< PWM6 output. */
    PPS_OUTPUT_PWM7       = 0x0FU,   /**< PWM7 output. */
    PPS_OUTPUT_EUSART_TX  = 0x10U,   /**< EUSART TX. */
    PPS_OUTPUT_C1OUT      = 0x12U,   /**< Comparator C1 output (Table 13-3). */
    PPS_OUTPUT_C2OUT      = 0x13U,   /**< Comparator C2 output (Table 13-3). */
    PPS_OUTPUT_SSP1_SCK   = 0x14U,   /**< MSSP1 SCK (SPI) or SCL (I2C). */
    PPS_OUTPUT_SSP1_SDO   = 0x15U,   /**< MSSP1 SDO (SPI) or SDA (I2C). */
    PPS_OUTPUT_NCO1       = 0x19U,   /**< NCO1 output: PORTA plus PORTC (18854/55/56),
                                       PORTB (18857) or PORTD (18875/76/77). */
} PPS_OutputTypeDef;

/**
 * @brief Point an input peripheral at a pin.
 * @details Writes the input's xxxPPS register. The pin must still be set
 * up as a digital input with EPIC_GPIO_Init.
 * @param input Peripheral input to route.
 * @param port Port of the pin.
 * @param pin Pin number 0..7.
 * @return EPIC_OK on success, EPIC_BUSY while PPSLOCK is set, EPIC_INVALID
 *         for an unknown input, a port the part or the input lacks, or
 *         pin above 7.
 */
EPIC_StatusTypeDef EPIC_PPS_MapInput(PPS_InputTypeDef input,
                                     PPS_PortTypeDef port, uint8_t pin);

/**
 * @brief Drive a pin from a peripheral output (or hand it back to the latch).
 * @details Writes the pin's RxyPPS register. The pin must still be cleared
 * in TRIS, and in ANSEL, with EPIC_GPIO_Init for the peripheral to drive it.
 * @param port Port of the pin.
 * @param pin Pin number 0..7.
 * @param output Source for the pin.
 * @return EPIC_OK on success, EPIC_BUSY while PPSLOCK is set, EPIC_INVALID
 *         for an unknown output, a port the part lacks, or pin above 7.
 */
EPIC_StatusTypeDef EPIC_PPS_MapOutput(PPS_PortTypeDef port, uint8_t pin,
                                      PPS_OutputTypeDef output);

/**
 * @brief Restore every xxxPPS input select to its POR pin and every RxyPPS
 *        output to the port latch.
 * @return EPIC_OK, or EPIC_BUSY while PPSLOCK is set (nothing is written).
 */
EPIC_StatusTypeDef EPIC_PPS_DeInit(void);

/**
 * @brief Run the 0x55/0xAA unlock sequence and clear PPSLOCKED.
 * @details Interrupts are masked for the sequence and restored after. With
 * the PPS1WAY configuration bit set (the XC8 default), silicon honours one
 * unlock and one lock per reset; later attempts leave PPSLOCKED set.
 * @return EPIC_OK when PPSLOCKED reads clear afterwards, EPIC_ERROR when
 *         the hardware refused the unlock.
 */
EPIC_StatusTypeDef EPIC_PPS_Unlock(void);

/**
 * @brief Run the 0x55/0xAA sequence and set PPSLOCKED, freezing the mapping.
 * @details Interrupts are masked for the sequence and restored after.
 * @return EPIC_OK when PPSLOCKED reads set afterwards, EPIC_ERROR otherwise.
 */
EPIC_StatusTypeDef EPIC_PPS_Lock(void);

/**
 * @brief Report whether the PPS registers are locked (PPSLOCK<PPSLOCKED>).
 * @return 1 when locked, 0 otherwise.
 */
uint8_t EPIC_PPS_IsLocked(void);

#endif
