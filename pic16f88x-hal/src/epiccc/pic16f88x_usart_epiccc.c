/* epic-cc USART driver (sibling to src/peripherals/pic16f88x_usart.c);
 * avoids llvm.umin.i32 (generated for `x > 65535 ? 65535 : x` clamp) which
 * isel does not yet lower. The clamp is kept as a plain branch with a
 * compiler barrier so clang emits `icmp`/`br` instead of `llvm.umin`.
 * Filed as a HAL gap against epic-cc. */


#include "peripherals/pic16f88x_usart.h"
#include "core/pic16_irq.h"

/* SPBRG computation. */

/**
 * @brief Compute the SPBRG reload value for a target baud rate.
 *
 *   Async 8-bit:  rate = FOSC / (64 × (X+1))  (BRGH=0)
 *                 rate = FOSC / (16 × (X+1))  (BRGH=1)
 *   Async 16-bit: rate = FOSC / (16 × (X+1))  (BRG16=1)
 *   Sync:         rate = FOSC / (4  × (X+1))
 *
 * DS40001291H Table 12-3. Returns 0xFFFF if the divisor would exceed
 * the register width.
 * @param fosc_hz the oscillator frequency in Hz.
 * @param baud the desired baud rate in bits/s.
 * @param mode USART_MODE_ASYNCHRONOUS or USART_MODE_SYNCHRONOUS.
 * @param brgh USART_BRGH_LOW or USART_BRGH_HIGH (async only).
 * @return the SPBRG value, or 0xFFFF if unattainable.
 */
uint16_t USART_ComputeSPBRG(uint32_t fosc_hz, uint32_t baud,
                            USART_ModeTypeDef mode,
                            USART_BaudRateHighTypeDef brgh)
{
    uint32_t divisor;
    if (mode == USART_MODE_SYNCHRONOUS) {
        divisor = 4U;
    } else if (brgh == USART_BRGH_HIGH) {
        divisor = 16U;
    } else {
        divisor = 64U;
    }
    uint32_t x = fosc_hz / (divisor * baud);
    if (x > 0U) x -= 1U;
    volatile uint32_t xv = x;
    if (xv > 255U) return 0xFFFFU;
    return (uint16_t)xv;
}
/**
 * @brief Compute the 16-bit SPBRGH:SPBRG reload value (BRG16 mode).
 * @param fosc_hz the oscillator frequency in Hz.
 * @param baud the desired baud rate in bits/s.
 * @param mode USART_MODE_ASYNCHRONOUS or USART_MODE_SYNCHRONOUS.
 * @return the 16-bit value, or 0xFFFF if unattainable.
 */
uint16_t USART_ComputeSPBRG16(uint32_t fosc_hz, uint32_t baud,
                              USART_ModeTypeDef mode)
{
    uint32_t divisor = (mode == USART_MODE_SYNCHRONOUS) ? 4U : 16U;
    uint32_t x = fosc_hz / (divisor * baud);
    if (x > 0U) x -= 1U;
    volatile uint32_t xv = x;
    if (xv > 65535U) return 0xFFFFU;
    return (uint16_t)xv;
}

/* handle storage. */

static const USART_HandleTypeDef *g_usart = NULL;

/* public API. */

/**
 * @brief Initialize the EUSART: program SPBRGH:SPBRG, TXSTA, RCSTA,
 *        BAUDCTL (BRG16) and the interrupt enables for the callbacks.
 * @param h handle with Mode, ClockSource, BaudHigh, DataWidth, SPBRG,
 *        Brg16, TxCpltCallback, RxCpltCallback.
 * @return EPIC_OK on success, EPIC_INVALID if `h` is NULL.
 */
