/**
 * EUSART host-sim probe. Host-only: the TX shifter and the RX FIFO advance
 * through epic_harness_tick, so it is not in the XC8 build. Covers the
 * baud-generator math, Init validation and register image, the TX frame
 * timing and double buffer, 9-bit frames in both directions with address
 * detect, the two-deep RX FIFO with overrun and its clear, and both
 * interrupt paths through the shared dispatcher.
 *
 * Expected register image (32 MHz, BRGH=1, 8-bit generator, n=207, after
 * Init and one tick):
 *   SP1BRGL = 0xCF, SP1BRGH = 0x00
 *   BAUD1CON = 0x40  (RCIDL set, BRG16 clear)
 *   TX1STA = 0x26    (TXEN, BRGH, TRMT)
 *   RC1STA = 0x90    (SPEN, CREN)
 * One frame is 10 bits of 16 * 208 / 4 = 832 ticks each.
 */
#include <stddef.h>
#include "pic16f188xx.h"
#include "pic16f188xx_sim.h"
#include "core/pic16f188xx_irq.h"
#include "core/epic_harness.h"
#include "peripherals/pic16f188xx_usart.h"

#define FOSC_HZ 32000000UL
#define FRAME_TICKS 8320U

static unsigned int s_rx_calls;
static uint8_t s_rx_last;
static uint8_t s_rx_last9;
static unsigned int s_tx_calls;

/**
 * @brief RX callback that records the byte and its ninth bit.
 * @param data Received byte.
 */
static void on_rx(uint8_t data)
{
    s_rx_calls++;
    s_rx_last = data;
    s_rx_last9 = EPIC_USART_GetLastRX9D();
}

/**
 * @brief TX callback that records its call.
 */
static void on_tx(void)
{
    s_tx_calls++;
}

/**
 * @brief Step the simulator n instruction ticks.
 * @param n Tick count.
 */
static void step(unsigned int n)
{
    unsigned int i;

    for (i = 0U; i < n; i++)
    {
        epic_harness_tick();
    }
}

/**
 * @brief EUSART checks on the host simulator.
 * @return Harness result: 0 on PASS, 1 on FAIL.
 */
