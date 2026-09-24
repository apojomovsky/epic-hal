/*
 * EUSART driver (DS39609B §18.0, DS39661 §18.0 for the ECAN quads):
 * two modules, one on the quads (single EUSART at the EUSART1 addresses,
 * 0xFAB-0xFAF). Every SFR access branches on the instance first, so each
 * branch stays a literal `PIC_REG_*` token (the §4 rule, same shape as
 * the CCP/mssp selectors). Programs the SFRs only; the mdb uart gate
 * captures real TX bytes.
 */

#include "peripherals/pic18f6520_usart.h"
#include "core/pic18_irq.h"

/**
 * @brief  Compute the SPBRG reload value for a target baud rate:
 *         SPBRG = (Fosc / (divisor * baud)) - 1, integer-truncated, with
 *         the divisor taken from DS39609B Table 18-1 (sync = 4, async
 *         depends on BRGH). This part's BRG is 8-bit only (no BRG16 /
 *         SPBRGH).
 * @param fosc_hz System oscillator frequency in Hz.
 * @param baud Desired baud rate in bits per second.
 * @param mode USART_MODE_SYNCHRONOUS or USART_MODE_ASYNCHRONOUS.
 * @param brgh High-baud-rate select (USART_BRGH_HIGH or USART_BRGH_LOW).
 * @return The SPBRG value, or 0xFFFF if `baud` is 0 or the ratio exceeds
 *         the 8-bit register width.
 */
uint16_t USART_ComputeSPBRG(uint32_t fosc_hz, uint32_t baud,
                            USART_ModeTypeDef mode,
                            USART_BaudRateHighTypeDef brgh)
{
    if (baud == 0) return 0xFFFFU;

    /* DS39609B Table 18-1 (8-bit SPBRGx only on this part). */
    uint32_t divisor;
    if (mode == USART_MODE_SYNCHRONOUS)
    {
        divisor = 4U;
    }
    else
    {
        divisor = (brgh == USART_BRGH_HIGH) ? 16U : 64U;
    }

    /* SPBRG = (Fosc / (divisor x baud)) - 1, integer-truncated. */
    uint32_t x = (fosc_hz / (divisor * baud)) - 1U;
    if (x > 255U) return 0xFFFFU;
    return (uint16_t)x;
}

/* Per-instance register access. Each macro branches on `inst` before
 * touching any SFR; see the file header. */
#define USART_WRITE_RCSTA(inst, v)                                       \
    do {                                                                 \
        switch (inst)                                                    \
        {                                                                \
            case USART_INSTANCE_1: EPIC_REG8(PIC_REG_RCSTA1) = (uint8_t)(v); break; \
            case USART_INSTANCE_2: EPIC_REG8(PIC_REG_RCSTA2) = (uint8_t)(v); break; \
            default: break;                                              \
        }                                                                \
    } while (0)
#define USART_WRITE_TXSTA(inst, v)                                       \
    do {                                                                 \
        switch (inst)                                                    \
        {                                                                \
            case USART_INSTANCE_1: EPIC_REG8(PIC_REG_TXSTA1) = (uint8_t)(v); break; \
            case USART_INSTANCE_2: EPIC_REG8(PIC_REG_TXSTA2) = (uint8_t)(v); break; \
            default: break;                                              \
        }                                                                \
    } while (0)
#define USART_WRITE_TXREG(inst, v)                                       \
    do {                                                                 \
        switch (inst)                                                    \
        {                                                                \
            case USART_INSTANCE_1: EPIC_REG8(PIC_REG_TXREG1) = (uint8_t)(v); break; \
            case USART_INSTANCE_2: EPIC_REG8(PIC_REG_TXREG2) = (uint8_t)(v); break; \
            default: break;                                              \
        }                                                                \
    } while (0)
#define USART_WRITE_SPBRG(inst, v)                                       \
    do {                                                                 \
        switch (inst)                                                    \
        {                                                                \
            case USART_INSTANCE_1: EPIC_REG8(PIC_REG_SPBRG1) = (uint8_t)(v); break; \
            case USART_INSTANCE_2: EPIC_REG8(PIC_REG_SPBRG2) = (uint8_t)(v); break; \
            default: break;                                              \
        }                                                                \
    } while (0)
#define USART_READ_RCSTA(inst, out)                                      \
    do {                                                                 \
        switch (inst)                                                    \
        {                                                                \
            case USART_INSTANCE_1: (out) = EPIC_REG8(PIC_REG_RCSTA1); break; \
            case USART_INSTANCE_2: (out) = EPIC_REG8(PIC_REG_RCSTA2); break; \
            default: (out) = 0U; break;                                  \
        }                                                                \
    } while (0)
