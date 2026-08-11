/*
 * Interrupt-driven ring-buffered UART + printf retarget: RX is always-on
 * into a ring, TX is demand-driven (TXIE stays off until a write queues
 * bytes). Installed via the USART handle's RxCpltCallback/TxCpltCallback,
 * never by redefining the HAL's own strong USART_RX/TX_IRQHandler. Ring
 * access shared with an ISR relies on single-byte atomic fields (see the
 * ring-discipline note below), not critical sections.
 */

#include "epic_serial.h"
#include "epic_hal.h"               /* family umbrella: usart, sfr, irq, platform */
#include "core/epic_harness.h"      /* epic_dispatch_all_irqs (host TX drain)    */

#if defined(PIC18F2455) || defined(PIC18F2550) || defined(PIC18F4455) || defined(PIC18F4550)
  #define SERIAL_IS_PIC18     1
  #define SERIAL_IRQ_TX       PIC18_IRQ_USART_TX
  #define SERIAL_IRQ_RX       PIC18_IRQ_USART_RX
  #define SERIAL_TXREG_WRITE(b)  epic_sfr_write8(PIC_REG_TXREG, (uint8_t)(b))
#else
  #define SERIAL_IS_PIC18     0
  #define SERIAL_IRQ_TX       PIC16_IRQ_USART_TX
  #define SERIAL_IRQ_RX       PIC16_IRQ_USART_RX
  #define SERIAL_TXREG_WRITE(b)  (EPIC_REG8(PIC_REG_TXREG) = (uint8_t)(b))
#endif

#define SZ     EPIC_SERIAL_RING_SZ
#define MASK   (SZ - 1u)

static volatile uint8_t g_tx_buf[SZ], g_rx_buf[SZ];
static volatile uint8_t g_tx_head, g_tx_tail, g_tx_count;
static volatile uint8_t g_rx_head, g_rx_tail, g_rx_count;

/* Ring discipline (class-G): every field is one byte, so each
 * increment/decrement is a single atomic RMW on both families and
 * cannot tear. Producers write the buffer slot before bumping
 * index/count; consumers read the slot only after the count check, so
 * a half-written slot is never observed. The only ISR writer of the TX
 * ring is the USART TX handler (the RX handler touches only the RX
 * ring), so no GIE manipulation is needed: the Disable/Restore pairs
 * that used to guard these sites exposed the Finding 10.1 hazard (a
 * latched interrupt delivered inside a GIE=0 window tears the region
 * and can leave GIE cleared after ISR return). */

/* ISR callbacks (called by the HAL's USART handlers). */

static void epic_serial_on_rx(uint8_t data)
{
    if (g_rx_count < SZ) {                  /* drop on overflow */
        g_rx_buf[g_rx_head] = data;
        g_rx_head = (uint8_t)((g_rx_head + 1u) & MASK);
        g_rx_count++;
    }
}

static void epic_serial_on_tx(void)
{
    if (g_tx_count > 0u) {
        /* No GIE manipulation (see the ring-discipline note): the pop is
         * the only ISR writer of the TX ring's tail/count, and each
         * update is a single-byte atomic store. */
        uint8_t b = g_tx_buf[g_tx_tail];
        g_tx_tail = (uint8_t)((g_tx_tail + 1u) & MASK);
        g_tx_count--;
        SERIAL_TXREG_WRITE(b);              /* writing TXREG clears TXIF (HW) */
    } else {
        EPIC_IRQ_DisableSrc(SERIAL_IRQ_TX);  /* ring empty: disarm the TX ISR */
    }
}

void epic_serial_init(uint32_t fosc_hz, uint32_t baud)
{
    /* Static: the USART driver stores the handle pointer for ISR use, so
     * a stack-local handle would dangle. */
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
    /* Non-NULL callbacks make Init enable RCIE/TXIE + CREN/TXEN; RX stays
     * always-on and TX demand-driven, so TXIE goes off right after init. */
    h.RxCpltCallback = epic_serial_on_rx;
    h.TxCpltCallback = epic_serial_on_tx;
    s_usart = h;
    EPIC_USART_Init(&s_usart);
    EPIC_IRQ_DisableSrc(SERIAL_IRQ_TX);

    g_tx_head = g_tx_tail = g_tx_count = 0u;
    g_rx_head = g_rx_tail = g_rx_count = 0u;
}

int epic_serial_write(const uint8_t *data, int len)
{
    for (int i = 0; i < len; i++) {
        while (g_tx_count >= SZ) {
            epic_dispatch_all_irqs();        /* ring full: drain (host pumps, target ISR drains) */
        }
        g_tx_buf[g_tx_head] = data[i];
        g_tx_head = (uint8_t)((g_tx_head + 1u) & MASK);
        g_tx_count++;
        EPIC_IRQ_Enable(SERIAL_IRQ_TX);       /* kick the TX ISR */
    }
    return len;
}

int epic_serial_read(uint8_t *buf, int max)
{
    int n = 0;
    while (n < max && g_rx_count > 0u) {
        buf[n++] = g_rx_buf[g_rx_tail];
        g_rx_tail = (uint8_t)((g_rx_tail + 1u) & MASK);
        g_rx_count--;
    }
    return n;
}

int epic_serial_available(void)
{
    return (int)g_rx_count;                  /* single-byte read: atomic */
}

int epic_serial_tx_pending(void)
{
    return (int)g_tx_count;
}

void epic_serial_flush(void)
{
    while (g_tx_count > 0u) {
        epic_dispatch_all_irqs();
    }
    while (!EPIC_USART_IsTxShiftRegisterEmpty()) {
        epic_dispatch_all_irqs();            /* last byte still in the shift register */
    }
}

void putch(char c)
{
    uint8_t b = (uint8_t)c;
    epic_serial_write(&b, 1);
}
