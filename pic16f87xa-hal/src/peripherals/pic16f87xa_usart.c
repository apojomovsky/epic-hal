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

EPIC_StatusTypeDef EPIC_USART_Init(const USART_HandleTypeDef *h)
{
    if (!h) return EPIC_INVALID;
    g_usart = h;

    /* Program SPBRG (Bank 1, address 0x99, DS39582B §10.1). */
#ifdef EPIC_BANK1_WRITE8
    /* See target/pic16f87xa_platform.h: a plain bank-switch write here
     * silently corrupts under XC8 v4.00. A wrong SPBRG is masked in
     * sim-target testing since MPLAB SIM's UART capture isn't
     * baud-timing-sensitive; only a real receiver would show it. */
    EPIC_BANK1_WRITE8(SPBRG, h->SPBRG);
#else
    uint8_t prev = (EPIC_REG8(PIC_REG_STATUS) >> 5) & 0x03U;
    pic_select_bank(1);
    EPIC_REG8(PIC_REG_SPBRG) = h->SPBRG;
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
#ifdef EPIC_BANK1_WRITE8
    /* See target/pic16f87xa_platform.h: a plain EPIC_REG8 write to the
     * Bank-1 TXSTA (0x98) silently misdirects under XC8 v4.00 (hits
     * the Bank-0 alias 0x18, RCSTA). Probed and confirmed 2026-08-09
     * by pic16f87xa-hal/tests/sim_bank_probe.c. */
    EPIC_BANK1_WRITE8(TXSTA, txsta);
#else
    EPIC_REG8(PIC_REG_TXSTA) = txsta;
#endif

    /* Build RCSTA (Bank 0, address 0x18).
     *   SPEN bit 7, enable serial port
     *   RX9  bit 6, 9-bit RX
     *   CREN bit 4, continuous receive enable
     *   ADDEN bit 3, address detect
     * Reset value: 0000 000x. */
    uint8_t rcsta = PIC_RCSTA_SPEN;
    if (h->DataWidth == USART_DATA_9BITS) rcsta |= PIC_RCSTA_RX9;
    if (h->RxCpltCallback) rcsta |= PIC_RCSTA_CREN;
    EPIC_REG8(PIC_REG_RCSTA) = rcsta;

    /* TXIF is initially 1 (TXREG empty after reset, §10.2.1).
     * RCIF is initially 0 (RCREG empty after reset). */
    EPIC_IRQ_ClearFlag(PIC16_IRQ_USART_RX);

    if (h->TxCpltCallback) EPIC_IRQ_Enable(PIC16_IRQ_USART_TX);
    else                   EPIC_IRQ_DisableSrc(PIC16_IRQ_USART_TX);
    if (h->RxCpltCallback) EPIC_IRQ_Enable(PIC16_IRQ_USART_RX);
    else                   EPIC_IRQ_DisableSrc(PIC16_IRQ_USART_RX);

    return EPIC_OK;
}

EPIC_StatusTypeDef EPIC_USART_DeInit(void)
{
    EPIC_IRQ_DisableSrc(PIC16_IRQ_USART_TX);
    EPIC_IRQ_DisableSrc(PIC16_IRQ_USART_RX);
    EPIC_IRQ_ClearFlag(PIC16_IRQ_USART_TX);
    EPIC_IRQ_ClearFlag(PIC16_IRQ_USART_RX);
    EPIC_REG8(PIC_REG_RCSTA) = 0x00U;
    EPIC_REG8(PIC_REG_TXSTA) = 0x02U;     /* keep TRMT=1 reset state. */
    {
        uint8_t prev = (EPIC_REG8(PIC_REG_STATUS) >> 5) & 0x03U;
        pic_select_bank(1);
        EPIC_REG8(PIC_REG_SPBRG) = 0x00U;
        pic_select_bank(prev);
    }
    g_usart = NULL;
    return EPIC_OK;
}

