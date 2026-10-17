#include "peripherals/pic16f188xx_gpio.h"

/* Set by EPIC_GPIO_RegisterChangeCallback, run from IOC_IRQHandler. */
static void (*s_ioc_callback)(uint8_t iocbf, uint8_t portb);

/* ISR flag snapshots. File-scope statics, not cstack autos: XC8 -O2
 * omits bank restores for autos after banked SFR accesses (read in the
 * .s), while every static access gets its own bank select. */
static uint8_t s_iocaf_snapshot;
static uint8_t s_iocbf_snapshot;
static uint8_t s_ioccf_snapshot;
static uint8_t s_portb_snapshot;

/**
 * @brief Implemented-pin mask for a port.
 * @param port GPIO port.
 * @return Pin mask, or 0 for an unknown port (callers then do nothing).
 */
static uint8_t port_mask(GPIO_TypeDef port)
{
    switch (port)
    {
    case GPIOA: return PIC16F188XX_FAMILY_PORTA_MASK;
    case GPIOB: return PIC16F188XX_FAMILY_PORTB_MASK;
    case GPIOC: return PIC16F188XX_FAMILY_PORTC_MASK;
#if PIC16F188XX_FAMILY_HAS_PORTD
    case GPIOD: return PIC16F188XX_FAMILY_PORTD_MASK;
#endif
#if PIC16F188XX_FAMILY_HAS_PORTE
    case GPIOE: return PIC16F188XX_FAMILY_PORTE_MASK;
#endif
    default:    return 0U;
    }
}

/**
 * @brief TRIS register address of a port.
 * @param port GPIO port (validated by the caller via port_mask).
 * @return SFR address.
 */
static uint16_t tris_addr(GPIO_TypeDef port)
{
    switch (port)
    {
    case GPIOA: return PIC_REG_TRISA;
    case GPIOB: return PIC_REG_TRISB;
#if PIC16F188XX_FAMILY_HAS_PORTD
    case GPIOD: return PIC_REG_TRISD;
#endif
#if PIC16F188XX_FAMILY_HAS_PORTE
    case GPIOE: return PIC_REG_TRISE;
#endif
    default:    return PIC_REG_TRISC;
    }
}

/**
 * @brief LAT register address of a port.
 * @param port GPIO port (validated by the caller via port_mask).
 * @return SFR address.
 */
static uint16_t lat_addr(GPIO_TypeDef port)
{
    switch (port)
    {
    case GPIOA: return PIC_REG_LATA;
    case GPIOB: return PIC_REG_LATB;
#if PIC16F188XX_FAMILY_HAS_PORTD
    case GPIOD: return PIC_REG_LATD;
#endif
#if PIC16F188XX_FAMILY_HAS_PORTE
    case GPIOE: return PIC_REG_LATE;
#endif
    default:    return PIC_REG_LATC;
    }
}

/**
 * @brief PORT register address of a port.
 * @param port GPIO port (validated by the caller via port_mask).
 * @return SFR address.
 */
static uint16_t port_addr(GPIO_TypeDef port)
{
    switch (port)
    {
    case GPIOA: return PIC_REG_PORTA;
    case GPIOB: return PIC_REG_PORTB;
#if PIC16F188XX_FAMILY_HAS_PORTD
    case GPIOD: return PIC_REG_PORTD;
#endif
#if PIC16F188XX_FAMILY_HAS_PORTE
    case GPIOE: return PIC_REG_PORTE;
#endif
    default:    return PIC_REG_PORTC;
    }
}

/**
 * @brief ANSEL register address of a port.
 * @param port GPIO port (validated by the caller via port_mask).
 * @return SFR address.
 */
static uint16_t ansel_addr(GPIO_TypeDef port)
{
    switch (port)
    {
    case GPIOA: return PIC_REG_ANSELA;
    case GPIOB: return PIC_REG_ANSELB;
#if PIC16F188XX_FAMILY_HAS_PORTD
    case GPIOD: return PIC_REG_ANSELD;
#endif
#if PIC16F188XX_FAMILY_HAS_PORTE
    case GPIOE: return PIC_REG_ANSELE;
#endif
    default:    return PIC_REG_ANSELC;
    }
}

