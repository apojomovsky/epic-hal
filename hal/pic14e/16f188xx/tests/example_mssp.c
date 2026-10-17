/**
 * MSSP1 host-sim probe. Host-only: the SPI shifter and the I2C engine
 * advance through epic_harness_tick, so it is not in the XC8 build. Covers
 * the SSP1ADD math, Init validation and register images, SPI transfers
 * (timing per mode, MISO and loopback, write collision, receive overflow),
 * the SSP1 interrupt, and the I2C master against a scripted slave:
 * START, repeated START, STOP, address and data bytes with ACK and NACK,
 * two-byte read ending in NACK, and the busy and mode guards.
 *
 * Expected register image (SPI master Fosc/4, CKE=1, CKP=0, SMP=0):
 *   SSP1STAT = 0x40, SSP1CON1 = 0x20, SSP1ADD = 0x00
 * I2C master, Add 7, SMP=1 (slew control off, 100 kHz):
 *   SSP1STAT = 0x80, SSP1CON1 = 0x28, SSP1ADD = 0x07
 * A SPI byte takes 8 SCK periods, an I2C byte 9 SCL periods, a receive 8,
 * every other I2C operation 1; a period is 1, 4 or 16 ticks (Fosc/4, /16,
 * /64) or SSP1ADD + 1 ticks (the two Fosc/(4*(n+1)) modes).
 */
#include <stddef.h>
#include "pic16f188xx.h"
#include "pic16f188xx_sim.h"
#include "core/pic16f188xx_irq.h"
#include "core/epic_harness.h"
#include "peripherals/pic16f188xx_ssp.h"

#define SLAVE_ADDR 0x50U

static unsigned int s_calls;

/**
 * @brief SSP1 callback that records its call.
 */
