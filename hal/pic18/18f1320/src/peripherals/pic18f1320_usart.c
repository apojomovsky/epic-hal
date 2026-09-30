/*
 * EUSART driver, implementation (DS39605F §16.0). Programs the SFRs only
 * (no bit-shift or auto-baud modeling); the sim backend re-asserts TXIF
 * and dispatches RCREG bytes. RMW on TXSTA/RCSTA/BAUDCTL uses split
 * read+write for XC8; the handle is copied into owned storage.
 */

#include "peripherals/pic18f1320_usart.h"
#include "core/pic18_irq.h"

/**
 * @brief  Compute the SPBRG reload value for a target baud rate:
 *         SPBRG = (Fosc / (divisor * baud)) - 1, integer-truncated, with
 *         the divisor taken from DS39605F Table 16-1 (sync = 4, async
 *         depends on BRG16/BRGH).
 * @param fosc_hz System oscillator frequency in Hz.
 * @param baud Desired baud rate in bits per second.
 * @param mode USART_MODE_SYNCHRONOUS or USART_MODE_ASYNCHRONOUS.
 * @param brgh High-baud-rate select (USART_BRGH_HIGH or USART_BRGH_LOW).
 * @param brg16 16-bit baud generator select (USART_BAUDGEN_16BIT or
 *              USART_BAUDGEN_8BIT).
 * @return The SPBRG value, or 0xFFFF if `baud` is 0 or the ratio exceeds
 *         the register width.
 */
uint16_t USART_ComputeSPBRG(uint32_t fosc_hz, uint32_t baud,
                            USART_ModeTypeDef mode,
                            USART_BaudRateHighTypeDef brgh,
                            USART_BaudGenTypeDef brg16)
{
    if (baud == 0) return 0xFFFFU;

    uint32_t divisor;
    if (mode == USART_MODE_SYNCHRONOUS)
    {
        divisor = 4U;
    }
    else
    {
        /* DS39605F Table 16-1, async rows. */
        if (brg16 == USART_BAUDGEN_16BIT)
        {
            divisor = (brgh == USART_BRGH_HIGH) ? 4U : 16U;
        }
        else
        {
            divisor = (brgh == USART_BRGH_HIGH) ? 16U : 64U;
        }
    }

    /* SPBRG = (Fosc / (divisor × baud)) - 1, integer-truncated. */
    uint32_t x = (fosc_hz / (divisor * baud)) - 1U;
    uint32_t max = (brg16 == USART_BAUDGEN_16BIT) ? 65535U : 255U;
    if (x > max) return 0xFFFFU;
    return (uint16_t)x;
}

static USART_HandleTypeDef        g_usart_storage;
static const USART_HandleTypeDef *g_usart = NULL;

/**
 * @brief  Initialize the EUSART from a handle. Programs SPBRG/SPBRGH,
 *         BAUDCTL (BRG16/ABDEN), TXSTA (mode/data-width/TXEN) and RCSTA
 *         (SPEN/RX9/CREN/ADDEN), then arms the TX/RX interrupts per the
 *         callbacks.
 * @param h Handle describing the EUSART configuration.
 * @return EPIC_OK on success, EPIC_INVALID if `h` is NULL.
 */
