/**
 * EUSART driver, implementation. TX1REG, RC1REG and the status registers go
 * through epic_sfr_read8/write8 so the host simulator can model the side
 * effects of those accesses; the handlers keep no cstack autos (ISR bank
 * rule, family MANUAL.md). Callback slots and the RX latches are
 * file-scope statics written from main context before the interrupt is
 * enabled.
 */
#include "peripherals/pic16f188xx_usart.h"
#include "core/pic16f188xx_irq.h"

static void (*s_tx_cb)(void) = NULL;
static void (*s_rx_cb)(uint8_t data) = NULL;
static uint8_t s_rx_byte;
static uint8_t s_rx_9d;

/**
 * @brief Compute the baud generator value for a baud rate (asynchronous).
 * @param fosc_hz System oscillator frequency in Hz.
 * @param baud Desired baud rate in bit/s.
 * @param brgh BRGH selection.
 * @param brg16 Baud generator width.
 * @return The value n, or 0xFFFF when it cannot be formed or does not fit.
 */
uint16_t USART_ComputeSPBRG(uint32_t fosc_hz, uint32_t baud,
                            USART_BaudRateHighTypeDef brgh,
                            USART_BaudGenTypeDef brg16)
{
    uint32_t div;
    uint32_t d;
    uint32_t q;

    if ((baud == 0U) || (fosc_hz == 0U))
    {
        return 0xFFFFU;
    }
    if (brg16 == USART_BAUDGEN_16BIT)
    {
        div = (brgh == USART_BRGH_HIGH) ? 4UL : 16UL;
    }
    else
    {
        div = (brgh == USART_BRGH_HIGH) ? 16UL : 64UL;
    }
    if (baud > (0xFFFFFFFFUL / div))
    {
        return 0xFFFFU;
    }
    d = div * baud;
    q = fosc_hz / d;
    if ((fosc_hz % d) >= (d - (d / 2U)))
    {
        q++;
    }
    if ((q == 0U) || ((q - 1U) > ((brg16 == USART_BAUDGEN_16BIT) ? 0xFFFFUL : 0xFFUL)))
    {
        return 0xFFFFU;
    }
    return (uint16_t)(q - 1U);
}

/**
 * @brief Check the enum fields and generator range of a handle.
 * @param h Handle to check.
 * @return 1 when every field is in range, 0 otherwise.
 */
static uint8_t fields_ok(const USART_HandleTypeDef *h)
{
    if (((unsigned int)h->BaudHigh > 1U) || ((unsigned int)h->BaudGen > 1U) ||
        ((unsigned int)h->DataWidth > 1U))
    {
        return 0U;
    }
    if ((h->BaudGen == USART_BAUDGEN_8BIT) && (h->SPBRG > 0xFFU))
    {
        return 0U;
    }
    return 1U;
}

/**
 * @brief Configure the EUSART from the handle and enable it.
 * @param h Handle with baud, width and callbacks.
 * @return EPIC_OK on success, EPIC_INVALID for a NULL or out-of-range handle.
 */
EPIC_StatusTypeDef EPIC_USART_Init(const USART_HandleTypeDef *h)
{
    uint8_t txsta;
    uint8_t rcsta;

    if ((h == NULL) || (fields_ok(h) == 0U))
    {
        return EPIC_INVALID;
    }

    /* Port off while it is configured, so no half-written setup reaches the pin. */
    epic_sfr_write8(PIC_REG_RC1STA, 0U);
    EPIC_IRQ_DisableSrc(PIC16F188XX_IRQ_USART_TX);
    EPIC_IRQ_DisableSrc(PIC16F188XX_IRQ_USART_RX);
    s_rx_cb = h->RxCpltCallback;
    s_tx_cb = h->TxCpltCallback;

    epic_sfr_write8(PIC_REG_SP1BRGL, (uint8_t)h->SPBRG);
    epic_sfr_write8(PIC_REG_SP1BRGH, (uint8_t)(h->SPBRG >> 8));
    epic_sfr_write8(PIC_REG_BAUD1CON,
        (h->BaudGen == USART_BAUDGEN_16BIT) ? PIC_BAUD1CON_BRG16 : 0U);

    txsta = PIC_TX1STA_TXEN;
    if (h->BaudHigh == USART_BRGH_HIGH)
    {
        txsta |= PIC_TX1STA_BRGH;
    }
    if (h->DataWidth == USART_DATA_9BITS)
    {
        txsta |= PIC_TX1STA_TX9;
    }
    epic_sfr_write8(PIC_REG_TX1STA, txsta);

    rcsta = (uint8_t)(PIC_RC1STA_SPEN | PIC_RC1STA_CREN);
    if (h->DataWidth == USART_DATA_9BITS)
    {
        rcsta |= PIC_RC1STA_RX9;
        if (h->AddressDetect != 0U)
        {
            rcsta |= PIC_RC1STA_ADDEN;
        }
    }
    epic_sfr_write8(PIC_REG_RC1STA, rcsta);

    if (h->RxCpltCallback != NULL)
    {
        EPIC_IRQ_Enable(PIC16F188XX_IRQ_USART_RX);
    }
    return EPIC_OK;
}

/**
 * @brief Disable the EUSART and restore the POR state of its registers.
 * @return EPIC_OK.
 */
