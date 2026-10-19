/*
 * PIC18F2520-family UART sim-target test harness (core/epic_harness.h):
 * real firmware under MPLAB SIM, driven headlessly via mdb. Mirrors
 * hal/pic18/18fxx5x's UART harness; only the EUSART surface differs
 * (16-bit BRG option here, 20 MHz FOSC_HZ). The `usart` sim gate's
 * firmware (tests/example_usart.c) transmits 0x55 before the harness
 * reports, so the uartio capture holds the TX byte and PASS marker.
 */

#include "core/epic_harness.h"
#include "core/pic18_irq.h"
#include "peripherals/pic18f2520_usart.h"

#include <stdint.h>

#ifndef FOSC_HZ
#define FOSC_HZ 20000000UL
#endif

#define EPIC_HARNESS_SIM_BAUD 9600UL

static uint32_t g_cycles = 0U;

/**
 * @brief  Transmit one character on the EUSART, blocking until the shift
 *         register drains.
 * @param c character to transmit.
 */
static void s_uart_putc(char c)
{
    while (!EPIC_USART_IsTxShiftRegisterEmpty())
    {
        /* wait for the shift register to drain */
    }
    EPIC_USART_Transmit((uint8_t)c);
}

/**
 * @brief  Harness start-up (sim-target build): configures the EUSART for
 *         polled 9600-baud output and stores `cycles` as the run bound.
 *         The TX interrupt source stays disarmed throughout (null
 *         callback); transmission here is polled.
 * @param cycles bound on the run; unused on the real target.
 */
void epic_harness_init(uint32_t cycles)
{
    g_cycles = cycles;

    /* BRGH=HIGH (divisor 16) at this file's 20 MHz FOSC_HZ needs
     * SPBRG=129 for 9600 baud, the same value tests/example_usart.c
     * programs; BRGH=LOW (divisor 64) would need SPBRG=31. */
    USART_HandleTypeDef h = USART_HANDLE_DEFAULT;
    h.BaudHigh = USART_BRGH_HIGH;
    h.SPBRG = (uint8_t)USART_ComputeSPBRG(FOSC_HZ, EPIC_HARNESS_SIM_BAUD,
                                          USART_MODE_ASYNCHRONOUS,
                                          USART_BRGH_HIGH,
                                          USART_BAUDGEN_8BIT);
    h.TxCpltCallback = NULL;
    /* TXEN is always on in this driver and a null callback leaves the
     * TX source disarmed in init, so polled TX cannot latch the
     * level-held TXIF into a vector storm (epic-hal#270). */
    (void)EPIC_USART_Init(&h);
}

/**
 * @brief  Advance simulated time by one instruction cycle (sim-target
 *         build): real time advances on its own under MPLAB SIM, so this
 *         is a no-op.
 */
void epic_harness_tick(void)
{
    /* Real time advances on its own under MPLAB SIM too, nothing to pump. */
}

/**
 * @brief  Loop-continuation test (sim-target build): returns 1 while the
 *         bounded run is in progress, 0 when it is over.
 * @param iteration the current loop index.
 * @return 1 while the run should continue, 0 when the host run is over.
 */
int epic_harness_running(uint32_t iteration)
{
    return (iteration < g_cycles) ? 1 : 0;
}

/**
 * @brief  printf-style log line (sim-target build): transmits the format
 *         string over the EUSART, one character at a time.
 * @param fmt printf-style format string.
 */
void epic_harness_log(const char *fmt, ...)
{
    while (*fmt)
    {
        s_uart_putc(*fmt);
        fmt++;
    }
}
