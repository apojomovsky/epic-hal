/*
 * EUSART driver, implementation (DS39632E §20.0). The driver only
 * programs the SFRs; it does not model bit shifts or auto-baud
 * measurement. The sim backend re-asserts TXIF each cycle when TXEN is
 * set and dispatches RCREG values from `pic18_sim_drive_usart_rx()`. RMW
 * on TXSTA/RCSTA/BAUDCON uses split read+write because XC8 cannot lower
 * a compound assignment on a volatile cast-lvalue. The handle is copied
 * into owned storage so a caller may stack-allocate it.
 */

#include "peripherals/pic18fxx5x_usart.h"
#include "core/pic18_irq.h"

uint16_t USART_ComputeSPBRG(uint32_t fosc_hz, uint32_t baud,
                            USART_ModeTypeDef mode,
                            USART_BaudRateHighTypeDef brgh,
                            USART_BaudGenTypeDef brg16)
{
    if (baud == 0) return 0xFFFFU;

    uint32_t divisor;
    if (mode == USART_MODE_SYNCHRONOUS) {
        divisor = 4U;
    } else {
        /* DS39632E Table 20-1, async rows. */
        if (brg16 == USART_BAUDGEN_16BIT) {
            divisor = (brgh == USART_BRGH_HIGH) ? 4U : 16U;
        } else {
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

EPIC_StatusTypeDef EPIC_USART_Init(const USART_HandleTypeDef *h)
{
    if (!h) return EPIC_INVALID;
    g_usart_storage = *h;
    g_usart = &g_usart_storage;

    /* BRG: SPBRG (low) always; SPBRGH (high) only used when BRG16=1. */
    epic_sfr_write8(PIC_REG_SPBRG,  h->SPBRG);
    epic_sfr_write8(PIC_REG_SPBRGH, h->SPBRGH);

    /* BAUDCON: BRG16 + ABDEN. The polarity / wake-up bits default to 0.
     * Reset value 0x00. */
    uint8_t baudcon = 0U;
    if (h->BaudGen  == USART_BAUDGEN_16BIT) baudcon |= PIC_BAUDCON_BRG16;
    if (h->AutoBaud)                         baudcon |= PIC_BAUDCON_ABDEN;
    epic_sfr_write8(PIC_REG_BAUDCON, baudcon);

    /* Build TXSTA (Register 20-1).
     *   CSRC  bit 7, sync clock source
     *   TX9   bit 6, 9-bit TX
     *   TXEN  bit 5, TX enable
     *   SYNC  bit 4, sync/async
     *   BRGH  bit 2, high baud rate
     *   TRMT  bit 1, TSR empty (read-only, set after reset)
     *   TX9D  bit 0, 9th bit
     * Reset value: 0x02 (TRMT=1). */
    uint8_t txsta = PIC_TXSTA_POR_VALUE;     /* 0x02, keep TRMT. */
    if (h->Mode == USART_MODE_SYNCHRONOUS) txsta |= PIC_TXSTA_SYNC;
    if (h->Mode == USART_MODE_SYNCHRONOUS &&
        h->ClockSource == USART_CLOCK_MASTER) txsta |= PIC_TXSTA_CSRC;
    if (h->BaudHigh   == USART_BRGH_HIGH)  txsta |= PIC_TXSTA_BRGH;
    if (h->DataWidth  == USART_DATA_9BITS) txsta |= PIC_TXSTA_TX9;
    if (h->TxCpltCallback) txsta |= PIC_TXSTA_TXEN;  /* TXEN implied if user has a callback. */
    epic_sfr_write8(PIC_REG_TXSTA, txsta);

    /* Build RCSTA (Register 20-2).
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

    /* TXIF is initially 1 (TXREG empty after reset, §20.2.1); RCIF is 0. */
    EPIC_IRQ_ClearFlag(PIC18_IRQ_USART_RX);

    if (h->TxCpltCallback) EPIC_IRQ_Enable(PIC18_IRQ_USART_TX);
    else                   EPIC_IRQ_DisableSrc(PIC18_IRQ_USART_TX);
    if (h->RxCpltCallback) EPIC_IRQ_Enable(PIC18_IRQ_USART_RX);
    else                   EPIC_IRQ_DisableSrc(PIC18_IRQ_USART_RX);

    return EPIC_OK;
}

EPIC_StatusTypeDef EPIC_USART_DeInit(void)
{
    EPIC_IRQ_DisableSrc(PIC18_IRQ_USART_TX);
    EPIC_IRQ_DisableSrc(PIC18_IRQ_USART_RX);
    EPIC_IRQ_ClearFlag(PIC18_IRQ_USART_TX);
    EPIC_IRQ_ClearFlag(PIC18_IRQ_USART_RX);
    epic_sfr_write8(PIC_REG_RCSTA,   PIC_RCSTA_POR_VALUE);    /* 0x00 */
    epic_sfr_write8(PIC_REG_TXSTA,    PIC_TXSTA_POR_VALUE);   /* 0x02, keep TRMT */
    epic_sfr_write8(PIC_REG_BAUDCON,  PIC_BAUDCON_POR_VALUE); /* 0x00 */
    epic_sfr_write8(PIC_REG_SPBRG,    PIC_SPBRG_POR_VALUE);
    epic_sfr_write8(PIC_REG_SPBRGH,   PIC_SPBRGH_POR_VALUE);
    g_usart = NULL;
    return EPIC_OK;
}

void EPIC_USART_Transmit(uint8_t data)
{
    /* Writing TXREG clears TXIF (DS39632E §20.2.1). The hardware starts the
     * TSR→line shift; the sim backend re-asserts TXIF on the next
     * pic18_sim_step() call. */
    epic_sfr_write8(PIC_REG_TXREG, data);
    EPIC_IRQ_ClearFlag(PIC18_IRQ_USART_TX);
}

uint8_t EPIC_USART_GetTX9D(void)
{
    return (epic_sfr_read8(PIC_REG_TXSTA) & PIC_TXSTA_TX9D) ? 1U : 0U;
}

void EPIC_USART_SetTX9D(uint8_t bit9)
{
    uint8_t v = epic_sfr_read8(PIC_REG_TXSTA);
    if (bit9) v |= PIC_TXSTA_TX9D;
    else      v &= (uint8_t)~PIC_TXSTA_TX9D;
    epic_sfr_write8(PIC_REG_TXSTA, v);
}

uint8_t EPIC_USART_IsTxShiftRegisterEmpty(void)
{
    return (epic_sfr_read8(PIC_REG_TXSTA) & PIC_TXSTA_TRMT) ? 1U : 0U;
}

uint8_t EPIC_USART_Receive(void)
{
    /* Reading RCREG clears RCIF (DS39632E §20.2.2). */
    uint8_t data = epic_sfr_read8(PIC_REG_RCREG);
    EPIC_IRQ_ClearFlag(PIC18_IRQ_USART_RX);
    return data;
}

uint8_t EPIC_USART_GetRX9D(void)
{
    return (epic_sfr_read8(PIC_REG_RCSTA) & PIC_RCSTA_RX9D) ? 1U : 0U;
}

uint8_t EPIC_USART_HasOverrun(void)
{
    return (epic_sfr_read8(PIC_REG_RCSTA) & PIC_RCSTA_OERR) ? 1U : 0U;
}

void EPIC_USART_ClearOverrun(void)
{
    /* DS39632E §20.2.2: clear CREN, then set it again to reset the receiver. */
    uint8_t rcsta = (uint8_t)(epic_sfr_read8(PIC_REG_RCSTA) & (uint8_t)~PIC_RCSTA_CREN);
    epic_sfr_write8(PIC_REG_RCSTA, rcsta);
    rcsta |= PIC_RCSTA_CREN;
    epic_sfr_write8(PIC_REG_RCSTA, rcsta);
}

void EPIC_USART_StartAutoBaud(void)
{
    uint8_t v = (uint8_t)(epic_sfr_read8(PIC_REG_BAUDCON) | PIC_BAUDCON_ABDEN);
    epic_sfr_write8(PIC_REG_BAUDCON, v);
}

uint8_t EPIC_USART_IsAutoBaudBusy(void)
{
    return (epic_sfr_read8(PIC_REG_BAUDCON) & PIC_BAUDCON_ABDEN) ? 1U : 0U;
}

uint8_t EPIC_USART_HasAutoBaudOverflow(void)
{
    return (epic_sfr_read8(PIC_REG_BAUDCON) & PIC_BAUDCON_ABDOVF) ? 1U : 0U;
}

void EPIC_USART_ClearAutoBaudOverflow(void)
{
    uint8_t v = (uint8_t)(epic_sfr_read8(PIC_REG_BAUDCON) & (uint8_t)~PIC_BAUDCON_ABDOVF);
    epic_sfr_write8(PIC_REG_BAUDCON, v);
}

void USART_TX_IRQHandler(void)
{
    if (!EPIC_IRQ_GetFlag(PIC18_IRQ_USART_TX)) return;
    /* TXIF is read-only and cleared by writing TXREG; nothing to clear
     * here, just call the user callback. */
    if (g_usart && g_usart->TxCpltCallback) g_usart->TxCpltCallback();
}

void USART_RX_IRQHandler(void)
{
    if (!EPIC_IRQ_GetFlag(PIC18_IRQ_USART_RX)) return;
    uint8_t data = epic_sfr_read8(PIC_REG_RCREG);
    EPIC_IRQ_ClearFlag(PIC18_IRQ_USART_RX);
    if (g_usart && g_usart->RxCpltCallback) g_usart->RxCpltCallback(data);
}
