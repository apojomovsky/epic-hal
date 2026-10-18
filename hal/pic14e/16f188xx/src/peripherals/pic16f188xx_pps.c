/**
 * PPS driver, implementation. Every xxxPPS and RxyPPS access is a plain
 * register write; the lock sequence goes through epic_sfr_write8 so the
 * host simulator can model it. Nothing here is ISR-reachable.
 */
#include "peripherals/pic16f188xx_pps.h"
#include "core/pic16f188xx_irq.h"

/* Ports each input can reach (bit n = port n), from the EDC ppsports of the
 * input's virtual pin, identical on all seven parts. T0CKIPPS is a 4-bit
 * field (EDC impl 0xF), which is why it stops at PORTB. Inputs not listed
 * here take any port the part has. */
#define PPS_PORTS_ANY   0x0FU
#define PPS_PORTS_AB    0x03U
#define PPS_PORTS_AC    0x05U
#define PPS_PORTS_BC    0x06U

/* POR pins of the input selects (EDC por): T0CKI RA4, SSP1CLK RC3,
 * SSP1DAT RC4, SSP1SS RA5, RX RC7, CCP1 RC2, CCP2 RC1, CCP3 RB5, CCP4 RB0,
 * CCP5 RA4, T1CKI RC0, T1G RB5, T2IN RC3. */
#define PPS_POR_T0CKI   0x04U
#define PPS_POR_SSP1CLK 0x13U
#define PPS_POR_SSP1DAT 0x14U
#define PPS_POR_SSP1SS  0x05U
#define PPS_POR_RX      0x17U
#define PPS_POR_CCP1    0x12U
#define PPS_POR_CCP2    0x11U
#define PPS_POR_CCP3    0x0DU
#define PPS_POR_CCP4    0x08U
#define PPS_POR_CCP5    0x04U
#define PPS_POR_T1CKI   0x10U
#define PPS_POR_T1G     0x0DU
#define PPS_POR_T2IN    0x13U

/**
 * @brief Check that a port exists on the selected part.
 * @param port Port to check.
 * @return 1 for PORTA..PORTC, and for PORTD on the 40-pin parts, else 0.
 */
static uint8_t port_ok(PPS_PortTypeDef port)
{
    if ((unsigned int)port <= (unsigned int)PPS_PORTC)
    {
        return 1U;
    }
#if PIC16F188XX_FAMILY_HAS_PORTD
    if (port == PPS_PORTD)
    {
        return 1U;
    }
#endif
    return 0U;
}

/**
 * @brief Ports a peripheral input can be routed from.
 * @param input Input to route (validated by the caller).
 * @return Bit mask with bit n set when port n reaches the input.
 */
static uint8_t input_ports(PPS_InputTypeDef input)
{
    switch (input)
    {
    case PPS_INPUT_T0CKI: return PPS_PORTS_AB;
    case PPS_INPUT_T1CKI: return PPS_PORTS_AC;
    case PPS_INPUT_T1G:   return PPS_PORTS_BC;
    case PPS_INPUT_T2IN:  return PPS_PORTS_AC;
    default:              return PPS_PORTS_ANY;
    }
}

/**
 * @brief Input select register for a peripheral input.
 * @param input Input to route (validated by the caller).
 * @return SFR address of its xxxPPS register.
 */
static uint16_t input_addr(PPS_InputTypeDef input)
{
    switch (input)
    {
    case PPS_INPUT_T0CKI:     return PIC_REG_T0CKIPPS;
    case PPS_INPUT_SSP1CLK:   return PIC_REG_SSP1CLKPPS;
    case PPS_INPUT_SSP1DAT:   return PIC_REG_SSP1DATPPS;
    case PPS_INPUT_SSP1SS:    return PIC_REG_SSP1SSPPS;
    case PPS_INPUT_CCP1:      return PIC_REG_CCP1PPS;
    case PPS_INPUT_CCP2:      return PIC_REG_CCP2PPS;
    case PPS_INPUT_CCP3:      return PIC_REG_CCP3PPS;
    case PPS_INPUT_CCP4:      return PIC_REG_CCP4PPS;
    case PPS_INPUT_CCP5:      return PIC_REG_CCP5PPS;
    case PPS_INPUT_T1CKI:     return PIC_REG_T1CKIPPS;
    case PPS_INPUT_T1G:       return PIC_REG_T1GPPS;
    case PPS_INPUT_T2IN:      return PIC_REG_T2AINPPS;
    default:                  return PIC_REG_RXPPS;
    }
}

/**
 * @brief Output select register of a pin.
 * @details RxyPPS registers are consecutive, eight per port, from RA0PPS.
 * @param port Port of the pin (validated by the caller).
 * @param pin Pin number 0..7.
 * @return SFR address of its RxyPPS register.
 */
static uint16_t output_addr(PPS_PortTypeDef port, uint8_t pin)
{
    return (uint16_t)(PIC_REG_RA0PPS + ((unsigned int)port * 8U) + pin);
}

/**
 * @brief Point an input peripheral at a pin.
 * @param input Peripheral input to route.
 * @param port Port of the pin.
 * @param pin Pin number 0..7.
 * @return EPIC_OK on success, EPIC_BUSY while PPSLOCK is set, EPIC_INVALID
 *         for an unknown input, a port the part or the input lacks, or
 *         pin above 7.
 */