void EPIC_USART_Transmit(uint8_t data)
{
    /* Writing TXREG clears TXIF (DS39582B §10.2.1). The hardware
     * simultaneously starts the TSR→line shift; the sim backend
     * re-asserts TXIF on the next pic16f87xa_sim_step() call. */
    EPIC_REG8(PIC_REG_TXREG) = data;
    EPIC_IRQ_ClearFlag(PIC16_IRQ_USART_TX);
}

uint8_t EPIC_USART_GetTX9D(void)
{
#ifdef EPIC_BANK1_READ8
    uint8_t txsta = 0u;
    EPIC_BANK1_READ8(TXSTA, txsta);
    return (txsta & PIC_TXSTA_TX9D) ? 1U : 0U;
#else
    return (EPIC_REG8(PIC_REG_TXSTA) & PIC_TXSTA_TX9D) ? 1U : 0U;
#endif
}

void EPIC_USART_SetTX9D(uint8_t bit9)
{
#ifdef EPIC_BANK1_READ8
    /* Plain EPIC_REG8 RMWs on the Bank-1 TXSTA silently misdirect to
     * the Bank-0 alias (RCSTA) under XC8 v4.00; see the probe note in
     * EPIC_USART_Init. */
    uint8_t txsta = 0u;
    EPIC_BANK1_READ8(TXSTA, txsta);
    if (bit9) {
        txsta |= PIC_TXSTA_TX9D;
    } else {
        txsta &= (uint8_t)~PIC_TXSTA_TX9D;
    }
    EPIC_BANK1_WRITE8(TXSTA, txsta);
#else
    if (bit9) EPIC_BIT_SET(EPIC_REG8(PIC_REG_TXSTA), PIC_TXSTA_TX9D);
    else      EPIC_BIT_CLR(EPIC_REG8(PIC_REG_TXSTA), PIC_TXSTA_TX9D);
#endif
}

uint8_t EPIC_USART_IsTxShiftRegisterEmpty(void)
{
#ifdef EPIC_BANK1_READ8
    uint8_t txsta = 0u;
    EPIC_BANK1_READ8(TXSTA, txsta);
    return (txsta & PIC_TXSTA_TRMT) ? 1U : 0U;
#else
    return (EPIC_REG8(PIC_REG_TXSTA) & PIC_TXSTA_TRMT) ? 1U : 0U;
#endif
}

uint8_t EPIC_USART_Receive(void)
{
    /* Reading RCREG clears RCIF (DS39582B §10.2.2). */
    uint8_t data = EPIC_REG8(PIC_REG_RCREG);
    EPIC_IRQ_ClearFlag(PIC16_IRQ_USART_RX);
    return data;
}

uint8_t EPIC_USART_GetRX9D(void)
{
    return (EPIC_REG8(PIC_REG_RCSTA) & PIC_RCSTA_RX9D) ? 1U : 0U;
}

/* ───────────────────────── ISRs ─────────────────────────────────── */

void USART_TX_IRQHandler(void)
{
    /* Direct flag read (class-F: the table route clobbers PCLATH in
     * ISR context). TXIF is PIR1 bit 4; read-only, cleared by writing
     * TXREG. */
    if (!(EPIC_REG8(PIC_REG_PIR1) & PIC_PIR1_TXIF)) return;
    /* TXIF is read-only and cleared by writing TXREG; there is nothing
     * to clear here, just call the user callback. */
    if (g_usart && g_usart->TxCpltCallback) g_usart->TxCpltCallback();
}

void USART_RX_IRQHandler(void)
{
    /* Direct flag ops (class-F). RCIF is PIR1 bit 5; reading RCREG
     * clears it by hardware, the explicit clear is belt-and-suspenders
     * for callers that arrive without the read. */
    if (!(EPIC_REG8(PIC_REG_PIR1) & PIC_PIR1_RCIF)) return;
    uint8_t data = EPIC_REG8(PIC_REG_RCREG);
    EPIC_BIT_CLR(EPIC_REG8(PIC_REG_PIR1), PIC_PIR1_RCIF);
    if (g_usart && g_usart->RxCpltCallback) g_usart->RxCpltCallback(data);
}