/**
 * @brief Weak pull-up register address of a port.
 * @param port GPIO port (validated by the caller via port_mask).
 * @return SFR address.
 */
static uint16_t wpu_addr(GPIO_TypeDef port)
{
    switch (port)
    {
    case GPIOA: return PIC_REG_WPUA;
    case GPIOB: return PIC_REG_WPUB;
#if PIC16F188XX_FAMILY_HAS_PORTD
    case GPIOD: return PIC_REG_WPUD;
#endif
#if PIC16F188XX_FAMILY_HAS_PORTE
    case GPIOE: return PIC_REG_WPUE;
#endif
    default:    return PIC_REG_WPUC;
    }
}

/**
 * @brief Rising-edge (IOCxP) register address of a port.
 * @param port GPIO port (validated by the caller via port_mask).
 * @return SFR address.
 */
static uint16_t ioc_pos_addr(GPIO_TypeDef port)
{
    switch (port)
    {
    case GPIOA: return PIC_REG_IOCAP;
    case GPIOB: return PIC_REG_IOCBP;
    default:    return PIC_REG_IOCCP;
    }
}

/**
 * @brief Falling-edge (IOCxN) register address of a port.
 * @param port GPIO port (validated by the caller via port_mask).
 * @return SFR address.
 */
static uint16_t ioc_neg_addr(GPIO_TypeDef port)
{
    switch (port)
    {
    case GPIOA: return PIC_REG_IOCAN;
    case GPIOB: return PIC_REG_IOCBN;
    default:    return PIC_REG_IOCCN;
    }
}

/**
 * @brief Set the direction and digital or analog mode of pins on a port.
 * @param port GPIO port.
 * @param pins Pin mask (GPIO_PIN_*), limited to the port's implemented pins.
 * @param mode GPIO_MODE_INPUT, GPIO_MODE_OUTPUT (latch cleared first), or
 *        GPIO_MODE_ANALOG.
 */
void EPIC_GPIO_Init(GPIO_TypeDef port, uint16_t pins, GPIO_ModeTypeDef mode)
{
    uint8_t mask = (uint8_t)(pins & port_mask(port));
    if (mask == 0U)
    {
        return;
    }

    switch (mode)
    {
    case GPIO_MODE_INPUT:
        EPIC_REG8(tris_addr(port)) |= mask;
        EPIC_REG8(ansel_addr(port)) &= (uint8_t)~mask;
        break;
    case GPIO_MODE_OUTPUT:
        EPIC_REG8(tris_addr(port)) &= (uint8_t)~mask;
        EPIC_REG8(ansel_addr(port)) &= (uint8_t)~mask;
        EPIC_REG8(lat_addr(port)) &= (uint8_t)~mask;
        break;
    case GPIO_MODE_ANALOG:
        EPIC_REG8(tris_addr(port)) |= mask;
        EPIC_REG8(ansel_addr(port)) |= mask;
        break;
    default:
        break;
    }
}

/**
 * @brief Restore the implemented pins of a port to reset: input, analog,
 *        latch low. ANSEL powers up all-analog on this part (EDC por).
 * @param port GPIO port.
 */
void EPIC_GPIO_DeInit(GPIO_TypeDef port)
{
    uint8_t mask = port_mask(port);
    if (mask == 0U)
    {
        return;
    }

    EPIC_REG8(tris_addr(port)) = mask;
    EPIC_REG8(ansel_addr(port)) = mask;
    EPIC_REG8(lat_addr(port)) = 0U;
}

/**
 * @brief Drive pins high or low by setting or clearing their LATx bits.
 * @param port GPIO port.
 * @param pins Pin mask (GPIO_PIN_*).
 * @param state GPIO_PIN_SET drives high, GPIO_PIN_RESET drives low.
 */
void EPIC_GPIO_WritePin(GPIO_TypeDef port, uint16_t pins, GPIO_PinState state)
{
    uint8_t mask = (uint8_t)(pins & port_mask(port));
    if (mask == 0U)
    {
        return;
    }

    if (state == GPIO_PIN_SET)
    {
        EPIC_REG8(lat_addr(port)) |= mask;
    }
    else
    {
        EPIC_REG8(lat_addr(port)) &= (uint8_t)~mask;
    }
}