#define USART_READ_TXSTA(inst, out)                                      \
    do {                                                                 \
        switch (inst)                                                    \
        {                                                                \
            case USART_INSTANCE_1: (out) = EPIC_REG8(PIC_REG_TXSTA1); break; \
            case USART_INSTANCE_2: (out) = EPIC_REG8(PIC_REG_TXSTA2); break; \
            default: (out) = 0U; break;                                  \
        }                                                                \
    } while (0)
#define USART_READ_RCREG(inst, out)                                      \
    do {                                                                 \
        switch (inst)                                                    \
        {                                                                \
            case USART_INSTANCE_1: (out) = EPIC_REG8(PIC_REG_RCREG1); break; \
            case USART_INSTANCE_2: (out) = EPIC_REG8(PIC_REG_RCREG2); break; \
            default: (out) = 0U; break;                                  \
        }                                                                \
    } while (0)

/**
 * @brief  Return 1 if `inst` is a valid EUSART instance, else 0.
 *         ECAN quads carry only the single EUSART (INSTANCE_1).
 * @param inst the instance to validate.
 * @return 1 if valid, else 0.
 */
static uint8_t usart_valid(USART_InstanceTypeDef inst)
{
#if PIC18F6520_FAMILY_HAS_CAN
    return (inst == USART_INSTANCE_1) ? 1U : 0U;
#else
    return (inst == USART_INSTANCE_1 || inst == USART_INSTANCE_2) ? 1U : 0U;
#endif
}

/**
 * @brief  Map an EUSART instance to its TX interrupt ID.
 * @param inst EUSART instance.
 * @return PIC18_IRQ_USART1_TX or PIC18_IRQ_USART2_TX.
 */
static PIC18_IRQn usart_tx_irq(USART_InstanceTypeDef inst)
{
    return (inst == USART_INSTANCE_1) ? PIC18_IRQ_USART1_TX
                                      : PIC18_IRQ_USART2_TX;
}

/**
 * @brief  Map an EUSART instance to its RX interrupt ID.
 * @param inst EUSART instance.
 * @return PIC18_IRQ_USART1_RX or PIC18_IRQ_USART2_RX.
 */
static PIC18_IRQn usart_rx_irq(USART_InstanceTypeDef inst)
{
    return (inst == USART_INSTANCE_1) ? PIC18_IRQ_USART1_RX
                                      : PIC18_IRQ_USART2_RX;
}

/* Per-instance handle storage. COPIES the caller's handle (dangling-
 * pointer rationale, see Timer1). The weak ISRs read from these. */
static USART_HandleTypeDef        g_usart_storage[3];
static const USART_HandleTypeDef *g_usart_handles[3] = { NULL, NULL, NULL };

/**
 * @brief  Initialize an EUSART module from a handle. Programs SPBRGx,
 *         TXSTAx (sync, CSRC, BRGH, TX9, TXEN) and RCSTAx (SPEN, RX9,
 *         ADDEN, CREN), then arms the TX/RX interrupts according to the
 *         registered callbacks.
 * @param h Handle describing the USART configuration.
 * @return EPIC_OK on success, EPIC_INVALID on bad handle/instance.
 */
EPIC_StatusTypeDef EPIC_USART_Init(const USART_HandleTypeDef *h)
{
    if (!h) return EPIC_INVALID;
    if (!usart_valid(h->Instance)) return EPIC_INVALID;
    g_usart_storage[h->Instance] = *h;
    g_usart_handles[h->Instance] = &g_usart_storage[h->Instance];

    /* BRG: 8-bit SPBRGx only (no SPBRGH on this part). */
    USART_WRITE_SPBRG(h->Instance, h->SPBRG);

    /* Build TXSTAx (Register 18-1): CSRC b7, TX9 b6, TXEN b5, SYNC b4,
     * BRGH b2, TRMT b1 (RO), TX9D b0. Reset 0x02 (TRMT=1). */
    uint8_t txsta = PIC_TXSTA_POR_VALUE;     /* 0x02, keep TRMT. */
    if (h->Mode == USART_MODE_SYNCHRONOUS) txsta |= PIC_TXSTA_SYNC;
    if (h->Mode == USART_MODE_SYNCHRONOUS &&
        h->ClockSource == USART_CLOCK_MASTER) txsta |= PIC_TXSTA_CSRC;
    if (h->BaudHigh   == USART_BRGH_HIGH)  txsta |= PIC_TXSTA_BRGH;
    if (h->DataWidth  == USART_DATA_9BITS) txsta |= PIC_TXSTA_TX9;
    /* TXEN always: polled Transmit needs the transmitter even with no
     * TX callback (TXIE still gates the interrupt, set below). */
    txsta |= PIC_TXSTA_TXEN;
    USART_WRITE_TXSTA(h->Instance, txsta);

    /* Build RCSTAx (Register 18-2).
     *   SPEN  bit 7, enable serial port
     *   RX9   bit 6, 9-bit RX
     *   CREN  bit 4, continuous receive enable
     *   ADDEN bit 3, address detect (9-bit)
     * Reset value: 0x00. */
    uint8_t rcsta = PIC_RCSTA_SPEN;
    if (h->DataWidth    == USART_DATA_9BITS) rcsta |= PIC_RCSTA_RX9;
    if (h->AddressDetect)                     rcsta |= PIC_RCSTA_ADDEN;
    /* CREN always: polled Receive needs it even with no RX callback
     * (RCIE still gates the interrupt, set below). */
    rcsta |= PIC_RCSTA_CREN;
    USART_WRITE_RCSTA(h->Instance, rcsta);

    /* TXIF is initially 1 (TXREG empty after reset, §18.2.1); RCIF is 0. */
    EPIC_IRQ_ClearFlag(usart_rx_irq(h->Instance));

    if (h->TxCpltCallback) EPIC_IRQ_Enable(usart_tx_irq(h->Instance));
    else                   EPIC_IRQ_DisableSrc(usart_tx_irq(h->Instance));
    if (h->RxCpltCallback) EPIC_IRQ_Enable(usart_rx_irq(h->Instance));
    else                   EPIC_IRQ_DisableSrc(usart_rx_irq(h->Instance));

    return EPIC_OK;
}

