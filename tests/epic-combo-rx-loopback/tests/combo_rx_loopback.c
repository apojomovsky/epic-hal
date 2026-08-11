/**
 * Task 8 of the quality roadmap (docs/quality-roadmap.md): the RX
 * wall, via a target-in-the-loop harness. This file is the firmware:
 * a real USART RX path (interrupt-driven, line-framing echo) a host
 * can talk to through the actual UART pins. MPLAB SIM cannot inject
 * RX (docs/toolchain-coverage.md), so the mdb leg checks what the sim
 * CAN show (RX arm state, boot banner over TX); the echo path is
 * driven on the host sim, where pic16f87xa_sim_drive_usart_rx()
 * injects bytes through the same dispatch and callback the real
 * vector runs (tests/test_rx_loopback.c).
 *
 * Line protocol (the byte-exact contract the host test and
 * scripts/serial-rx-loop.py both check): boot emits "RXLOOP UP\r\n";
 * a line is a byte run terminated by '\n' (a lone '\r' is a plain
 * payload byte); well-formed = non-empty payload, CRLF-terminated,
 * never over 32 bytes; the echo is "OK:<payload>\r\n" or
 * "ERR:<payload>\r\n", where an over-long line echoes its first 32
 * buffered bytes and discards the rest until the '\n'. Line state
 * resets after every frame.
 *
 * Build seam: the host test compiles this file with
 * RX_LOOPBACK_HOST_TEST, which drops main() and the target polled-TX
 * implementation (rx_loopback_tx is then provided by the test as a
 * byte recorder); the XC8 builds compile the file as-is.
 */

#include "core/epic_harness.h"
#include "epic_hal.h"

#include <stdint.h>

#ifndef FOSC_HZ
#define FOSC_HZ 20000000UL
#endif

#define RX_LOOPBACK_BAUD 9600UL
#define RX_LINE_MAX      32u

/** Bounded run length (see core/epic_harness.h); the loop is a bare
 *  tick pump, so the budget is small. */
#define SIM_ITERATIONS 2000UL

static uint8_t g_line[RX_LINE_MAX];
static uint8_t g_line_len;
static uint8_t g_line_overflow;

/* Forward declarations: rx_loopback_tx is the build seam (defined by
 * this file on the XC8 builds, by the host test on the host build);
 * the others are defined below and called from main/init. */
void rx_loopback_tx(uint8_t b);
void rx_loopback_init(void);
void rx_loopback_on_rx_byte(uint8_t b);

#ifndef RX_LOOPBACK_HOST_TEST

static uint16_t g_fail = 0u;
static uint32_t g_loop_ticks = 0u;

/** Inner guard on the per-byte TRMT wait (same value as the taskmgr
 *  serial combo): a wedged TX path must time out, not hang the gate. */
#define TX_TRMT_GUARD 100000UL

static void fail(uint8_t idx)
{
    static const char hx[] = "0123456789ABCDEF";
    char c[2];
    g_fail++;
    epic_harness_log("F");
    c[0] = hx[(idx >> 4) & 0xF];
    c[1] = '\0';
    epic_harness_log(c);
    c[0] = hx[idx & 0xF];
    c[1] = '\0';
    epic_harness_log(c);
    epic_harness_log(".");
}

#define CHECK(cond, idx) do {         \
    if (!(cond)) fail(idx);            \
} while (0)

/** The polled-TX helper. Target implementation: bounded TSR-empty
 *  wait, then TXREG write. The host test build replaces this with a
 *  recorder (see the file header's build-seam note). */
void rx_loopback_tx(uint8_t b)
{
    uint32_t guard = 0UL;
    while (!EPIC_USART_IsTxShiftRegisterEmpty() && guard < TX_TRMT_GUARD) {
        guard++;
    }
    EPIC_USART_Transmit(b);
}

