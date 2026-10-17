/**
 * MSSP1 driver, implementation. SSP1BUF and the status registers go through
 * epic_sfr_read8/write8 so the host simulator can model the side effects of
 * those accesses; the handler keeps no cstack autos (ISR bank rule, family
 * MANUAL.md). The callback slot is a file-scope static written from main
 * context before the interrupt is enabled.
 */
#include "peripherals/pic16f188xx_ssp.h"
#include "core/pic16f188xx_irq.h"

static void (*s_ssp_cb)(void) = NULL;

/**
 * @brief Compute SSP1ADD for a bus clock, rounding the divisor up.
 * @param fosc_hz System oscillator frequency in Hz.
 * @param clock_hz Desired SCK or SCL frequency in Hz.
 * @return n (0..255), or 0xFFFF when it cannot be formed or does not fit.
 */
uint16_t SSP_ComputeADD(uint32_t fosc_hz, uint32_t clock_hz)
{
    uint32_t d;
    uint32_t q;

    if ((fosc_hz == 0U) || (clock_hz == 0U) || (clock_hz > (0xFFFFFFFFUL / 4UL)))
    {
        return 0xFFFFU;
    }
    d = 4UL * clock_hz;
    q = fosc_hz / d;
    if ((fosc_hz % d) != 0U)
    {
        q++;
    }
    if ((q == 0U) || ((q - 1U) > 0xFFUL))
    {
        return 0xFFFFU;
    }
    return (uint16_t)(q - 1U);
}

/**
 * @brief Check the mode and the enum fields of a handle.
 * @param h Handle to check.
 * @return 1 when every field is in range, 0 otherwise.
 */
static uint8_t fields_ok(const SSP_HandleTypeDef *h)
{
    if ((h->Mode != SSP_MODE_SPI_MASTER_FOSC_4) &&
        (h->Mode != SSP_MODE_SPI_MASTER_FOSC_16) &&
        (h->Mode != SSP_MODE_SPI_MASTER_FOSC_64) &&
        (h->Mode != SSP_MODE_I2C_MASTER) &&
        (h->Mode != SSP_MODE_SPI_MASTER_ADD))
    {
        return 0U;
    }
    return (((unsigned int)h->ClockEdge <= 1U) &&
            ((unsigned int)h->ClockPolarity <= 1U) &&
            ((unsigned int)h->SamplePhase <= 1U)) ? 1U : 0U;
}

/**
 * @brief Check that MSSP1 is enabled as an I2C master.
 * @return 1 when SSPEN is set and SSPM selects I2C master, 0 otherwise.
 */
static uint8_t is_i2c_master(void)
{
    uint8_t con1 = epic_sfr_read8(PIC_REG_SSP1CON1);

    return ((con1 & (uint8_t)(PIC_SSP1CON1_SSPEN | PIC_SSP1CON1_SSPM_MASK)) ==
            (uint8_t)(PIC_SSP1CON1_SSPEN | (uint8_t)SSP_MODE_I2C_MASTER)) ? 1U : 0U;
}

/**
 * @brief Start one I2C bus operation by setting its SSP1CON2 bit.
 * @param op SSP1CON2 operation bit (SEN, RSEN, PEN or RCEN).
 * @return EPIC_OK when started, EPIC_BUSY when not idle, EPIC_INVALID when
 *         MSSP1 is not an enabled I2C master.
 */
static EPIC_StatusTypeDef i2c_start_op(uint8_t op)
{
    uint8_t con2;

    if (is_i2c_master() == 0U)
    {
        return EPIC_INVALID;
    }
    if (EPIC_SSP_I2C_IsIdle() == 0U)
    {
        return EPIC_BUSY;
    }
    con2 = epic_sfr_read8(PIC_REG_SSP1CON2);
    epic_sfr_write8(PIC_REG_SSP1CON2, (uint8_t)(con2 | op));
    return EPIC_OK;
}

/**
 * @brief Configure MSSP1 from the handle and enable it.
 * @param h Handle with mode, SPI timing, Add and callback.
 * @return EPIC_OK on success, EPIC_INVALID for a NULL or out-of-range handle.
 */