int main(void)
{
    int ok = 1;
    USART_HandleTypeDef h = USART_HANDLE_DEFAULT;

    epic_harness_init(100000UL);
    EPIC_IRQ_Restore(1U);

    /* Baud math: nearest divisor, range checks for both generator widths. */
    ok &= (USART_ComputeSPBRG(FOSC_HZ, 9600UL, USART_BRGH_HIGH, USART_BAUDGEN_8BIT) == 207U);
    ok &= (USART_ComputeSPBRG(FOSC_HZ, 9600UL, USART_BRGH_LOW, USART_BAUDGEN_8BIT) == 51U);
    ok &= (USART_ComputeSPBRG(FOSC_HZ, 9600UL, USART_BRGH_HIGH, USART_BAUDGEN_16BIT) == 832U);
    ok &= (USART_ComputeSPBRG(FOSC_HZ, 57600UL, USART_BRGH_HIGH, USART_BAUDGEN_8BIT) == 34U);
    ok &= (USART_ComputeSPBRG(FOSC_HZ, 300UL, USART_BRGH_HIGH, USART_BAUDGEN_8BIT) == 0xFFFFU);
    ok &= (USART_ComputeSPBRG(FOSC_HZ, 300UL, USART_BRGH_LOW, USART_BAUDGEN_16BIT) == 6666U);
    ok &= (USART_ComputeSPBRG(FOSC_HZ, 0UL, USART_BRGH_HIGH, USART_BAUDGEN_8BIT) == 0xFFFFU);
    ok &= (USART_ComputeSPBRG(0UL, 9600UL, USART_BRGH_HIGH, USART_BAUDGEN_8BIT) == 0xFFFFU);
    ok &= (USART_ComputeSPBRG(FOSC_HZ, 64000000UL, USART_BRGH_HIGH, USART_BAUDGEN_16BIT) == 0xFFFFU);

    /* Validation: NULL and out-of-range fields are rejected unwritten. */
    ok &= (EPIC_USART_Init(NULL) == EPIC_INVALID);
    h.SPBRG = 256U;
    ok &= (EPIC_USART_Init(&h) == EPIC_INVALID);
    h.BaudGen = (USART_BaudGenTypeDef)2;
    h.SPBRG = 10U;
    ok &= (EPIC_USART_Init(&h) == EPIC_INVALID);
    h.BaudGen = USART_BAUDGEN_8BIT;
    h.DataWidth = (USART_DataWidthTypeDef)2;
    ok &= (EPIC_USART_Init(&h) == EPIC_INVALID);
    h.DataWidth = USART_DATA_8BITS;
    ok &= (EPIC_REG8(PIC_REG_RC1STA) == 0x00U);

    /* Register image after Init. TXIF is set once the transmitter is on
     * and TX1REG is empty. */
    h.SPBRG = 207U;
    ok &= (EPIC_USART_Init(&h) == EPIC_OK);
    step(1U);
    ok &= (EPIC_REG8(PIC_REG_SP1BRGL) == 0xCFU);
    ok &= (EPIC_REG8(PIC_REG_SP1BRGH) == 0x00U);
    ok &= (EPIC_REG8(PIC_REG_BAUD1CON) == 0x40U);
    ok &= (EPIC_REG8(PIC_REG_TX1STA) == 0x26U);
    ok &= (EPIC_REG8(PIC_REG_RC1STA) == 0x90U);
    ok &= (EPIC_USART_IsTxBufferEmpty() == 1U);
    ok &= (EPIC_USART_IsTxShiftRegisterEmpty() == 1U);
    ok &= ((EPIC_REG8(PIC_REG_PIE3) & (PIC_PIE3_TXIE | PIC_PIE3_RCIE)) == 0U);

    /* One frame: TXREG write clears TXIF at once, the shifter takes the
     * byte on the next tick (TXIF back, TRMT low) and logs it when the
     * 10 bits have elapsed. */
    EPIC_USART_Transmit(0x55U);
    ok &= (EPIC_USART_IsTxBufferEmpty() == 0U);
    step(1U);
    ok &= (EPIC_USART_IsTxBufferEmpty() == 1U);
    ok &= (EPIC_USART_IsTxShiftRegisterEmpty() == 0U);
    step(8000U);
    ok &= (pic16f188xx_sim_usart_tx_count() == 0U);
    step(FRAME_TICKS - 8000U);
    ok &= (pic16f188xx_sim_usart_tx_count() == 1U);
    ok &= (pic16f188xx_sim_usart_tx_frame(0U) == 0x55U);
    ok &= (EPIC_USART_IsTxShiftRegisterEmpty() == 1U);

    /* Double buffer: a second byte queues while the first shifts, and
     * both come out in order. */
    pic16f188xx_sim_usart_tx_clear();
    EPIC_USART_Transmit(0xA1U);
    step(1U);
    EPIC_USART_Transmit(0xB2U);
    ok &= (EPIC_USART_IsTxBufferEmpty() == 0U);
    step((2U * FRAME_TICKS) + 4U);
    ok &= (pic16f188xx_sim_usart_tx_count() == 2U);
    ok &= (pic16f188xx_sim_usart_tx_frame(0U) == 0xA1U);
    ok &= (pic16f188xx_sim_usart_tx_frame(1U) == 0xB2U);

    /* Receive: RCIF follows the FIFO, the read pops it. */
    ok &= (EPIC_USART_IsRxReady() == 0U);
    ok &= (pic16f188xx_sim_usart_rx_inject(0xA5U) == 1U);
    ok &= (EPIC_USART_IsRxReady() == 1U);
    ok &= (EPIC_USART_Receive() == 0xA5U);
    ok &= (EPIC_USART_IsRxReady() == 0U);

    /* Overrun: two bytes fit, the third raises OERR and is lost; both
     * queued bytes still read, and ClearOverrun re-arms reception. */
    ok &= (pic16f188xx_sim_usart_rx_inject(0x01U) == 1U);
    ok &= (pic16f188xx_sim_usart_rx_inject(0x02U) == 1U);
    ok &= (pic16f188xx_sim_usart_rx_inject(0x03U) == 0U);
    ok &= (EPIC_USART_HasOverrunError() == 1U);
    ok &= (pic16f188xx_sim_usart_rx_inject(0x04U) == 0U);
    ok &= (EPIC_USART_Receive() == 0x01U);
    ok &= (EPIC_USART_IsRxReady() == 1U);
    ok &= (EPIC_USART_Receive() == 0x02U);
    ok &= (EPIC_USART_IsRxReady() == 0U);
    ok &= (EPIC_USART_HasOverrunError() == 1U);
    EPIC_USART_ClearOverrun();
    ok &= (EPIC_USART_HasOverrunError() == 0U);
    ok &= ((EPIC_REG8(PIC_REG_RC1STA) & PIC_RC1STA_CREN) != 0U);
    ok &= (pic16f188xx_sim_usart_rx_inject(0x05U) == 1U);
    ok &= (EPIC_USART_Receive() == 0x05U);
    ok &= (EPIC_USART_HasFramingError() == 0U);

    /* DeInit restores the POR image and masks both interrupts. */
    ok &= (EPIC_USART_DeInit() == EPIC_OK);
    ok &= (EPIC_REG8(PIC_REG_RC1STA) == 0x00U);
    ok &= (EPIC_REG8(PIC_REG_TX1STA) == 0x02U);
    ok &= (EPIC_REG8(PIC_REG_BAUD1CON) == 0x40U);
    ok &= (EPIC_REG8(PIC_REG_SP1BRGL) == 0x00U);
    step(1U);
    ok &= (EPIC_USART_IsTxBufferEmpty() == 0U);
    ok &= (pic16f188xx_sim_usart_rx_inject(0x66U) == 0U);

    /* 9-bit frames: TX9D rides with each byte, and address detect keeps
     * only frames whose ninth bit is set. */
    h.DataWidth = USART_DATA_9BITS;
    ok &= (EPIC_USART_Init(&h) == EPIC_OK);
    ok &= (EPIC_REG8(PIC_REG_TX1STA) == 0x66U);
    ok &= (EPIC_REG8(PIC_REG_RC1STA) == 0xD0U);
    pic16f188xx_sim_usart_tx_clear();
    EPIC_USART_SetTX9D(1U);
    ok &= ((EPIC_REG8(PIC_REG_TX1STA) & PIC_TX1STA_TX9D) != 0U);
    EPIC_USART_Transmit(0x12U);
    step(1U);
    EPIC_USART_SetTX9D(0U);
    EPIC_USART_Transmit(0x34U);
    step((2U * (FRAME_TICKS + 832U)) + 4U);
    ok &= (pic16f188xx_sim_usart_tx_count() == 2U);
    ok &= (pic16f188xx_sim_usart_tx_frame(0U) == 0x112U);
    ok &= (pic16f188xx_sim_usart_tx_frame(1U) == 0x034U);
    ok &= (pic16f188xx_sim_usart_rx_inject(0x1FFU) == 1U);
    ok &= (EPIC_USART_GetRX9D() == 1U);
    ok &= (EPIC_USART_Receive() == 0xFFU);
    ok &= (pic16f188xx_sim_usart_rx_inject(0x0FFU) == 1U);
    ok &= (EPIC_USART_GetRX9D() == 0U);
    ok &= (EPIC_USART_Receive() == 0xFFU);
    ok &= (EPIC_USART_DeInit() == EPIC_OK);
    h.AddressDetect = 1U;
    ok &= (EPIC_USART_Init(&h) == EPIC_OK);
    ok &= (EPIC_REG8(PIC_REG_RC1STA) == 0xD8U);
    ok &= (pic16f188xx_sim_usart_rx_inject(0x042U) == 0U);
    ok &= (EPIC_USART_IsRxReady() == 0U);
    ok &= (pic16f188xx_sim_usart_rx_inject(0x142U) == 1U);
    ok &= (EPIC_USART_Receive() == 0x42U);
    ok &= (EPIC_USART_DeInit() == EPIC_OK);

    /* RX interrupt: Init with a callback enables RCIE and PEIE; the
     * dispatcher reads the byte, the callback sees it with its ninth bit,
     * and RCIF clears. 8-bit first, then 9-bit. */
    h.DataWidth = USART_DATA_8BITS;
    h.AddressDetect = 0U;
    h.RxCpltCallback = on_rx;
    ok &= (EPIC_USART_Init(&h) == EPIC_OK);
    ok &= ((EPIC_REG8(PIC_REG_PIE3) & PIC_PIE3_RCIE) != 0U);
    ok &= ((EPIC_REG8(PIC_REG_INTCON) & PIC_INTCON_PEIE) != 0U);
    s_rx_calls = 0U;
    ok &= (pic16f188xx_sim_usart_rx_inject(0x3CU) == 1U);
    step(1U);
    ok &= (s_rx_calls == 1U);
    ok &= (s_rx_last == 0x3CU);
    ok &= (s_rx_last9 == 0U);
    ok &= (EPIC_USART_IsRxReady() == 0U);
    ok &= (EPIC_USART_DeInit() == EPIC_OK);
    ok &= ((EPIC_REG8(PIC_REG_PIE3) & PIC_PIE3_RCIE) == 0U);
    h.DataWidth = USART_DATA_9BITS;
    ok &= (EPIC_USART_Init(&h) == EPIC_OK);
    ok &= (pic16f188xx_sim_usart_rx_inject(0x155U) == 1U);
    step(1U);
    ok &= (s_rx_calls == 2U);
    ok &= (s_rx_last == 0x55U);
    ok &= (s_rx_last9 == 1U);
    ok &= (EPIC_USART_DeInit() == EPIC_OK);
    step(4U);
    ok &= (s_rx_calls == 2U);

    /* TX interrupt: one-shot. Init leaves TXIE off, Enable arms it, the
     * handler masks it before the callback, so the callback runs once
     * even though TXIF stays set. */
    h.DataWidth = USART_DATA_8BITS;
    h.RxCpltCallback = NULL;
    h.TxCpltCallback = on_tx;
    ok &= (EPIC_USART_Init(&h) == EPIC_OK);
    ok &= ((EPIC_REG8(PIC_REG_PIE3) & PIC_PIE3_TXIE) == 0U);
    s_tx_calls = 0U;
    step(4U);
    ok &= (s_tx_calls == 0U);
    EPIC_USART_EnableTxInterrupt();
    ok &= ((EPIC_REG8(PIC_REG_PIE3) & PIC_PIE3_TXIE) != 0U);
    step(4U);
    ok &= (s_tx_calls == 1U);
    ok &= ((EPIC_REG8(PIC_REG_PIE3) & PIC_PIE3_TXIE) == 0U);
    EPIC_USART_EnableTxInterrupt();
    step(1U);
    ok &= (s_tx_calls == 2U);
    ok &= (EPIC_USART_DeInit() == EPIC_OK);

    epic_harness_log("usart: %s\n", ok ? "PASS" : "FAIL");
    return epic_harness_report(ok);
}
