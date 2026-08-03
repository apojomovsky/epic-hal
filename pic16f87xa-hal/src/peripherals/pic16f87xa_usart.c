/**
 * @file    pic16f87xa_usart.c
 * @brief   USART driver, implementation (DS39582B §10.0).
 *
 *   The driver only programs the SFRs; it does not model the bit
 *   shifts. The sim backend (src/sim/pic16f87xa_sim.c) re-asserts
 *   TXIF each cycle when TXEN is set and dispatches RCREG values
 *   from pic16f87xa_sim_drive_usart_rx().
 */

#include "peripherals/pic16f87xa_usart.h"
#include "core/pic16_irq.h"

/* ───────────────────────── SPBRG computation ────────────────────── */

uint16_t USART_ComputeSPBRG(uint32_t fosc_hz, uint32_t baud,
                            USART_ModeTypeDef mode,
                            USART_BaudRateHighTypeDef brgh)
{
    if (baud == 0) return 0xFFFFU;
    uint32_t divisor = (mode == USART_MODE_ASYNCHRONOUS)
                       ? (brgh == USART_BRGH_HIGH ? 16U : 64U)
                       : 4U;
    /* SPBRG = (Fosc / (divisor × baud)) - 1 */
    uint32_t x = (fosc_hz / (divisor * baud)) - 1U;
    if (x > 255U) return 0xFFFFU;
    return (uint16_t)x;
}

/* ───────────────────────── handle storage ───────────────────────── */

static const USART_HandleTypeDef *g_usart = NULL;

/* ───────────────────────── public API ───────────────────────────── */

HAL_StatusTypeDef HAL_USART_Init(const USART_HandleTypeDef *h)
{
    if (!h) return HAL_INVALID;
    g_usart = h;

    /* Program SPBRG (Bank 1, address 0x99, DS39582B §10.1). */
#ifdef PIC8_BANK1_WRITE8
    /* See target/pic16f87xa_platform.h: a plain bank-switch write here
     * silently corrupts under XC8 v4.00. A wrong SPBRG is masked in
     * sim-target testing since MPLAB SIM's UART capture isn't
     * baud-timing-sensitive; only a real receiver would show it. */
    PIC8_BANK1_WRITE8(SPBRG, h->SPBRG);
#else
    uint8_t prev = (PIC8_REG8(PIC_REG_STATUS) >> 5) & 0x03U;
    pic_select_bank(1);
    PIC8_REG8(PIC_REG_SPBRG) = h->SPBRG;
    pic_select_bank(prev);
#endif

    /* Build TXSTA (Bank 1, address 0x98).
     *   CSRC  bit 7, sync clock source
     *   TX9   bit 6, 9-bit TX
     *   TXEN  bit 5, TX enable
     *   SYNC  bit 4, sync/async
     *   BRGH  bit 2, high baud rate
     *   TX9D  bit 0, 9th bit
     * Reset value of TXSTA: 0000 -010 (TRMT=1). */
    uint8_t txsta = 0x02U;
    if (h->Mode == USART_MODE_SYNCHRONOUS) txsta |= PIC_TXSTA_SYNC;
    if (h->Mode == USART_MODE_SYNCHRONOUS &&
        h->ClockSource == USART_CLOCK_MASTER) txsta |= PIC_TXSTA_CSRC;
    if (h->BaudHigh == USART_BRGH_HIGH) txsta |= PIC_TXSTA_BRGH;
    if (h->DataWidth == USART_DATA_9BITS) txsta |= PIC_TXSTA_TX9;
    if (h->TxCpltCallback) txsta |= PIC_TXSTA_TXEN;  /* TXEN implied if user has a callback. */
    PIC8_REG8(PIC_REG_TXSTA) = txsta;

    /* Build RCSTA (Bank 0, address 0x18).
     *   SPEN bit 7, enable serial port
     *   RX9  bit 6, 9-bit RX
     *   CREN bit 4, continuous receive enable
     *   ADDEN bit 3, address detect
     * Reset value: 0000 000x. */
    uint8_t rcsta = PIC_RCSTA_SPEN;
    if (h->DataWidth == USART_DATA_9BITS) rcsta |= PIC_RCSTA_RX9;
    if (h->RxCpltCallback) rcsta |= PIC_RCSTA_CREN;
    PIC8_REG8(PIC_REG_RCSTA) = rcsta;

    /* TXIF is initially 1 (TXREG empty after reset, §10.2.1).
     * RCIF is initially 0 (RCREG empty after reset). */
    HAL_IRQ_ClearFlag(PIC16_IRQ_USART_RX);

    if (h->TxCpltCallback) HAL_IRQ_Enable(PIC16_IRQ_USART_TX);
    else                   HAL_IRQ_DisableSrc(PIC16_IRQ_USART_TX);
    if (h->RxCpltCallback) HAL_IRQ_Enable(PIC16_IRQ_USART_RX);
    else                   HAL_IRQ_DisableSrc(PIC16_IRQ_USART_RX);

    return HAL_OK;
}