EPIC_StatusTypeDef EPIC_USART_Init(const USART_HandleTypeDef *h)
{
    if (!h) return EPIC_INVALID;
    g_usart_storage = *h;
    g_usart = &g_usart_storage;

    /* BRG: SPBRG (low) always; SPBRGH (high) only used when BRG16=1. */
    epic_sfr_write8(PIC_REG_SPBRG,  h->SPBRG);
    epic_sfr_write8(PIC_REG_SPBRGH, h->SPBRGH);

    /* BAUDCTL (Register 16-3): BRG16 + ABDEN. No ABDOVF on this part.
     * The polarity / wake-up bits default to 0. Reset value 0x00. */
    uint8_t baudctl = 0U;
    if (h->BaudGen  == USART_BAUDGEN_16BIT) baudctl |= PIC_BAUDCTL_BRG16;
    if (h->AutoBaud)                         baudctl |= PIC_BAUDCTL_ABDEN;
    epic_sfr_write8(PIC_REG_BAUDCTL, baudctl);

    /* Build TXSTA (Register 16-1): CSRC(7), TX9(6), TXEN(5), SYNC(4),
     * BRGH(2), TRMT(1 RO, set after reset), TX9D(0). Reset value 0x02. */
    uint8_t txsta = PIC_TXSTA_POR_VALUE;     /* 0x02, keep TRMT. */
    if (h->Mode == USART_MODE_SYNCHRONOUS) txsta |= PIC_TXSTA_SYNC;
    if (h->Mode == USART_MODE_SYNCHRONOUS &&
        h->ClockSource == USART_CLOCK_MASTER) txsta |= PIC_TXSTA_CSRC;
    if (h->BaudHigh   == USART_BRGH_HIGH)  txsta |= PIC_TXSTA_BRGH;
    if (h->DataWidth  == USART_DATA_9BITS) txsta |= PIC_TXSTA_TX9;
    if (h->TxCpltCallback) txsta |= PIC_TXSTA_TXEN;  /* TXEN implied if user has a callback. */
    epic_sfr_write8(PIC_REG_TXSTA, txsta);

    /* Build RCSTA (Register 16-2).
     *   SPEN  bit 7, enable serial port
     *   RX9   bit 6, 9-bit RX
     *   CREN  bit 4, continuous receive enable
     *   ADDEN bit 3, address detect (9-bit)
     * Reset value: 0x00. */
    uint8_t rcsta = PIC_RCSTA_SPEN;
    if (h->DataWidth    == USART_DATA_9BITS) rcsta |= PIC_RCSTA_RX9;
    if (h->AddressDetect)                     rcsta |= PIC_RCSTA_ADDEN;
    if (h->RxCpltCallback)                    rcsta |= PIC_RCSTA_CREN;
    epic_sfr_write8(PIC_REG_RCSTA, rcsta);

    /* TXIF is initially 1 (TXREG empty after reset, §16.3.1); RCIF is 0. */
    EPIC_IRQ_ClearFlag(PIC18_IRQ_USART_RX);

    if (h->TxCpltCallback) EPIC_IRQ_Enable(PIC18_IRQ_USART_TX);
    else                   EPIC_IRQ_DisableSrc(PIC18_IRQ_USART_TX);
    if (h->RxCpltCallback) EPIC_IRQ_Enable(PIC18_IRQ_USART_RX);
    else                   EPIC_IRQ_DisableSrc(PIC18_IRQ_USART_RX);

    return EPIC_OK;
}

/**
 * @brief  De-initialize the EUSART: disable the TX/RX interrupts, clear
 *         the flags, restore RCSTA/TXSTA/BAUDCTL/SPBRG/SPBRGH to their
 *         power-on values and drop the stored handle.
 * @return EPIC_OK.
 */
EPIC_StatusTypeDef EPIC_USART_DeInit(void)
{
    EPIC_IRQ_DisableSrc(PIC18_IRQ_USART_TX);
    EPIC_IRQ_DisableSrc(PIC18_IRQ_USART_RX);
    EPIC_IRQ_ClearFlag(PIC18_IRQ_USART_TX);
    EPIC_IRQ_ClearFlag(PIC18_IRQ_USART_RX);
    epic_sfr_write8(PIC_REG_RCSTA,   PIC_RCSTA_POR_VALUE);    /* 0x00 */
    epic_sfr_write8(PIC_REG_TXSTA,    PIC_TXSTA_POR_VALUE);   /* 0x02, keep TRMT */
    epic_sfr_write8(PIC_REG_BAUDCTL,  PIC_BAUDCTL_POR_VALUE); /* 0x00 */
    epic_sfr_write8(PIC_REG_SPBRG,    PIC_SPBRG_POR_VALUE);
    epic_sfr_write8(PIC_REG_SPBRGH,   PIC_SPBRGH_POR_VALUE);
    g_usart = NULL;
    return EPIC_OK;
}

/**
 * @brief  Transmit one byte: writing TXREG clears TXIF and starts the
 *         TSR-to-line shift.
 * @param data Byte to transmit.
 */
void EPIC_USART_Transmit(uint8_t data)
{
    /* Writing TXREG clears TXIF (DS39605F §16.3.1). The hardware starts the
     * TSR→line shift; the sim backend re-asserts TXIF on the next
     * pic18_sim_step() call. */
    epic_sfr_write8(PIC_REG_TXREG, data);
    EPIC_IRQ_ClearFlag(PIC18_IRQ_USART_TX);
}

/**
 * @brief  Return the 9th transmit data bit (TXSTA<TX9D>).
 * @return 1 if the TX9D bit is set, else 0.
 */
uint8_t EPIC_USART_GetTX9D(void)
{
    return (epic_sfr_read8(PIC_REG_TXSTA) & PIC_TXSTA_TX9D) ? 1U : 0U;
}

/**
 * @brief  Set the 9th transmit data bit (TXSTA<TX9D>).
 * @param bit9 Value to write: nonzero sets, zero clears.
 */