EPIC_StatusTypeDef EPIC_PPS_MapInput(PPS_InputTypeDef input,
                                     PPS_PortTypeDef port, uint8_t pin)
{
    if (((unsigned int)input > (unsigned int)PPS_INPUT_T2IN) ||
        (port_ok(port) == 0U) || (pin > 7U) ||
        (((input_ports(input) >> (unsigned int)port) & 1U) == 0U))
    {
        return EPIC_INVALID;
    }
    if (EPIC_PPS_IsLocked() != 0U)
    {
        return EPIC_BUSY;
    }
    EPIC_REG8(input_addr(input)) = (uint8_t)(((unsigned int)port << 3) | pin);
    return EPIC_OK;
}

/**
 * @brief Drive a pin from a peripheral output (or hand it back to the latch).
 * @param port Port of the pin.
 * @param pin Pin number 0..7.
 * @param output Source for the pin.
 * @return EPIC_OK on success, EPIC_BUSY while PPSLOCK is set, EPIC_INVALID
 *         for an unknown output, a port the part lacks, or pin above 7.
 */
EPIC_StatusTypeDef EPIC_PPS_MapOutput(PPS_PortTypeDef port, uint8_t pin,
                                      PPS_OutputTypeDef output)
{
    if ((port_ok(port) == 0U) || (pin > 7U))
    {
        return EPIC_INVALID;
    }
    if (output != PPS_OUTPUT_LAT)
    {
        if ((((unsigned int)output < (unsigned int)PPS_OUTPUT_CCP1) ||
             ((unsigned int)output > (unsigned int)PPS_OUTPUT_PWM7)) &&
            (output != PPS_OUTPUT_EUSART_TX) && (output != PPS_OUTPUT_SSP1_SCK) &&
            (output != PPS_OUTPUT_SSP1_SDO) && (output != PPS_OUTPUT_NCO1))
        {
            return EPIC_INVALID;
        }
    }
    if (EPIC_PPS_IsLocked() != 0U)
    {
        return EPIC_BUSY;
    }
    EPIC_REG8(output_addr(port, pin)) = (uint8_t)output;
    return EPIC_OK;
}

/**
 * @brief Restore every input select to its POR pin and every output to the latch.
 * @return EPIC_OK, or EPIC_BUSY while PPSLOCK is set (nothing is written).
 */
EPIC_StatusTypeDef EPIC_PPS_DeInit(void)
{
    uint8_t port;
    uint8_t pin;

    if (EPIC_PPS_IsLocked() != 0U)
    {
        return EPIC_BUSY;
    }
    EPIC_REG8(PIC_REG_T0CKIPPS) = PPS_POR_T0CKI;
    EPIC_REG8(PIC_REG_SSP1CLKPPS) = PPS_POR_SSP1CLK;
    EPIC_REG8(PIC_REG_SSP1DATPPS) = PPS_POR_SSP1DAT;
    EPIC_REG8(PIC_REG_SSP1SSPPS) = PPS_POR_SSP1SS;
    EPIC_REG8(PIC_REG_RXPPS) = PPS_POR_RX;
    EPIC_REG8(PIC_REG_CCP1PPS) = PPS_POR_CCP1;
    EPIC_REG8(PIC_REG_CCP2PPS) = PPS_POR_CCP2;
    EPIC_REG8(PIC_REG_CCP3PPS) = PPS_POR_CCP3;
    EPIC_REG8(PIC_REG_CCP4PPS) = PPS_POR_CCP4;
    EPIC_REG8(PIC_REG_CCP5PPS) = PPS_POR_CCP5;
    EPIC_REG8(PIC_REG_T1CKIPPS) = PPS_POR_T1CKI;
    EPIC_REG8(PIC_REG_T1GPPS) = PPS_POR_T1G;
    EPIC_REG8(PIC_REG_T2AINPPS) = PPS_POR_T2IN;
    for (port = 0U; port <= (uint8_t)PPS_PORTD; port++)
    {
        if (port_ok((PPS_PortTypeDef)port) == 0U)
        {
            continue;
        }
        for (pin = 0U; pin < 8U; pin++)
        {
            EPIC_REG8(output_addr((PPS_PortTypeDef)port, pin)) = 0U;
        }
    }
    return EPIC_OK;
}

/**
 * @brief Run the 0x55/0xAA unlock sequence and clear PPSLOCKED.
 * @details The sequence is a platform macro: the target form is the
 * datasheet's banksel, two key writes and an explicit bcf, consecutive,
 * with global interrupts masked around it.
 * @return EPIC_OK when PPSLOCKED reads clear afterwards, EPIC_ERROR otherwise.
 */
EPIC_StatusTypeDef EPIC_PPS_Unlock(void)
{
    uint8_t gie = EPIC_IRQ_Disable();

    epic_pps_unlock_sequence();
    EPIC_IRQ_Restore(gie);
    return (EPIC_PPS_IsLocked() == 0U) ? EPIC_OK : EPIC_ERROR;
}

/**
 * @brief Run the 0x55/0xAA sequence and set PPSLOCKED.
 * @details Same sequence as unlock, ending in bsf.
 * @return EPIC_OK when PPSLOCKED reads set afterwards, EPIC_ERROR otherwise.
 */
EPIC_StatusTypeDef EPIC_PPS_Lock(void)
{
    uint8_t gie = EPIC_IRQ_Disable();

    epic_pps_lock_sequence();
    EPIC_IRQ_Restore(gie);
    return (EPIC_PPS_IsLocked() != 0U) ? EPIC_OK : EPIC_ERROR;
}

/**
 * @brief Report whether the PPS registers are locked.
 * @return 1 when PPSLOCKED is set, 0 otherwise.
 */
uint8_t EPIC_PPS_IsLocked(void)
{
    return ((epic_sfr_read8(PIC_REG_PPSLOCK) & PIC_PPSLOCK_PPSLOCKED) != 0U) ? 1U : 0U;
}