HAL_StatusTypeDef HAL_USART_DeInit(void)
{
    HAL_IRQ_DisableSrc(PIC16_IRQ_USART_TX);
    HAL_IRQ_DisableSrc(PIC16_IRQ_USART_RX);
    HAL_IRQ_ClearFlag(PIC16_IRQ_USART_TX);
    HAL_IRQ_ClearFlag(PIC16_IRQ_USART_RX);
    PIC8_REG8(PIC_REG_RCSTA) = 0x00U;
    PIC8_REG8(PIC_REG_TXSTA) = 0x02U;     /* keep TRMT=1 reset state. */
    {
        uint8_t prev = (PIC8_REG8(PIC_REG_STATUS) >> 5) & 0x03U;
        pic_select_bank(1);
        PIC8_REG8(PIC_REG_SPBRG) = 0x00U;
        pic_select_bank(prev);
    }
    g_usart = NULL;
    return HAL_OK;
}

void HAL_USART_Transmit(uint8_t data)
{
    /* Writing TXREG clears TXIF (DS39582B §10.2.1). The hardware
     * simultaneously starts the TSR→line shift; the sim backend
     * re-asserts TXIF on the next pic16f87xa_sim_step() call. */
    PIC8_REG8(PIC_REG_TXREG) = data;
    HAL_IRQ_ClearFlag(PIC16_IRQ_USART_TX);
}

uint8_t HAL_USART_GetTX9D(void)
{
    return (PIC8_REG8(PIC_REG_TXSTA) & PIC_TXSTA_TX9D) ? 1U : 0U;
}

void HAL_USART_SetTX9D(uint8_t bit9)
{
    if (bit9) PIC8_BIT_SET(PIC8_REG8(PIC_REG_TXSTA), PIC_TXSTA_TX9D);
    else      PIC8_BIT_CLR(PIC8_REG8(PIC_REG_TXSTA), PIC_TXSTA_TX9D);
}

uint8_t HAL_USART_IsTxShiftRegisterEmpty(void)
{
    return (PIC8_REG8(PIC_REG_TXSTA) & PIC_TXSTA_TRMT) ? 1U : 0U;
}

uint8_t HAL_USART_Receive(void)
{
    /* Reading RCREG clears RCIF (DS39582B §10.2.2). */
    uint8_t data = PIC8_REG8(PIC_REG_RCREG);
    HAL_IRQ_ClearFlag(PIC16_IRQ_USART_RX);
    return data;
}

uint8_t HAL_USART_GetRX9D(void)
{
    return (PIC8_REG8(PIC_REG_RCSTA) & PIC_RCSTA_RX9D) ? 1U : 0U;
}

/* ───────────────────────── ISRs ─────────────────────────────────── */

void USART_TX_IRQHandler(void)
{
    if (!HAL_IRQ_GetFlag(PIC16_IRQ_USART_TX)) return;
    /* TXIF is read-only and cleared by writing TXREG; there is nothing
     * to clear here, just call the user callback. */
    if (g_usart && g_usart->TxCpltCallback) g_usart->TxCpltCallback();
}

void USART_RX_IRQHandler(void)
{
    if (!HAL_IRQ_GetFlag(PIC16_IRQ_USART_RX)) return;
    uint8_t data = PIC8_REG8(PIC_REG_RCREG);
    HAL_IRQ_ClearFlag(PIC16_IRQ_USART_RX);
    if (g_usart && g_usart->RxCpltCallback) g_usart->RxCpltCallback(data);
}