/**
 * @brief  De-initialize an EUSART module: disable the TX/RX interrupts,
 *         clear their flags, restore RCSTAx/TXSTAx/SPBRGx to their
 *         power-on values and drop the stored handle.
 * @param inst which EUSART module to de-initialize.
 * @return EPIC_OK on success, EPIC_INVALID for an unknown instance.
 */
EPIC_StatusTypeDef EPIC_USART_DeInit(USART_InstanceTypeDef inst)
{
    if (!usart_valid(inst)) return EPIC_INVALID;
    EPIC_IRQ_DisableSrc(usart_tx_irq(inst));
    EPIC_IRQ_DisableSrc(usart_rx_irq(inst));
    EPIC_IRQ_ClearFlag(usart_tx_irq(inst));
    EPIC_IRQ_ClearFlag(usart_rx_irq(inst));
    USART_WRITE_RCSTA(inst, PIC_RCSTA_POR_VALUE);    /* 0x00 */
    USART_WRITE_TXSTA(inst, PIC_TXSTA_POR_VALUE);    /* 0x02, keep TRMT */
    USART_WRITE_SPBRG(inst, PIC_SPBRG_POR_VALUE);
    g_usart_handles[inst] = NULL;
    return EPIC_OK;
}

/**
 * @brief  Transmit one byte: writing TXREGx clears TXIF and starts the
 *         TSR-to-line shift.
 * @param inst which EUSART module to transmit on.
 * @param data Byte to transmit.
 */
void EPIC_USART_Transmit(USART_InstanceTypeDef inst, uint8_t data)
{
    /* Writing TXREGx clears TXIF (DS39609B §18.2.1). */
    USART_WRITE_TXREG(inst, data);
    EPIC_IRQ_ClearFlag(usart_tx_irq(inst));
}

/**
 * @brief  Return the 9th transmit data bit (TXSTAx<TX9D>).
 * @param inst which EUSART module to query.
 * @return 1 if the TX9D bit is set, else 0.
 */
uint8_t EPIC_USART_GetTX9D(USART_InstanceTypeDef inst)
{
    uint8_t txsta = 0U;
    USART_READ_TXSTA(inst, txsta);
    return (txsta & PIC_TXSTA_TX9D) ? 1U : 0U;
}

/**
 * @brief  Set the 9th transmit data bit (TXSTAx<TX9D>).
 * @param inst which EUSART module to configure.
 * @param bit9 Value to write: nonzero sets, zero clears.
 */
void EPIC_USART_SetTX9D(USART_InstanceTypeDef inst, uint8_t bit9)
{
    uint8_t txsta = 0U;
    USART_READ_TXSTA(inst, txsta);
    if (bit9) txsta |= PIC_TXSTA_TX9D;
    else      txsta &= (uint8_t)~PIC_TXSTA_TX9D;
    USART_WRITE_TXSTA(inst, txsta);
}

/**
 * @brief  Return 1 if the transmit shift register is empty (TXSTAx<TRMT>).
 * @param inst which EUSART module to query.
 * @return 1 if TRMT is set, else 0.
 */
uint8_t EPIC_USART_IsTxShiftRegisterEmpty(USART_InstanceTypeDef inst)
{
    uint8_t txsta = 0U;
    USART_READ_TXSTA(inst, txsta);
    return (txsta & PIC_TXSTA_TRMT) ? 1U : 0U;
}

