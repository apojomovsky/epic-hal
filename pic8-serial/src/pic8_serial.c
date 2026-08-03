/**
 * @file    pic8_serial.c
 * @brief   Interrupt-driven ring-buffered UART + printf retarget: RX is
 *          always-on into a ring, TX is demand-driven (TXIE stays off
 *          until `pic8_serial_write` has bytes queued). Installed through
 *          the USART handle's `RxCpltCallback`/`TxCpltCallback`, never
 *          redefining the HAL's own strong `USART_RX/TX_IRQHandler`. Ring
 *          access shared with an ISR is critical-sectioned.
 */

#include "pic8_serial.h"
#include "pic8_hal.h"               /* family umbrella: usart, sfr, irq, platform */
#include "core/pic8_harness.h"      /* pic8_dispatch_all_irqs (host TX drain)    */

#if defined(PIC18F2455) || defined(PIC18F2550) || defined(PIC18F4455) || defined(PIC18F4550)
  #define SERIAL_IS_PIC18     1
  #define SERIAL_IRQ_TX       PIC18_IRQ_USART_TX
  #define SERIAL_IRQ_RX       PIC18_IRQ_USART_RX
  #define SERIAL_TXREG_WRITE(b)  pic8_sfr_write8(PIC_REG_TXREG, (uint8_t)(b))
#else
  #define SERIAL_IS_PIC18     0
  #define SERIAL_IRQ_TX       PIC16_IRQ_USART_TX
  #define SERIAL_IRQ_RX       PIC16_IRQ_USART_RX
  #define SERIAL_TXREG_WRITE(b)  (PIC8_REG8(PIC_REG_TXREG) = (uint8_t)(b))
#endif

#define SZ     PIC8_SERIAL_RING_SZ
#define MASK   (SZ - 1u)

static volatile uint8_t g_tx_buf[SZ], g_rx_buf[SZ];
static volatile uint8_t g_tx_head, g_tx_tail, g_tx_count;
static volatile uint8_t g_rx_head, g_rx_tail, g_rx_count;

/* ---- ISR callbacks (called by the HAL's USART handlers) ---- */

static void pic8_serial_on_rx(uint8_t data)
{
    if (g_rx_count < SZ) {                  /* drop on overflow */
        g_rx_buf[g_rx_head] = data;
        g_rx_head = (uint8_t)((g_rx_head + 1u) & MASK);
        g_rx_count++;
    }
}

static void pic8_serial_on_tx(void)
{
    if (g_tx_count > 0u) {
        uint8_t prev = HAL_IRQ_Disable();   /* atomic pop + TXREG load */
        uint8_t b = g_tx_buf[g_tx_tail];
        g_tx_tail = (uint8_t)((g_tx_tail + 1u) & MASK);
        g_tx_count--;
        HAL_IRQ_Restore(prev);
        SERIAL_TXREG_WRITE(b);              /* writing TXREG clears TXIF (HW) */
    } else {
        HAL_IRQ_DisableSrc(SERIAL_IRQ_TX);  /* ring empty: stop the TX ISR */
    }
}

/* ---- public API ---- */

void pic8_serial_init(uint32_t fosc_hz, uint32_t baud)
{
    /* Static: the USART driver stores the caller's pointer (g_usart = h), so
     * the handle must outlive the ISR -- a stack-local handle would dangle
     * and the callbacks would read stale memory. */
    static USART_HandleTypeDef s_usart;
    USART_HandleTypeDef h = USART_HANDLE_DEFAULT;
    h.Mode      = USART_MODE_ASYNCHRONOUS;
    h.BaudHigh  = USART_BRGH_HIGH;
    h.DataWidth = USART_DATA_8BITS;
#if SERIAL_IS_PIC18
    h.BaudGen   = USART_BAUDGEN_16BIT;
    uint16_t sp = USART_ComputeSPBRG(fosc_hz, baud, USART_MODE_ASYNCHRONOUS,
                                     USART_BRGH_HIGH, USART_BAUDGEN_16BIT);
    h.SPBRG  = (uint8_t)(sp & 0xFFu);
    h.SPBRGH = (uint8_t)(sp >> 8);
#else
    uint16_t sp = USART_ComputeSPBRG(fosc_hz, baud, USART_MODE_ASYNCHRONOUS,
                                     USART_BRGH_HIGH);
    h.SPBRG  = (uint8_t)sp;
#endif
    /* Non-NULL callbacks make Init enable RCIE/TXIE + CREN/TXEN. We want RX
     * always-on and TX demand-driven, so disable TXIE right after init. */
    h.RxCpltCallback = pic8_serial_on_rx;
    h.TxCpltCallback = pic8_serial_on_tx;
    s_usart = h;
    HAL_USART_Init(&s_usart);
    HAL_IRQ_DisableSrc(SERIAL_IRQ_TX);

    g_tx_head = g_tx_tail = g_tx_count = 0u;
    g_rx_head = g_rx_tail = g_rx_count = 0u;
}

int pic8_serial_write(const uint8_t *data, int len)
{
    for (int i = 0; i < len; i++) {
        uint8_t prev = HAL_IRQ_Disable();
        while (g_tx_count >= SZ) {           /* block until space */
            HAL_IRQ_Restore(prev);
            pic8_dispatch_all_irqs();        /* drain (host pumps; target ISR drains) */
            prev = HAL_IRQ_Disable();
        }
        g_tx_buf[g_tx_head] = data[i];
        g_tx_head = (uint8_t)((g_tx_head + 1u) & MASK);
        g_tx_count++;
        HAL_IRQ_Restore(prev);
        HAL_IRQ_Enable(SERIAL_IRQ_TX);       /* kick the TX ISR */
    }
    return len;
}

int pic8_serial_read(uint8_t *buf, int max)
{
    int n = 0;
    uint8_t prev = HAL_IRQ_Disable();
    while (n < max && g_rx_count > 0u) {
        buf[n++] = g_rx_buf[g_rx_tail];
        g_rx_tail = (uint8_t)((g_rx_tail + 1u) & MASK);
        g_rx_count--;
    }
    HAL_IRQ_Restore(prev);
    return n;
}

int pic8_serial_available(void)
{
    return (int)g_rx_count;                  /* single-byte read: atomic */
}

int pic8_serial_tx_pending(void)
{
    return (int)g_tx_count;
}

void pic8_serial_flush(void)
{
    while (g_tx_count > 0u) {
        pic8_dispatch_all_irqs();            /* drain the TX ring */
    }
    while (!HAL_USART_IsTxShiftRegisterEmpty()) {
        pic8_dispatch_all_irqs();            /* wait for the last byte to leave TSR */
    }
}

void putch(char c)
{
    uint8_t b = (uint8_t)c;
    pic8_serial_write(&b, 1);
}