void EPIC_USART_SetTX9D(uint8_t bit9)
{
    uint8_t v = epic_sfr_read8(PIC_REG_TXSTA);
    if (bit9) v |= PIC_TXSTA_TX9D;
    else      v &= (uint8_t)~PIC_TXSTA_TX9D;
    epic_sfr_write8(PIC_REG_TXSTA, v);
}

/**
 * @brief  Return 1 if the transmit shift register is empty (TXSTA<TRMT>).
 * @return 1 if TRMT is set, else 0.
 */
uint8_t EPIC_USART_IsTxShiftRegisterEmpty(void)
{
    return (epic_sfr_read8(PIC_REG_TXSTA) & PIC_TXSTA_TRMT) ? 1U : 0U;
}

/**
 * @brief  Receive one byte: reading RCREG clears RCIF (DS39605F §16.3.4).
 * @return The received byte.
 */
uint8_t EPIC_USART_Receive(void)
{
    /* Reading RCREG clears RCIF (DS39605F §16.3.4). */
    uint8_t data = epic_sfr_read8(PIC_REG_RCREG);
    EPIC_IRQ_ClearFlag(PIC18_IRQ_USART_RX);
    return data;
}

/**
 * @brief  Return the 9th received data bit (RCSTA<RX9D>).
 * @return 1 if the RX9D bit is set, else 0.
 */
uint8_t EPIC_USART_GetRX9D(void)
{
    return (epic_sfr_read8(PIC_REG_RCSTA) & PIC_RCSTA_RX9D) ? 1U : 0U;
}

/**
 * @brief  Return 1 if a receive overrun has occurred (RCSTA<OERR>).
 * @return 1 if OERR is set, else 0.
 */
uint8_t EPIC_USART_HasOverrun(void)
{
    return (epic_sfr_read8(PIC_REG_RCSTA) & PIC_RCSTA_OERR) ? 1U : 0U;
}

/**
 * @brief  Clear a receive overrun by toggling CREN off and back on
 *         (DS39605F §16.3.4).
 */
void EPIC_USART_ClearOverrun(void)
{
    /* DS39605F §16.3.4: clear CREN, then set it again to reset the receiver. */
    uint8_t rcsta = (uint8_t)(epic_sfr_read8(PIC_REG_RCSTA) & (uint8_t)~PIC_RCSTA_CREN);
    epic_sfr_write8(PIC_REG_RCSTA, rcsta);
    rcsta |= PIC_RCSTA_CREN;
    epic_sfr_write8(PIC_REG_RCSTA, rcsta);
}

/**
 * @brief  Start auto-baud detection by setting BAUDCTL<ABDEN>.
 */
void EPIC_USART_StartAutoBaud(void)
{
    uint8_t v = (uint8_t)(epic_sfr_read8(PIC_REG_BAUDCTL) | PIC_BAUDCTL_ABDEN);
    epic_sfr_write8(PIC_REG_BAUDCTL, v);
}

/**
 * @brief  Return 1 if auto-baud detection is in progress (BAUDCTL<ABDEN>).
 * @return 1 if ABDEN is set, else 0.
 */
uint8_t EPIC_USART_IsAutoBaudBusy(void)
{
    return (epic_sfr_read8(PIC_REG_BAUDCTL) & PIC_BAUDCTL_ABDEN) ? 1U : 0U;
}

/**
 * @brief  Weak USART TX interrupt handler. TXIF is read-only and cleared
 *         by writing TXREG, so nothing is cleared here; the registered
 *         TX-complete callback is invoked directly.
 */
void USART_TX_IRQHandler(void)
{
    if (!EPIC_IRQ_GetFlag(PIC18_IRQ_USART_TX)) return;
    /* TXIF is read-only and cleared by writing TXREG; nothing to clear
     * here, just call the user callback. */
    if (g_usart && g_usart->TxCpltCallback) g_usart->TxCpltCallback();
}

/**
 * @brief  Weak USART RX interrupt handler: reads RCREG (clearing RCIF)
 *         and forwards the byte to the RX-complete callback registered
 *         via Init.
 */
void USART_RX_IRQHandler(void)
{
    if (!EPIC_IRQ_GetFlag(PIC18_IRQ_USART_RX)) return;
    uint8_t data = epic_sfr_read8(PIC_REG_RCREG);
    EPIC_IRQ_ClearFlag(PIC18_IRQ_USART_RX);
    if (g_usart && g_usart->RxCpltCallback) g_usart->RxCpltCallback(data);
}
