/**
 * @file    example_serial.c
 * @brief   pic8-serial host smoke test: RX ring fills from injected bytes
 *          via the family sim's `*_sim_drive_usart_rx`, and TX drains
 *          through `pic8_dispatch_all_irqs` into a captured TXREG.
 */

#include "pic8_serial.h"
#include "core/pic8_harness.h"
#include "pic8_hal.h"

#if defined(PIC18F2455) || defined(PIC18F2550) || defined(PIC18F4455) || defined(PIC18F4550)
  #include "pic18fxx5x_sim.h"
  #define SIM_RX(b) pic18_sim_drive_usart_rx((uint8_t)(b))
  #define FOSC_HZ_  48000000UL
#else
  #include "pic16f87xa_sim.h"
  #define SIM_RX(b) pic16f87xa_sim_drive_usart_rx((uint8_t)(b))
  #define FOSC_HZ_  20000000UL
#endif

static int g_fails = 0;
#define CHECK(c, m) do { if (!(c)) { pic8_harness_log("FAIL: %s\n", m); g_fails++; } } while (0)

int main(void)
{
    pic8_harness_init(1000000UL);
    pic8_serial_init(FOSC_HZ_, 9600u);

    /* ---- RX: inject "Hi", read it back ---- */
    SIM_RX('H');
    SIM_RX('i');
    CHECK(pic8_serial_available() == 2, "rx available=2");
    uint8_t r[8] = {0};
    int n = pic8_serial_read(r, 8);
    CHECK(n == 2 && r[0] == 'H' && r[1] == 'i', "rx bytes == Hi");
    CHECK(pic8_serial_available() == 0, "rx ring drained");

    /* ---- TX: enqueue "Ok", drain via the IRQ dispatch, capture TXREG ---- */
    pic8_serial_write((const uint8_t *)"Ok", 2);
    CHECK(pic8_serial_tx_pending() == 2, "tx enqueued=2");
    pic8_harness_tick();                 /* sim_step_usart sets TXIF */
    uint8_t t[8] = {0};
    int tn = 0;
    while (pic8_serial_tx_pending() > 0 && tn < 8) {
        pic8_dispatch_all_irqs();        /* fires the TX callback -> TXREG */
        t[tn++] = PIC8_REG8(PIC_REG_TXREG);
    }
    CHECK(tn == 2 && t[0] == 'O' && t[1] == 'k', "tx bytes == Ok");
    CHECK(pic8_serial_tx_pending() == 0, "tx ring drained");

    pic8_harness_log("serial: rx=%d tx=%d fails=%d\n", n, tn, g_fails);
    return pic8_harness_report(g_fails == 0);
}