EPIC_StatusTypeDef EPIC_SSP_Init(const SSP_HandleTypeDef *h)
{
    uint8_t stat = 0U;
    uint8_t con1;

    if ((h == NULL) || (fields_ok(h) == 0U))
    {
        return EPIC_INVALID;
    }

    /* Port off while it is configured, so no half-written setup reaches the pins. */
    epic_sfr_write8(PIC_REG_SSP1CON1, 0U);
    EPIC_IRQ_DisableSrc(PIC16F188XX_IRQ_SSP1);
    EPIC_IRQ_ClearFlag(PIC16F188XX_IRQ_SSP1);
    s_ssp_cb = h->TransferCallback;

    if (h->SamplePhase == SSP_SPI_SMP_END)
    {
        stat |= PIC_SSP1STAT_SMP;
    }
    if ((h->Mode != SSP_MODE_I2C_MASTER) && (h->ClockEdge == SSP_SPI_CKE_IDLE_ACTIVE))
    {
        stat |= PIC_SSP1STAT_CKE;
    }
    epic_sfr_write8(PIC_REG_SSP1STAT, stat);
    epic_sfr_write8(PIC_REG_SSP1ADD, h->Add);
    epic_sfr_write8(PIC_REG_SSP1CON2, 0U);

    con1 = (uint8_t)((uint8_t)h->Mode & PIC_SSP1CON1_SSPM_MASK);
    if ((h->Mode != SSP_MODE_I2C_MASTER) && (h->ClockPolarity == SSP_SPI_CKP_IDLE_HIGH))
    {
        con1 |= PIC_SSP1CON1_CKP;
    }
    con1 |= PIC_SSP1CON1_SSPEN;
    epic_sfr_write8(PIC_REG_SSP1CON1, con1);

    if (h->TransferCallback != NULL)
    {
        EPIC_IRQ_Enable(PIC16F188XX_IRQ_SSP1);
    }
    return EPIC_OK;
}

/**
 * @brief Disable MSSP1 and restore the POR state of its registers.
 * @return EPIC_OK.
 */
EPIC_StatusTypeDef EPIC_SSP_DeInit(void)
{
    epic_sfr_write8(PIC_REG_SSP1CON1, 0U);
    EPIC_IRQ_DisableSrc(PIC16F188XX_IRQ_SSP1);
    EPIC_IRQ_ClearFlag(PIC16F188XX_IRQ_SSP1);
    epic_sfr_write8(PIC_REG_SSP1STAT, 0U);
    epic_sfr_write8(PIC_REG_SSP1CON2, 0U);
    epic_sfr_write8(PIC_REG_SSP1CON3, 0U);
    epic_sfr_write8(PIC_REG_SSP1ADD, 0U);
    EPIC_IRQ_ClearFlag(PIC16F188XX_IRQ_SSP1);
    s_ssp_cb = NULL;
    return EPIC_OK;
}

/**
 * @brief Write one byte to SSP1BUF.
 * @param data Byte to send.
 * @return The byte written, or 0xFFFF on a write collision.
 */
uint16_t EPIC_SSP_WriteByte(uint8_t data)
{
    if ((epic_sfr_read8(PIC_REG_SSP1CON1) & PIC_SSP1CON1_WCOL) != 0U)
    {
        return 0xFFFFU;
    }
    epic_sfr_write8(PIC_REG_SSP1BUF, data);
    if ((epic_sfr_read8(PIC_REG_SSP1CON1) & PIC_SSP1CON1_WCOL) != 0U)
    {
        return 0xFFFFU;
    }
    return (uint16_t)data;
}

/**
 * @brief Read SSP1BUF, which clears BF.
 * @return The received byte.
 */
uint8_t EPIC_SSP_ReadByte(void)
{
    return epic_sfr_read8(PIC_REG_SSP1BUF);
}

/**
 * @brief Report whether SSP1BUF holds an unread byte.
 * @return 1 when BF is set, 0 otherwise.
 */
uint8_t EPIC_SSP_IsBufferFull(void)
{
    return ((epic_sfr_read8(PIC_REG_SSP1STAT) & PIC_SSP1STAT_BF) != 0U) ? 1U : 0U;
}

/**
 * @brief Report a write collision.
 * @return 1 when WCOL is set, 0 otherwise.
 */
uint8_t EPIC_SSP_HasWriteCollision(void)
{
    return ((epic_sfr_read8(PIC_REG_SSP1CON1) & PIC_SSP1CON1_WCOL) != 0U) ? 1U : 0U;
}

/**
 * @brief Clear the write-collision flag.
 */
void EPIC_SSP_ClearWriteCollision(void)
{
    epic_sfr_write8(PIC_REG_SSP1CON1,
        (uint8_t)(epic_sfr_read8(PIC_REG_SSP1CON1) & (uint8_t)~PIC_SSP1CON1_WCOL));
}

/**
 * @brief Report a receive overflow.
 * @return 1 when SSPOV is set, 0 otherwise.
 */
uint8_t EPIC_SSP_HasOverflow(void)
{
    return ((epic_sfr_read8(PIC_REG_SSP1CON1) & PIC_SSP1CON1_SSPOV) != 0U) ? 1U : 0U;
}

/**
 * @brief Clear the receive-overflow flag.
 */
void EPIC_SSP_ClearOverflow(void)
{
    epic_sfr_write8(PIC_REG_SSP1CON1,
        (uint8_t)(epic_sfr_read8(PIC_REG_SSP1CON1) & (uint8_t)~PIC_SSP1CON1_SSPOV));
}