EPIC_StatusTypeDef EPIC_USART_Init(const USART_HandleTypeDef *h)
{
    if (!h) return EPIC_INVALID;
    g_usart = h;

    /* Program SPBRGH:SPBRG (Bank 1, 0x9A/0x99, DS40001291H §12.1).
     * Plain bank-switch writes silently corrupt under XC8 v4.00 (see
     * target/pic16f88x_platform.h). */
#ifdef EPIC_BANK1_WRITE8
    EPIC_BANK1_WRITE8(SPBRGH, (uint8_t)(h->SPBRG >> 8));
    EPIC_BANK1_WRITE8(SPBRG,  (uint8_t)(h->SPBRG & 0xFFU));
#else
    uint8_t prev = (EPIC_REG8(PIC_REG_STATUS) >> 5) & 0x03U;
    pic_select_bank(1);
    EPIC_REG8(PIC_REG_SPBRGH) = (uint8_t)(h->SPBRG >> 8);
    EPIC_REG8(PIC_REG_SPBRG)  = (uint8_t)(h->SPBRG & 0xFFU);
    pic_select_bank(prev);
#endif

    /* Build TXSTA (Bank 1, address 0x98).
     *   CSRC  bit 7, sync clock source
     *   TX9   bit 6, 9-bit TX
     *   TXEN  bit 5, TX enable
     *   SYNC  bit 4, sync/async
     *   SENDB bit 3, send break
     *   BRGH  bit 2, high baud rate
     *   TX9D  bit 0, 9th bit
     * Reset value of TXSTA: 0000 0010 (TRMT=1). */
    uint8_t txsta = 0x02U;
    if (h->Mode == USART_MODE_SYNCHRONOUS) txsta |= PIC_TXSTA_SYNC;
    if (h->Mode == USART_MODE_SYNCHRONOUS &&
        h->ClockSource == USART_CLOCK_MASTER) txsta |= PIC_TXSTA_CSRC;
    if (h->BaudHigh == USART_BRGH_HIGH) txsta |= PIC_TXSTA_BRGH;
    if (h->DataWidth == USART_DATA_9BITS) txsta |= PIC_TXSTA_TX9;
    if (h->TxCpltCallback) txsta |= PIC_TXSTA_TXEN;  /* TXEN implied if user has a callback. */
#ifdef EPIC_BANK1_WRITE8
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

    /* BAUDCTL (Bank 3, 0x187): BRG16 only; the other bits (ABDEN, WUE,
     * SCKP) are left for the explicit enhanced-feature calls. */
    uint8_t baudctl = 0x00U;
    if (h->Brg16) baudctl |= PIC_BAUDCTL_BRG16;
#ifdef EPIC_BANK3_WRITE8
    EPIC_BANK3_WRITE8(BAUDCTL, baudctl);
#else
    EPIC_REG8(PIC_REG_BAUDCTL) = baudctl;
#endif

    /* TXIF is initially 1 (TXREG empty after reset, §12.2.1).
     * RCIF is initially 0 (RCREG empty after reset). */
    EPIC_IRQ_ClearFlag(PIC16_IRQ_USART_RX);

    if (h->TxCpltCallback) EPIC_IRQ_Enable(PIC16_IRQ_USART_TX);
    else                   EPIC_IRQ_DisableSrc(PIC16_IRQ_USART_TX);
    if (h->RxCpltCallback) EPIC_IRQ_Enable(PIC16_IRQ_USART_RX);
    else                   EPIC_IRQ_DisableSrc(PIC16_IRQ_USART_RX);

    return EPIC_OK;
}

/**
 * @brief De-initialize the EUSART: disable both interrupts and restore
 *        RCSTA/TXSTA/BAUDCTL/SPBRGH:SPBRG to reset values.
 * @return EPIC_OK on success.
 */
EPIC_StatusTypeDef EPIC_USART_DeInit(void)
{
    EPIC_IRQ_DisableSrc(PIC16_IRQ_USART_TX);
    EPIC_IRQ_DisableSrc(PIC16_IRQ_USART_RX);
    EPIC_IRQ_ClearFlag(PIC16_IRQ_USART_TX);
    EPIC_IRQ_ClearFlag(PIC16_IRQ_USART_RX);
    EPIC_REG8(PIC_REG_RCSTA) = 0x00U;
    EPIC_REG8(PIC_REG_TXSTA) = 0x02U;     /* keep TRMT=1 reset state. */
    EPIC_REG8(PIC_REG_BAUDCTL) = 0x00U;
    {
        uint8_t prev = (EPIC_REG8(PIC_REG_STATUS) >> 5) & 0x03U;
        pic_select_bank(1);
        EPIC_REG8(PIC_REG_SPBRGH) = 0x00U;
        EPIC_REG8(PIC_REG_SPBRG)  = 0x00U;
        pic_select_bank(prev);
    }
    g_usart = NULL;
    return EPIC_OK;
}

/**
 * @brief Write one byte to TXREG (clears TXIF and starts the shift).
 * @param data the byte to transmit.
 */
void EPIC_USART_Transmit(uint8_t data)
{
    /* Writing TXREG clears TXIF (DS40001291H §12.2.1). The hardware
     * simultaneously starts the TSR→line shift; the sim backend
     * re-asserts TXIF on the next pic16f88x_sim_step() call. */
    EPIC_REG8(PIC_REG_TXREG) = data;
    EPIC_IRQ_ClearFlag(PIC16_IRQ_USART_TX);
}