/**
 * @brief Toggle the LATx latch bits of the given pins.
 * @param port GPIO port.
 * @param pins Pin mask (GPIO_PIN_*).
 */
void EPIC_GPIO_TogglePin(GPIO_TypeDef port, uint16_t pins)
{
    uint8_t mask = (uint8_t)(pins & port_mask(port));
    if (mask == 0U)
    {
        return;
    }

    EPIC_REG8(lat_addr(port)) ^= mask;
}

/**
 * @brief Read the level of the given pins from PORTx.
 * @param port GPIO port.
 * @param pins Pin mask (GPIO_PIN_*).
 * @return GPIO_PIN_SET if any masked pin reads high, else GPIO_PIN_RESET.
 */
GPIO_PinState EPIC_GPIO_ReadPin(GPIO_TypeDef port, uint16_t pins)
{
    uint8_t mask = (uint8_t)(pins & port_mask(port));
    if (mask == 0U)
    {
        return GPIO_PIN_RESET;
    }

    return ((EPIC_REG8(port_addr(port)) & mask) != 0U) ? GPIO_PIN_SET : GPIO_PIN_RESET;
}

/**
 * @brief Write the implemented bits of a port's LATx latch in one access.
 * @param port GPIO port.
 * @param value Byte to write. Bits outside the port's implemented mask
 *        are ignored and the unimplemented LAT bits are kept.
 */
void EPIC_GPIO_WritePort(GPIO_TypeDef port, uint8_t value)
{
    uint8_t mask = port_mask(port);
    if (mask == 0U)
    {
        return;
    }

    EPIC_REG8(lat_addr(port)) = (uint8_t)((EPIC_REG8(lat_addr(port)) & (uint8_t)~mask) |
                                          (value & mask));
}

/**
 * @brief Read the pin levels of a port, masked to its implemented pins.
 * @param port GPIO port.
 * @return PORTx register value masked to the port's implemented pins.
 */
uint8_t EPIC_GPIO_ReadPort(GPIO_TypeDef port)
{
    uint8_t mask = port_mask(port);
    if (mask == 0U)
    {
        return 0U;
    }

    return (uint8_t)(EPIC_REG8(port_addr(port)) & mask);
}

/**
 * @brief Enable or disable the weak pull-up on pins of a port.
 * @details Only the port's own WPUx register is touched: this part has no
 *          global pull-up enable.
 * @param port GPIO port.
 * @param pins Pin mask (GPIO_PIN_*).
 * @param state GPIO_PIN_SET enables the pull-ups, GPIO_PIN_RESET disables.
 */
void EPIC_GPIO_SetPullups(GPIO_TypeDef port, uint16_t pins, GPIO_PinState state)
{
    uint8_t mask = (uint8_t)(pins & port_mask(port));
    if (mask == 0U)
    {
        return;
    }

    if (state == GPIO_PIN_SET)
    {
        EPIC_REG8(wpu_addr(port)) |= mask;
    }
    else
    {
        EPIC_REG8(wpu_addr(port)) &= (uint8_t)~mask;
    }
}

/**
 * @brief Register the callback run from IOC_IRQHandler, or NULL to
 *        unregister it.
 * @param callback Called with the IOCBF mask and the PORTB byte read in
 *        the handler.
 */
void EPIC_GPIO_RegisterChangeCallback(void (*callback)(uint8_t iocbf, uint8_t portb))
{
    s_ioc_callback = callback;
}

/**
 * @brief Edge-arm mask for a port. PORTD has no IOC registers and PORTE
 *        IOC is RE3 only, which has no driver, so both arm nothing.
 * @param port GPIO port.
 * @return IOC mask, or 0 for a port with no drivable edge detection.
 */