int main(void)
{
    epic_harness_init(SIM_ITERATIONS);
    g_fail = 0u;
    g_loop_ticks = 0u;

    rx_loopback_init();

    /* GIE on: no timer runs and no flag is pending (USART init cleared
     * RCIF, TXIE is off), so no ISR can preempt the TX-free loop. */
    EPIC_IRQ_Restore(1);

    for (uint32_t i = 0u; epic_harness_running(i); i++) {
        epic_harness_tick();
        g_loop_ticks++;
    }

    CHECK(g_loop_ticks == SIM_ITERATIONS, 0x03);
    /* GIE must still be on after the loop (checked before the disable
     * below). */
    CHECK((EPIC_REG8(PIC_REG_INTCON) & PIC_INTCON_GIE) != 0u, 0x02);

    /* Stop interrupts before the final marker TX: the polled-TX wait
     * is a wedge landing zone (nothing is pending in this gate, belt
     * and suspenders). */
    EPIC_IRQ_Disable();

    /* USART RX armed: RCSTA (Bank 0) has SPEN + CREN, set by
     * EPIC_USART_Init because RxCpltCallback is non-null. */
    CHECK((EPIC_REG8(PIC_REG_RCSTA) & (PIC_RCSTA_SPEN | PIC_RCSTA_CREN)) ==
          (PIC_RCSTA_SPEN | PIC_RCSTA_CREN), 0x00);
    /* PIE1 (Bank 1) must hold exactly RCIE: the RX source armed by
     * EPIC_USART_Init, nothing else (the TX source was disabled right
     * after init, and no timer source is armed). The platform macro
     * reads the whole PIE1 byte. */
    uint8_t pie1 = 0u;
    EPIC_PIE1_READ_TXIE(pie1);
    CHECK((pie1 & PIC_PIE1_RCIE) != 0u, 0x01);
    CHECK((pie1 & (uint8_t)~(PIC_PIE1_RCIE)) == 0u, 0x04);

    return epic_harness_report(g_fail == 0u);
}

#endif /* !RX_LOOPBACK_HOST_TEST */

static void s_tx_noop(void)
{
}

static void rx_loopback_tx_str(const char *s)
{
    while (*s) {
        rx_loopback_tx((uint8_t)*s);
        s++;
    }
}

/** USART init (family-generic via epic_hal.h, mirroring the other
 *  combos): 9600 8N1, RX interrupt armed (CREN + RCIE through the
 *  RxCpltCallback), TX polled (TXIE off), then the boot banner. The
 *  handle is static because EPIC_USART_Init stores its pointer for
 *  the ISR's whole lifetime (the same dangling-pointer hazard the
 *  sim-target harness documents). */
void rx_loopback_init(void)
{
    static USART_HandleTypeDef s_usart_handle;

    s_usart_handle = (USART_HandleTypeDef)USART_HANDLE_DEFAULT;
    s_usart_handle.SPBRG = (uint8_t)USART_ComputeSPBRG(
        FOSC_HZ, RX_LOOPBACK_BAUD, USART_MODE_ASYNCHRONOUS,
        USART_BRGH_HIGH);
    /* Non-null TxCpltCallback arms TXEN (and TXIE, disabled right
     * after); non-null RxCpltCallback arms CREN and RCIE. */
    s_usart_handle.TxCpltCallback = s_tx_noop;
    s_usart_handle.RxCpltCallback = rx_loopback_on_rx_byte;
    (void)EPIC_USART_Init(&s_usart_handle);
    /* Transmission is polled: turn the TX source back off (TXIF is
     * pending immediately after reset and only clears on a TXREG
     * write, so TXIE + GIE would fire the ISR forever). */
    EPIC_IRQ_DisableSrc(PIC16_IRQ_USART_TX);
    /* Boot banner: GIE is still off here (main enables it after this
     * returns), so no ISR can preempt the polled-TX wait. */
    rx_loopback_tx_str("RXLOOP UP\r\n");
}

/** The RX callback (registered as RxCpltCallback, fired by
 *  USART_RX_IRQHandler from the interrupt path): line framing + echo
 *  per the protocol in the file header. Runs in ISR context on the
 *  target; the echo's polled TX is safe there because this callback
 *  cannot fire under MPLAB SIM (no RX injection). */
void rx_loopback_on_rx_byte(uint8_t b)
{
    if (b == (uint8_t)'\n') {
        /* Frame complete. Well-formed: CRLF-terminated and never
         * over-long. */
        uint8_t ok = (g_line_len > 0u) &&
                     (g_line[g_line_len - 1u] == (uint8_t)'\r') &&
                     (g_line_overflow == 0u);
        if (ok) {
            rx_loopback_tx_str("OK:");
            g_line_len--;   /* drop the CRLF terminator's CR */
        } else {
            rx_loopback_tx_str("ERR:");
        }
        for (uint8_t i = 0u; i < g_line_len; i++) {
            rx_loopback_tx(g_line[i]);
        }
        rx_loopback_tx_str("\r\n");
        g_line_len = 0u;
        g_line_overflow = 0u;
    } else if (g_line_overflow) {
        /* Over-long: discard everything until the frame ends. */
    } else if (g_line_len < RX_LINE_MAX) {
        g_line[g_line_len++] = b;
    } else {
        g_line_overflow = 1u;
    }
}