EPIC_StatusTypeDef EPIC_USART_DeInit(void)
{
    EPIC_IRQ_DisableSrc(PIC16F188XX_IRQ_USART_TX);
    EPIC_IRQ_DisableSrc(PIC16F188XX_IRQ_USART_RX);
    epic_sfr_write8(PIC_REG_RC1STA, 0U);
    epic_sfr_write8(PIC_REG_TX1STA, PIC_TX1STA_POR_VALUE);
    epic_sfr_write8(PIC_REG_BAUD1CON, 0U);
    epic_sfr_write8(PIC_REG_SP1BRGL, 0U);
    epic_sfr_write8(PIC_REG_SP1BRGH, 0U);
    s_tx_cb = NULL;
    s_rx_cb = NULL;
    return EPIC_OK;
}

/**
 * @brief Queue one byte for transmission by writing TX1REG.
 * @param data Byte to send.
 */
void EPIC_USART_Transmit(uint8_t data)
{
    epic_sfr_write8(PIC_REG_TX1REG, data);
}

/**
 * @brief Set the ninth transmit bit for the next byte.
 * @param bit9 Nonzero sets TX9D, zero clears it.
 */
void EPIC_USART_SetTX9D(uint8_t bit9)
{
    uint8_t txsta = epic_sfr_read8(PIC_REG_TX1STA);

    if (bit9 != 0U)
    {
        txsta |= PIC_TX1STA_TX9D;
    }
    else
    {
        txsta &= (uint8_t)~PIC_TX1STA_TX9D;
    }
    epic_sfr_write8(PIC_REG_TX1STA, txsta);
}

/**
 * @brief Report whether the transmit buffer can take a byte.
 * @return 1 when TXIF is set, 0 otherwise.
 */
uint8_t EPIC_USART_IsTxBufferEmpty(void)
{
    return EPIC_IRQ_GetFlag(PIC16F188XX_IRQ_USART_TX);
}

/**
 * @brief Report whether the transmit shift register is empty.
 * @return 1 when TRMT is set, 0 otherwise.
 */
uint8_t EPIC_USART_IsTxShiftRegisterEmpty(void)
{
    return ((epic_sfr_read8(PIC_REG_TX1STA) & PIC_TX1STA_TRMT) != 0U) ? 1U : 0U;
}

/**
 * @brief Arm the one-shot transmit interrupt.
 */
void EPIC_USART_EnableTxInterrupt(void)
{
    EPIC_IRQ_Enable(PIC16F188XX_IRQ_USART_TX);
}

/**
 * @brief Report whether a received byte is waiting.
 * @return 1 when RCIF is set, 0 otherwise.
 */
uint8_t EPIC_USART_IsRxReady(void)
{
    return EPIC_IRQ_GetFlag(PIC16F188XX_IRQ_USART_RX);
}

/**
 * @brief Read the oldest received byte from RC1REG.
 * @return The byte.
 */
uint8_t EPIC_USART_Receive(void)
{
    return epic_sfr_read8(PIC_REG_RC1REG);
}

/**
 * @brief Read the ninth bit of the byte at the head of the FIFO.
 * @return RX9D, 0 or 1.
 */
uint8_t EPIC_USART_GetRX9D(void)
{
    return ((epic_sfr_read8(PIC_REG_RC1STA) & PIC_RC1STA_RX9D) != 0U) ? 1U : 0U;
}

/**
 * @brief Ninth bit of the byte the receive interrupt last delivered.
 * @return The bit the handler latched, 0 or 1.
 */
uint8_t EPIC_USART_GetLastRX9D(void)
{
    return s_rx_9d;
}

/**
 * @brief Report a receive overrun.
 * @return 1 when OERR is set, 0 otherwise.
 */
uint8_t EPIC_USART_HasOverrunError(void)
{
    return ((epic_sfr_read8(PIC_REG_RC1STA) & PIC_RC1STA_OERR) != 0U) ? 1U : 0U;
}

/**
 * @brief Clear an overrun by dropping and re-setting CREN.
 */
void EPIC_USART_ClearOverrun(void)
{
    uint8_t rcsta = epic_sfr_read8(PIC_REG_RC1STA);

    epic_sfr_write8(PIC_REG_RC1STA, (uint8_t)(rcsta & (uint8_t)~PIC_RC1STA_CREN));
    epic_sfr_write8(PIC_REG_RC1STA, (uint8_t)(rcsta | PIC_RC1STA_CREN));
}

/**
 * @brief Report a framing error on the byte at the FIFO head.
 * @return 1 when FERR is set, 0 otherwise.
 */
uint8_t EPIC_USART_HasFramingError(void)
{
    return ((epic_sfr_read8(PIC_REG_RC1STA) & PIC_RC1STA_FERR) != 0U) ? 1U : 0U;
}

/**
 * @brief Mask TXIE, then run the TX callback if one is set.
 * @details TXIF stays set while the buffer is empty and software cannot
 * clear it, so the one-shot mask is what stops the vector re-entering. No
 * cstack autos: the register is named literally.
 */
void USART_TX_IRQHandler(void)
{
    EPIC_BIT_CLR(EPIC_REG8(PIC_REG_PIE3), PIC_PIE3_TXIE);
    if (s_tx_cb != NULL)
    {
        s_tx_cb();
    }
}

/**
 * @brief Latch RX9D, read RC1REG, then run the RX callback if one is set.
 * @details The RC1REG read pops the FIFO and clears RCIF once it is empty.
 * RX9D is latched first because it belongs to the byte about to be read.
 * No cstack autos: both latches are file-scope statics.
 */
void USART_RX_IRQHandler(void)
{
    s_rx_9d = ((epic_sfr_read8(PIC_REG_RC1STA) & PIC_RC1STA_RX9D) != 0U) ? 1U : 0U;
    s_rx_byte = epic_sfr_read8(PIC_REG_RC1REG);
    if (s_rx_cb != NULL)
    {
        s_rx_cb(s_rx_byte);
    }
}