/**
 * @brief Generate an I2C START.
 * @return EPIC_OK when started, EPIC_BUSY when not idle, EPIC_INVALID when
 *         MSSP1 is not an enabled I2C master.
 */
EPIC_StatusTypeDef EPIC_SSP_I2C_Start(void)
{
    return i2c_start_op(PIC_SSP1CON2_SEN);
}

/**
 * @brief Generate an I2C repeated START.
 * @return EPIC_OK when started, EPIC_BUSY when not idle, EPIC_INVALID when
 *         MSSP1 is not an enabled I2C master.
 */
EPIC_StatusTypeDef EPIC_SSP_I2C_RepeatedStart(void)
{
    return i2c_start_op(PIC_SSP1CON2_RSEN);
}

/**
 * @brief Generate an I2C STOP.
 * @return EPIC_OK when started, EPIC_BUSY when not idle, EPIC_INVALID when
 *         MSSP1 is not an enabled I2C master.
 */
EPIC_StatusTypeDef EPIC_SSP_I2C_Stop(void)
{
    return i2c_start_op(PIC_SSP1CON2_PEN);
}

/**
 * @brief Clock in one byte from the slave.
 * @return EPIC_OK when started, EPIC_BUSY when not idle, EPIC_INVALID when
 *         MSSP1 is not an enabled I2C master.
 */
EPIC_StatusTypeDef EPIC_SSP_I2C_ReceiveEnable(void)
{
    return i2c_start_op(PIC_SSP1CON2_RCEN);
}

/**
 * @brief Send the acknowledge bit after a received byte.
 * @param nack Nonzero sends NACK, zero sends ACK.
 * @return EPIC_OK when started, EPIC_BUSY when not idle, EPIC_INVALID when
 *         MSSP1 is not an enabled I2C master.
 */
EPIC_StatusTypeDef EPIC_SSP_I2C_SendAck(uint8_t nack)
{
    uint8_t con2;

    if (is_i2c_master() == 0U)
    {
        return EPIC_INVALID;
    }
    if (EPIC_SSP_I2C_IsIdle() == 0U)
    {
        return EPIC_BUSY;
    }
    /* ACKDT must hold the answer before ACKEN starts the bit. */
    con2 = (uint8_t)(epic_sfr_read8(PIC_REG_SSP1CON2) & (uint8_t)~PIC_SSP1CON2_ACKDT);
    if (nack != 0U)
    {
        con2 |= PIC_SSP1CON2_ACKDT;
    }
    epic_sfr_write8(PIC_REG_SSP1CON2, con2);
    epic_sfr_write8(PIC_REG_SSP1CON2, (uint8_t)(con2 | PIC_SSP1CON2_ACKEN));
    return EPIC_OK;
}

/**
 * @brief Report whether the I2C master is idle.
 * @return 1 when no bus operation runs and no byte is being sent, else 0.
 */
uint8_t EPIC_SSP_I2C_IsIdle(void)
{
    if ((epic_sfr_read8(PIC_REG_SSP1CON2) & PIC_SSP1CON2_OP_MASK) != 0U)
    {
        return 0U;
    }
    return ((epic_sfr_read8(PIC_REG_SSP1STAT) & PIC_SSP1STAT_RW) == 0U) ? 1U : 0U;
}

/**
 * @brief Report whether the slave acknowledged the last byte written.
 * @return 1 when ACKSTAT is clear, 0 on NACK.
 */
uint8_t EPIC_SSP_I2C_SlaveAcked(void)
{
    return ((epic_sfr_read8(PIC_REG_SSP1CON2) & PIC_SSP1CON2_ACKSTAT) == 0U) ? 1U : 0U;
}

/**
 * @brief Report a bus collision.
 * @return 1 when BCL1IF is set, 0 otherwise.
 */
uint8_t EPIC_SSP_I2C_HasBusCollision(void)
{
    return ((EPIC_REG8(PIC_REG_PIR3) & PIC_PIR3_BCL1IF) != 0U) ? 1U : 0U;
}

/**
 * @brief Clear the bus-collision flag.
 */
void EPIC_SSP_I2C_ClearBusCollision(void)
{
    EPIC_BIT_CLR(EPIC_REG8(PIC_REG_PIR3), PIC_PIR3_BCL1IF);
}

/**
 * @brief Clear SSP1IF, then run the callback if one is set.
 * @details Clears the flag first so an event raised during the callback is
 * not lost. No cstack autos: the register is named literally.
 */
void SSP_IRQHandler(void)
{
    EPIC_BIT_CLR(EPIC_REG8(PIC_REG_PIR3), PIC_PIR3_SSP1IF);
    if (s_ssp_cb != NULL)
    {
        s_ssp_cb();
    }
}