static void on_ssp(void)
{
    s_calls++;
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
 * @brief Step until the I2C master reports idle, at most 2000 ticks.
 * @return Ticks stepped.
 */
static unsigned int wait_idle(void)
{
    unsigned int n = 0U;

    while ((EPIC_SSP_I2C_IsIdle() == 0U) && (n < 2000U))
    {
        epic_harness_tick();
        n++;
    }
    return n;
}

/**
 * @brief Write one I2C byte and wait for it to finish.
 * @param data Byte to send.
 * @return 1 when the slave acknowledged, 0 on NACK or write collision.
 */
static uint8_t i2c_write(uint8_t data)
{
    if (EPIC_SSP_WriteByte(data) == 0xFFFFU)
    {
        return 0U;
    }
    (void)wait_idle();
    return EPIC_SSP_I2C_SlaveAcked();
}

/**
 * @brief Clock one byte in from the slave and answer it.
 * @param nack Nonzero ends the read with NACK.
 * @return The byte read, or 0x100 when an operation could not start.
 */
static uint16_t i2c_read(uint8_t nack)
{
    uint16_t value;

    if (EPIC_SSP_I2C_ReceiveEnable() != EPIC_OK)
    {
        return 0x100U;
    }
    (void)wait_idle();
    value = EPIC_SSP_ReadByte();
    if (EPIC_SSP_I2C_SendAck(nack) != EPIC_OK)
    {
        return 0x100U;
    }
    (void)wait_idle();
    return value;
}

/**
 * @brief MSSP1 checks on the host simulator.
 * @return Harness result: 0 on PASS, 1 on FAIL.
 */
int main(void)
{
    int ok = 1;
    SSP_HandleTypeDef h = SSP_HANDLE_DEFAULT;
    static const uint8_t slave_tx[2] = { 0xDEU, 0xADU };
    unsigned int i;

    epic_harness_init(100000UL);
    EPIC_IRQ_Restore(1U);

    /* SSP1ADD math: divisor rounds up so the bus is never too fast. */
    ok &= (SSP_ComputeADD(32000000UL, 1000000UL) == 7U);
    ok &= (SSP_ComputeADD(32000000UL, 400000UL) == 19U);
    ok &= (SSP_ComputeADD(32000000UL, 100000UL) == 79U);
    ok &= (SSP_ComputeADD(32000000UL, 300000UL) == 26U);
    ok &= (SSP_ComputeADD(32000000UL, 8000000UL) == 0U);
    ok &= (SSP_ComputeADD(32000000UL, 9000000UL) == 0U);
    ok &= (SSP_ComputeADD(32000000UL, 10000UL) == 0xFFFFU);
    ok &= (SSP_ComputeADD(32000000UL, 0UL) == 0xFFFFU);
    ok &= (SSP_ComputeADD(0UL, 100000UL) == 0xFFFFU);

    /* Validation: NULL, an unsupported SSPM (0x3 is the Timer2 mode), and
     * out-of-range fields are rejected unwritten. */
    ok &= (EPIC_SSP_Init(NULL) == EPIC_INVALID);
    h.Mode = (SSP_ModeTypeDef)3;
    ok &= (EPIC_SSP_Init(&h) == EPIC_INVALID);
    h.Mode = SSP_MODE_SPI_MASTER_FOSC_4;
    h.ClockEdge = (SSP_ClockEdgeTypeDef)2;
    ok &= (EPIC_SSP_Init(&h) == EPIC_INVALID);
    h.ClockEdge = SSP_SPI_CKE_IDLE_ACTIVE;
    ok &= (EPIC_REG8(PIC_REG_SSP1CON1) == 0x00U);

    /* I2C calls are refused while MSSP1 is off or in an SPI mode. */
    ok &= (EPIC_SSP_I2C_Start() == EPIC_INVALID);

    /* SPI register image, then one Fosc/4 transfer: 8 ticks, BF and SSP1IF
     * together at the end, the read returns MISO and clears BF. */
    ok &= (EPIC_SSP_Init(&h) == EPIC_OK);
    ok &= (EPIC_REG8(PIC_REG_SSP1STAT) == 0x40U);
    ok &= (EPIC_REG8(PIC_REG_SSP1CON1) == 0x20U);
    ok &= (EPIC_REG8(PIC_REG_SSP1ADD) == 0x00U);
    ok &= (EPIC_SSP_I2C_Start() == EPIC_INVALID);
    pic16f188xx_sim_spi_set_miso(0x3EU);
    ok &= (EPIC_SSP_WriteByte(0x9CU) == 0x9CU);
    ok &= (EPIC_SSP_IsBufferFull() == 0U);
    step(7U);
    ok &= (EPIC_SSP_IsBufferFull() == 0U);
    ok &= (EPIC_IRQ_GetFlag(PIC16F188XX_IRQ_SSP1) == 0U);
    step(1U);
    ok &= (EPIC_SSP_IsBufferFull() == 1U);
    ok &= (EPIC_IRQ_GetFlag(PIC16F188XX_IRQ_SSP1) == 1U);
    ok &= (EPIC_SSP_ReadByte() == 0x3EU);
    ok &= (EPIC_SSP_IsBufferFull() == 0U);
    ok &= (pic16f188xx_sim_spi_count() == 1U);
    ok &= (pic16f188xx_sim_spi_mosi(0U) == 0x9CU);
    EPIC_IRQ_ClearFlag(PIC16F188XX_IRQ_SSP1);
    ok &= (EPIC_IRQ_GetFlag(PIC16F188XX_IRQ_SSP1) == 0U);

    /* Write collision: a write during a transfer sends nothing, WCOL
     * latches and refuses writes until cleared. */
    ok &= (EPIC_SSP_WriteByte(0x11U) == 0x11U);
    ok &= (EPIC_SSP_WriteByte(0x22U) == 0xFFFFU);
    ok &= (EPIC_SSP_HasWriteCollision() == 1U);
    step(8U);
    ok &= (pic16f188xx_sim_spi_count() == 2U);
    ok &= (pic16f188xx_sim_spi_mosi(1U) == 0x11U);
    ok &= (EPIC_SSP_WriteByte(0x33U) == 0xFFFFU);
    EPIC_SSP_ClearWriteCollision();
    ok &= (EPIC_SSP_HasWriteCollision() == 0U);

    /* Receive overflow: a transfer that ends with BF still set raises
     * SSPOV and keeps the unread byte. */
    pic16f188xx_sim_spi_set_miso(0x77U);
    ok &= (EPIC_SSP_WriteByte(0x44U) == 0x44U);
    step(8U);
    ok &= (EPIC_SSP_HasOverflow() == 1U);
    ok &= (EPIC_SSP_ReadByte() == 0x3EU);
    EPIC_SSP_ClearOverflow();
    ok &= (EPIC_SSP_HasOverflow() == 0U);
    ok &= (EPIC_SSP_WriteByte(0x55U) == 0x55U);
    step(8U);
    ok &= (EPIC_SSP_ReadByte() == 0x77U);
    ok &= (EPIC_SSP_DeInit() == EPIC_OK);
    ok &= (EPIC_REG8(PIC_REG_SSP1CON1) == 0x00U);
    ok &= (EPIC_REG8(PIC_REG_SSP1STAT) == 0x00U);
    ok &= (EPIC_REG8(PIC_REG_SSP1ADD) == 0x00U);

    /* Mode timing: Fosc/16 is 32 ticks a byte, Fosc/64 128; the ADD mode
     * with n=3 is 4 ticks a bit, and loopback reads the byte back. */
    h.Mode = SSP_MODE_SPI_MASTER_FOSC_16;
    ok &= (EPIC_SSP_Init(&h) == EPIC_OK);
    ok &= (EPIC_REG8(PIC_REG_SSP1CON1) == 0x21U);
    pic16f188xx_sim_spi_set_miso(0x00U);
    (void)EPIC_SSP_WriteByte(0x01U);
    step(31U);
    ok &= (EPIC_SSP_IsBufferFull() == 0U);
    step(1U);
    ok &= (EPIC_SSP_IsBufferFull() == 1U);
    (void)EPIC_SSP_ReadByte();
    ok &= (EPIC_SSP_DeInit() == EPIC_OK);
    h.Mode = SSP_MODE_SPI_MASTER_FOSC_64;
    ok &= (EPIC_SSP_Init(&h) == EPIC_OK);
    ok &= (EPIC_REG8(PIC_REG_SSP1CON1) == 0x22U);
    (void)EPIC_SSP_WriteByte(0x01U);
    step(127U);
    ok &= (EPIC_SSP_IsBufferFull() == 0U);
    step(1U);
    ok &= (EPIC_SSP_IsBufferFull() == 1U);
    (void)EPIC_SSP_ReadByte();
    ok &= (EPIC_SSP_DeInit() == EPIC_OK);
    h.Mode = SSP_MODE_SPI_MASTER_ADD;
    h.Add = 3U;
    h.ClockPolarity = SSP_SPI_CKP_IDLE_HIGH;
    h.ClockEdge = SSP_SPI_CKE_ACTIVE_IDLE;
    h.SamplePhase = SSP_SPI_SMP_END;
    ok &= (EPIC_SSP_Init(&h) == EPIC_OK);
    ok &= (EPIC_REG8(PIC_REG_SSP1STAT) == 0x80U);
    ok &= (EPIC_REG8(PIC_REG_SSP1CON1) == 0x3AU);
    ok &= (EPIC_REG8(PIC_REG_SSP1ADD) == 0x03U);
    pic16f188xx_sim_spi_set_loopback(1U);
    (void)EPIC_SSP_WriteByte(0xC3U);
    step(31U);
    ok &= (EPIC_SSP_IsBufferFull() == 0U);
    step(1U);
    ok &= (EPIC_SSP_ReadByte() == 0xC3U);
    ok &= (EPIC_SSP_DeInit() == EPIC_OK);
    pic16f188xx_sim_spi_set_loopback(0U);

    /* SSP1 interrupt: Init with a callback enables SSP1IE and PEIE, the
     * dispatcher clears SSP1IF and runs the slot once per transfer. */
    h = (SSP_HandleTypeDef)SSP_HANDLE_DEFAULT;
    h.TransferCallback = on_ssp;
    ok &= (EPIC_SSP_Init(&h) == EPIC_OK);
    ok &= ((EPIC_REG8(PIC_REG_PIE3) & PIC_PIE3_SSP1IE) != 0U);
    ok &= ((EPIC_REG8(PIC_REG_INTCON) & PIC_INTCON_PEIE) != 0U);
    s_calls = 0U;
    (void)EPIC_SSP_WriteByte(0x5AU);
    step(8U);
    ok &= (s_calls == 1U);
    ok &= (EPIC_IRQ_GetFlag(PIC16F188XX_IRQ_SSP1) == 0U);
    step(8U);
    ok &= (s_calls == 1U);
    (void)EPIC_SSP_ReadByte();
    ok &= (EPIC_SSP_DeInit() == EPIC_OK);
    ok &= ((EPIC_REG8(PIC_REG_PIE3) & PIC_PIE3_SSP1IE) == 0U);

    /* I2C register image. */
    h = (SSP_HandleTypeDef)SSP_HANDLE_DEFAULT;
    h.Mode = SSP_MODE_I2C_MASTER;
    h.SamplePhase = SSP_SPI_SMP_END;
    h.Add = 7U;
    ok &= (EPIC_SSP_Init(&h) == EPIC_OK);
    ok &= (EPIC_REG8(PIC_REG_SSP1STAT) == 0x80U);
    ok &= (EPIC_REG8(PIC_REG_SSP1CON1) == 0x28U);
    ok &= (EPIC_REG8(PIC_REG_SSP1ADD) == 0x07U);
    ok &= (EPIC_SSP_I2C_IsIdle() == 1U);

    /* One START takes one SCL period (8 ticks) and sets SSP1IF and S. */
    pic16f188xx_sim_i2c_set_slave(SLAVE_ADDR, slave_tx, 2U);
    pic16f188xx_sim_i2c_clear_trace();
    ok &= (EPIC_SSP_I2C_Start() == EPIC_OK);
    ok &= (EPIC_SSP_I2C_IsIdle() == 0U);
    ok &= (EPIC_SSP_I2C_Start() == EPIC_BUSY);
    ok &= (EPIC_SSP_I2C_Stop() == EPIC_BUSY);
    ok &= (EPIC_SSP_WriteByte(0xA0U) == 0xFFFFU);
    EPIC_SSP_ClearWriteCollision();
    step(7U);
    ok &= (EPIC_SSP_I2C_IsIdle() == 0U);
    step(1U);
    ok &= (EPIC_SSP_I2C_IsIdle() == 1U);
    ok &= ((EPIC_REG8(PIC_REG_SSP1STAT) & PIC_SSP1STAT_S) != 0U);
    ok &= (EPIC_IRQ_GetFlag(PIC16F188XX_IRQ_SSP1) == 1U);

    /* Write transaction: address then a data byte, both acknowledged. A
     * byte takes 9 periods (72 ticks). */
    ok &= (EPIC_SSP_WriteByte(0xA0U) == 0xA0U);
    ok &= (EPIC_SSP_I2C_IsIdle() == 0U);
    ok &= (wait_idle() == 72U);
    ok &= (EPIC_SSP_I2C_SlaveAcked() == 1U);
    ok &= (i2c_write(0x12U) == 1U);

    /* Repeated START then a read: address with R set, two bytes, the
     * last answered with NACK, then STOP. */
    ok &= (EPIC_SSP_I2C_RepeatedStart() == EPIC_OK);
    (void)wait_idle();
    ok &= (i2c_write(0xA1U) == 1U);
    ok &= (i2c_read(0U) == 0xDEU);
    ok &= (i2c_read(1U) == 0xADU);
    ok &= (EPIC_SSP_I2C_Stop() == EPIC_OK);
    (void)wait_idle();
    ok &= ((EPIC_REG8(PIC_REG_SSP1STAT) & PIC_SSP1STAT_P) != 0U);
    ok &= ((EPIC_REG8(PIC_REG_SSP1STAT) & PIC_SSP1STAT_S) == 0U);

    /* The whole transaction as the bus saw it. */
    {
        static const uint16_t expect[] = {
            PIC16F188XX_SIM_I2C_START,
            PIC16F188XX_SIM_I2C_ADDRESS | 0xA0U,
            PIC16F188XX_SIM_I2C_DATA | 0x12U,
            PIC16F188XX_SIM_I2C_RESTART,
            PIC16F188XX_SIM_I2C_ADDRESS | 0xA1U,
            PIC16F188XX_SIM_I2C_READ | 0xDEU,
            PIC16F188XX_SIM_I2C_ACK | 0U,
            PIC16F188XX_SIM_I2C_READ | 0xADU,
            PIC16F188XX_SIM_I2C_ACK | 1U,
            PIC16F188XX_SIM_I2C_STOP,
        };
        ok &= (pic16f188xx_sim_i2c_trace_count() == (sizeof expect / sizeof expect[0]));
        for (i = 0U; i < (sizeof expect / sizeof expect[0]); i++)
        {
            ok &= (pic16f188xx_sim_i2c_trace((uint16_t)i) == expect[i]);
        }
    }

    /* An address nobody owns is NACKed, and the data byte after it too. */
    ok &= (EPIC_SSP_I2C_Start() == EPIC_OK);
    (void)wait_idle();
    ok &= (i2c_write(0xA2U) == 0U);
    ok &= (i2c_write(0x00U) == 0U);
    ok &= (EPIC_SSP_I2C_Stop() == EPIC_OK);
    (void)wait_idle();

    /* Bus collision is a PIR3 flag with its own read and clear. */
    ok &= (EPIC_SSP_I2C_HasBusCollision() == 0U);
    EPIC_REG8(PIC_REG_PIR3) |= PIC_PIR3_BCL1IF;
    ok &= (EPIC_SSP_I2C_HasBusCollision() == 1U);
    EPIC_SSP_I2C_ClearBusCollision();
    ok &= (EPIC_SSP_I2C_HasBusCollision() == 0U);

    ok &= (EPIC_SSP_DeInit() == EPIC_OK);
    ok &= (EPIC_REG8(PIC_REG_SSP1CON1) == 0x00U);
    ok &= (EPIC_SSP_I2C_Start() == EPIC_INVALID);

    epic_harness_log("mssp: %s\n", ok ? "PASS" : "FAIL");
    return epic_harness_report(ok);
}