/**
 * @brief Read the TX9D bit of the most recently transmitted byte.
 * @return 1 if the 9th bit was set, 0 otherwise.
 */
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

/**
 * @brief Set the 9th data bit to send next.
 * @param bit9 the 9th bit value (0 or 1).
 */
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

/**
 * @brief Report whether the transmit shift register is empty.
 * @return 1 if TRMT is set, 0 otherwise.
 */
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

/**
 * @brief Send a Sync Break character (TXSTA<SENDB>).
 */
void EPIC_USART_SendBreak(void)
{
#ifdef EPIC_BANK1_READ8
    uint8_t txsta = 0u;
    EPIC_BANK1_READ8(TXSTA, txsta);
    txsta |= PIC_TXSTA_SENDB;
    EPIC_BANK1_WRITE8(TXSTA, txsta);
#else
    EPIC_BIT_SET(EPIC_REG8(PIC_REG_TXSTA), PIC_TXSTA_SENDB);
#endif
}

/**
 * @brief Read the latest byte from RCREG (clears RCIF).
 * @return the received byte.
 */
uint8_t EPIC_USART_Receive(void)
{
    /* Reading RCREG clears RCIF (DS40001291H §12.2.2). */
    uint8_t data = EPIC_REG8(PIC_REG_RCREG);
    EPIC_IRQ_ClearFlag(PIC16_IRQ_USART_RX);
    return data;
}

/**
 * @brief Read the RX9D bit of the most recently received byte.
 * @return 1 if the 9th bit was set, 0 otherwise.
 */
uint8_t EPIC_USART_GetRX9D(void)
{
    return (EPIC_REG8(PIC_REG_RCSTA) & PIC_RCSTA_RX9D) ? 1U : 0U;
}

/* enhanced features (BAUDCTL, Bank 3). */

/**
 * @brief Enable or disable the auto-baud detect mode (BAUDCTL<ABDEN>).
 * @param enable 1 to enter auto-baud mode, 0 to leave it.
 */
void EPIC_USART_SetAutoBaud(uint8_t enable)
{
    uint8_t baudctl = EPIC_REG8(PIC_REG_BAUDCTL);
    if (enable) baudctl |= PIC_BAUDCTL_ABDEN;
    else        baudctl &= (uint8_t)~PIC_BAUDCTL_ABDEN;
    EPIC_REG8(PIC_REG_BAUDCTL) = baudctl;
}

/**
 * @brief Report whether the auto-baud timer overflowed (BAUDCTL<ABDOVF>).
 * @return 1 if the auto-baud timer overflowed, 0 otherwise.
 */
uint8_t EPIC_USART_GetAutoBaudOverflow(void)
{
    return (EPIC_REG8(PIC_REG_BAUDCTL) & PIC_BAUDCTL_ABDOVF) ? 1U : 0U;
}

/**
 * @brief Enable the wake-up on start bit (BAUDCTL<WUE>).
 * @param enable 1 to arm the wake-up, 0 to disable it.
 */
void EPIC_USART_SetWakeUp(uint8_t enable)
{
    uint8_t baudctl = EPIC_REG8(PIC_REG_BAUDCTL);
    if (enable) baudctl |= PIC_BAUDCTL_WUE;
    else        baudctl &= (uint8_t)~PIC_BAUDCTL_WUE;
    EPIC_REG8(PIC_REG_BAUDCTL) = baudctl;
}

/* ISRs. */

/**
 * @brief Weak EUSART TX ISR: fires the TX-complete callback when TXIF
 *        is set (TXIF is read-only; cleared by writing TXREG).
 */
void USART_TX_IRQHandler(void)
{
    if (!(EPIC_REG8(PIC_REG_PIR1) & PIC_PIR1_TXIF)) return;
#ifndef EPIC_AT
    if (g_usart && g_usart->TxCpltCallback) g_usart->TxCpltCallback();
#else
    (void)g_usart;
#endif
}

/**
 * @brief Weak EUSART RX ISR: reads RCREG, clears RCIF and fires the
 *        RX-complete callback with the byte.
 */
void USART_RX_IRQHandler(void)
{
    if (!(EPIC_REG8(PIC_REG_PIR1) & PIC_PIR1_RCIF)) return;
    uint8_t data = EPIC_REG8(PIC_REG_RCREG);
    EPIC_BIT_CLR(EPIC_REG8(PIC_REG_PIR1), PIC_PIR1_RCIF);
#ifndef EPIC_AT
    if (g_usart && g_usart->RxCpltCallback) g_usart->RxCpltCallback(data);
#else
    (void)g_usart;
#endif
}