/**
 * @brief  Receive one byte: reading RCREGx clears RCIF (DS39609B §18.2.2).
 * @param inst which EUSART module to read.
 * @return The received byte.
 */
uint8_t EPIC_USART_Receive(USART_InstanceTypeDef inst)
{
    uint8_t data = 0U;
    USART_READ_RCREG(inst, data);
    EPIC_IRQ_ClearFlag(usart_rx_irq(inst));
    return data;
}

/**
 * @brief  Return the 9th received data bit (RCSTAx<RX9D>).
 * @param inst which EUSART module to query.
 * @return 1 if the RX9D bit is set, else 0.
 */
uint8_t EPIC_USART_GetRX9D(USART_InstanceTypeDef inst)
{
    uint8_t rcsta = 0U;
    USART_READ_RCSTA(inst, rcsta);
    return (rcsta & PIC_RCSTA_RX9D) ? 1U : 0U;
}

/**
 * @brief  Return 1 if a receive overrun has occurred (RCSTAx<OERR>).
 * @param inst which EUSART module to query.
 * @return 1 if OERR is set, else 0.
 */
uint8_t EPIC_USART_HasOverrun(USART_InstanceTypeDef inst)
{
    uint8_t rcsta = 0U;
    USART_READ_RCSTA(inst, rcsta);
    return (rcsta & PIC_RCSTA_OERR) ? 1U : 0U;
}

/**
 * @brief  Clear a receive overrun by toggling CREN off and back on
 *         (DS39609B §18.2.2).
 * @param inst which EUSART module to reset.
 */
void EPIC_USART_ClearOverrun(USART_InstanceTypeDef inst)
{
    uint8_t rcsta = 0U;
    USART_READ_RCSTA(inst, rcsta);
    rcsta = (uint8_t)(rcsta & (uint8_t)~PIC_RCSTA_CREN);
    USART_WRITE_RCSTA(inst, rcsta);
    rcsta |= PIC_RCSTA_CREN;
    USART_WRITE_RCSTA(inst, rcsta);
}

/**
 * @brief  Find the stored handle for an instance, or NULL.
 * @param inst EUSART instance.
 * @return the stored handle pointer, or NULL.
 */
static const USART_HandleTypeDef *usart_handle(USART_InstanceTypeDef inst)
{
    if (!usart_valid(inst)) return NULL;
    return g_usart_handles[inst];
}

/**
 * @brief  Weak EUSART1 TX interrupt handler. TXIF is read-only and
 *         cleared by writing TXREG, so nothing is cleared here; the
 *         registered TX-complete callback is invoked directly.
 */
void USART_TX_IRQHandler(void)
{
    if (!EPIC_IRQ_GetFlag(PIC18_IRQ_USART1_TX)) return;
    const USART_HandleTypeDef *h = usart_handle(USART_INSTANCE_1);
    if (h && h->TxCpltCallback) h->TxCpltCallback();
}

/**
 * @brief  Weak EUSART1 RX interrupt handler: reads RCREG (clearing RCIF)
 *         and forwards the byte to the RX-complete callback registered
 *         via Init.
 */
void USART_RX_IRQHandler(void)
{
    if (!EPIC_IRQ_GetFlag(PIC18_IRQ_USART1_RX)) return;
    uint8_t data = 0U;
    USART_READ_RCREG(USART_INSTANCE_1, data);
    EPIC_IRQ_ClearFlag(PIC18_IRQ_USART1_RX);
    const USART_HandleTypeDef *h = usart_handle(USART_INSTANCE_1);
    if (h && h->RxCpltCallback) h->RxCpltCallback(data);
}

/**
 * @brief  Weak EUSART2 TX interrupt handler (PIR3<TX2IF>).
 */
void USART2_TX_IRQHandler(void)
{
    if (!EPIC_IRQ_GetFlag(PIC18_IRQ_USART2_TX)) return;
    const USART_HandleTypeDef *h = usart_handle(USART_INSTANCE_2);
    if (h && h->TxCpltCallback) h->TxCpltCallback();
}

/**
 * @brief  Weak EUSART2 RX interrupt handler (PIR3<RC2IF>).
 */
void USART2_RX_IRQHandler(void)
{
    if (!EPIC_IRQ_GetFlag(PIC18_IRQ_USART2_RX)) return;
    uint8_t data = 0U;
    USART_READ_RCREG(USART_INSTANCE_2, data);
    EPIC_IRQ_ClearFlag(PIC18_IRQ_USART2_RX);
    const USART_HandleTypeDef *h = usart_handle(USART_INSTANCE_2);
    if (h && h->RxCpltCallback) h->RxCpltCallback(data);
}