static uint8_t ioc_mask(GPIO_TypeDef port)
{
    switch (port)
    {
    case GPIOA: return PIC16F188XX_FAMILY_IOCA_MASK;
    case GPIOB: return PIC16F188XX_FAMILY_IOCB_MASK;
    case GPIOC: return PIC16F188XX_FAMILY_IOCC_MASK;
    default:    return 0U;
    }
}

/**
 * @brief Set the rising and falling edge masks for one port's interrupt-on-change.
 * @param port Port to arm (GPIOA, GPIOB, or GPIOC; GPIOD/GPIOE arm nothing).
 * @param pos_mask Pins that trigger on a rising edge (IOCxP).
 * @param neg_mask Pins that trigger on a falling edge (IOCxN).
 */
void EPIC_GPIO_EnableChangeDetect(GPIO_TypeDef port, uint8_t pos_mask, uint8_t neg_mask)
{
    uint8_t mask = ioc_mask(port);
    if (mask == 0U)
    {
        return;
    }

    EPIC_REG8(ioc_pos_addr(port)) = (uint8_t)(pos_mask & mask);
    EPIC_REG8(ioc_neg_addr(port)) = (uint8_t)(neg_mask & mask);
}

/* Capture and clear one port's flag register a bit at a time into a
 * file-scope snapshot. Each access names its register literally, so it
 * stays a compile-time-constant token (a runtime address here misdirects
 * the access on banked cores). A masked write of the whole flag byte can
 * overwrite a flag hardware sets in its load/store window. The snapshot
 * is file-scope, not a cstack auto: XC8 -O2 omits bank restores for
 * autos after banked SFR accesses, while every static access gets its
 * own bank select. */
#define IOC_CAPTURE_BITS(flagreg, snap)                                  \
    do {                                                                 \
        snap = EPIC_REG8(flagreg);                                       \
        if ((snap & 0x01U) != 0U) { EPIC_BIT_CLR(EPIC_REG8(flagreg), 0x01U); } \
        if ((snap & 0x02U) != 0U) { EPIC_BIT_CLR(EPIC_REG8(flagreg), 0x02U); } \
        if ((snap & 0x04U) != 0U) { EPIC_BIT_CLR(EPIC_REG8(flagreg), 0x04U); } \
        if ((snap & 0x08U) != 0U) { EPIC_BIT_CLR(EPIC_REG8(flagreg), 0x08U); } \
        if ((snap & 0x10U) != 0U) { EPIC_BIT_CLR(EPIC_REG8(flagreg), 0x10U); } \
        if ((snap & 0x20U) != 0U) { EPIC_BIT_CLR(EPIC_REG8(flagreg), 0x20U); } \
        if ((snap & 0x40U) != 0U) { EPIC_BIT_CLR(EPIC_REG8(flagreg), 0x40U); } \
        if ((snap & 0x80U) != 0U) { EPIC_BIT_CLR(EPIC_REG8(flagreg), 0x80U); } \
    } while (0)

/**
 * @brief Interrupt-on-change ISR: capture and clear every port's IOCxF
 *        flags, then forward the PORTB capture to the callback.
 * @details Returns unless PIR0.IOCIF is set. IOCAF and IOCCF are cleared
 *          but not reported, matching the sibling families' PORTB-only
 *          contract. The handler keeps no cstack autos (see the snapshot
 *          note above).
 */
void IOC_IRQHandler(void)
{
    if ((EPIC_REG8(PIC_REG_PIR0) & PIC_PIR0_IOCIF) == 0U)
    {
        return;
    }

    IOC_CAPTURE_BITS(PIC_REG_IOCAF, s_iocaf_snapshot);
    IOC_CAPTURE_BITS(PIC_REG_IOCBF, s_iocbf_snapshot);
    IOC_CAPTURE_BITS(PIC_REG_IOCCF, s_ioccf_snapshot);
    s_portb_snapshot = EPIC_REG8(PIC_REG_PORTB);

    /* The sim models IOCIF as RAM, so this clear is required there. */
    EPIC_BIT_CLR(EPIC_REG8(PIC_REG_PIR0), PIC_PIR0_IOCIF);

    if (s_ioc_callback != 0)
    {
        s_ioc_callback(s_iocbf_snapshot, s_portb_snapshot);
    }
}
