/**
 * Host model of the EUSART (async TX/RX), MSSP1 (SPI master, I2C master vs
 * one scripted slave) and the PPSLOCK key sequence. Side effects hang off
 * the epic_sfr_read8/write8 hooks; time is instruction ticks (Fosc/4)
 * derived from the programmed divisors. Not modelled: bus collision, slave
 * modes, framing errors, the PPS1WAY one-shot.
 */
#include <stddef.h>
#include <stdint.h>
#include "pic16f188xx.h"
#include "pic16f188xx_sim.h"
#include "pic16f188xx_sim_priv.h"

#define SIM_LOG_SIZE 64U

#define SIM_SSPM_SPI_FOSC_4  0x0U
#define SIM_SSPM_SPI_FOSC_16 0x1U
#define SIM_SSPM_SPI_FOSC_64 0x2U
#define SIM_SSPM_I2C_MASTER  0x8U
#define SIM_SSPM_SPI_ADD     0xAU

#define SIM_RX_FIFO_DEPTH 2U

/* EUSART transmitter: TX1REG holds a pending frame (bit 8 is TX9D) until
 * the shift register frees; the shift register counts a frame down. */
static uint8_t  sim_tx_full;
static uint16_t sim_tx_hold;
static uint8_t  sim_tx_busy;
static uint16_t sim_tx_tsr;
static uint32_t sim_tx_left;
static uint16_t sim_tx_log[SIM_LOG_SIZE];
static uint16_t sim_tx_log_n;

/* EUSART receiver: the two-deep FIFO behind RC1REG (bit 8 is RX9D). */
static uint16_t sim_rx_fifo[SIM_RX_FIFO_DEPTH];
static uint8_t  sim_rx_n;

/* MSSP1: one bus operation at a time, finished after a tick countdown. */
typedef enum {
    SIM_OP_IDLE = 0,
    SIM_OP_SPI,
    SIM_OP_START,
    SIM_OP_RESTART,
    SIM_OP_STOP,
    SIM_OP_WRITE,
    SIM_OP_RECEIVE,
    SIM_OP_ACK
} sim_op_t;

static sim_op_t sim_op;
static uint32_t sim_op_left;
static uint8_t  sim_op_data;
static uint8_t  sim_spi_miso;
static uint8_t  sim_spi_loop;
static uint8_t  sim_spi_log[SIM_LOG_SIZE];
static uint16_t sim_spi_log_n;

static uint8_t        sim_i2c_present;
static uint8_t        sim_i2c_addr;
static const uint8_t *sim_i2c_tx;
static uint8_t        sim_i2c_tx_len;
static uint8_t        sim_i2c_tx_idx;
static uint8_t        sim_i2c_expect_addr;
static uint8_t        sim_i2c_addressed;
static uint8_t        sim_i2c_read;
static uint16_t       sim_i2c_log[SIM_LOG_SIZE];
static uint16_t       sim_i2c_log_n;

/* PPSLOCK key sequence: 0 idle, 1 after 0x55, 2 after 0xAA. */
static uint8_t sim_pps_state;

/**
 * @brief Shorthand for one SFR cell.
 * @param addr SFR address.
 * @return Reference to the array cell.
 */
static uint8_t *cell(uint16_t addr)
{
    return &pic16f188xx_sim_sfr[addr];
}

/**
 * @brief Report whether the EUSART transmitter is enabled (TXEN and SPEN).
 * @return 1 when it can shift, 0 otherwise.
 */
static uint8_t sim_usart_tx_on(void)
{
    return (((*cell(PIC_REG_TX1STA) & PIC_TX1STA_TXEN) != 0U) &&
            ((*cell(PIC_REG_RC1STA) & PIC_RC1STA_SPEN) != 0U)) ? 1U : 0U;
}

/**
 * @brief Instruction ticks per EUSART bit from the programmed divisors.
 * @details Baud = Fosc / (D * (n + 1)) with D 64, 16 or 4, and one tick is
 * four Fosc clocks, so a bit lasts D * (n + 1) / 4 ticks, at least one.
 * @return Ticks per bit.
 */
static uint32_t sim_usart_bit_ticks(void)
{
    uint8_t txsta = *cell(PIC_REG_TX1STA);
    uint8_t baud = *cell(PIC_REG_BAUD1CON);
    uint32_t n = *cell(PIC_REG_SP1BRGL);
    uint32_t div;
    uint32_t ticks;

    if ((baud & PIC_BAUD1CON_BRG16) != 0U)
    {
        n |= (uint32_t)(*cell(PIC_REG_SP1BRGH)) << 8;
        div = ((txsta & PIC_TX1STA_BRGH) != 0U) ? 4UL : 16UL;
    }
    else
    {
        div = ((txsta & PIC_TX1STA_BRGH) != 0U) ? 16UL : 64UL;
    }
    ticks = (div * (n + 1UL)) / 4UL;
    return (ticks == 0U) ? 1UL : ticks;
}

/**
 * @brief Refresh the read-only transmitter status: TRMT and PIR3.TXIF.
 * @details TRMT is set while the shift register is empty. TXIF is set
 * while the transmitter is on and TX1REG can take a byte.
 */
static void sim_usart_flags(void)
{
    if (sim_tx_busy == 0U)
    {
        *cell(PIC_REG_TX1STA) |= PIC_TX1STA_TRMT;
    }
    else
    {
        *cell(PIC_REG_TX1STA) &= (uint8_t)~PIC_TX1STA_TRMT;
    }
    if ((sim_usart_tx_on() != 0U) && (sim_tx_full == 0U))
    {
        *cell(PIC_REG_PIR3) |= PIC_PIR3_TXIF;
    }
    else
    {
        *cell(PIC_REG_PIR3) &= (uint8_t)~PIC_PIR3_TXIF;
    }
}

/**
 * @brief Mirror the receive FIFO head into RC1REG, RC1STA.RX9D and PIR3.RCIF.
 */
static void sim_rx_sync(void)
{
    uint8_t rcsta = (uint8_t)(*cell(PIC_REG_RC1STA) & (uint8_t)~PIC_RC1STA_RX9D);

    if (sim_rx_n > 0U)
    {
        *cell(PIC_REG_RC1REG) = (uint8_t)sim_rx_fifo[0];
        if ((sim_rx_fifo[0] & 0x100U) != 0U)
        {
            rcsta |= PIC_RC1STA_RX9D;
        }
        *cell(PIC_REG_PIR3) |= PIC_PIR3_RCIF;
    }
    else
    {
        *cell(PIC_REG_PIR3) &= (uint8_t)~PIC_PIR3_RCIF;
    }
    *cell(PIC_REG_RC1STA) = rcsta;
}

/**
 * @brief Instruction ticks per SCK or SCL period for the selected SSPM mode.
 * @return Ticks per bit, at least one.
 */
static uint32_t sim_ssp_period(void)
{
    switch (*cell(PIC_REG_SSP1CON1) & PIC_SSP1CON1_SSPM_MASK)
    {
    case SIM_SSPM_SPI_FOSC_4:  return 1UL;
    case SIM_SSPM_SPI_FOSC_16: return 4UL;
    case SIM_SSPM_SPI_FOSC_64: return 16UL;
    default:                   return (uint32_t)(*cell(PIC_REG_SSP1ADD)) + 1UL;
    }
}

/**
 * @brief Report whether MSSP1 is enabled in an SPI master mode.
 * @return 1 for SSPM 0, 1, 2 or 0xA with SSPEN set, else 0.
 */
static uint8_t sim_ssp_is_spi(void)
{
    uint8_t con1 = *cell(PIC_REG_SSP1CON1);
    uint8_t mode = (uint8_t)(con1 & PIC_SSP1CON1_SSPM_MASK);

    return (((con1 & PIC_SSP1CON1_SSPEN) != 0U) &&
            ((mode == SIM_SSPM_SPI_FOSC_4) || (mode == SIM_SSPM_SPI_FOSC_16) ||
             (mode == SIM_SSPM_SPI_FOSC_64) || (mode == SIM_SSPM_SPI_ADD))) ? 1U : 0U;
}

/**
 * @brief Report whether MSSP1 is enabled as an I2C master.
 * @return 1 for SSPM 8 with SSPEN set, else 0.
 */
static uint8_t sim_ssp_is_i2c(void)
{
    uint8_t con1 = *cell(PIC_REG_SSP1CON1);

    return (((con1 & PIC_SSP1CON1_SSPEN) != 0U) &&
            ((con1 & PIC_SSP1CON1_SSPM_MASK) == SIM_SSPM_I2C_MASTER)) ? 1U : 0U;
}

/**
 * @brief Append one I2C bus event to the trace, dropping it when the log is full.
 * @param event Event code (PIC16F188XX_SIM_I2C_*, low byte is the payload).
 */
static void sim_i2c_note(uint16_t event)
{
    if (sim_i2c_log_n < SIM_LOG_SIZE)
    {
        sim_i2c_log[sim_i2c_log_n++] = event;
    }
}

/**
 * @brief Start a byte write on SSP1BUF: an SPI shift or an I2C address/data byte.
 * @details A write while an operation runs, or an I2C write while a
 * SSP1CON2 operation is pending, raises WCOL and sends nothing.
 * @param value Byte written.
 */
static void sim_ssp_write(uint8_t value)
{
    if (sim_ssp_is_spi() != 0U)
    {
        if (sim_op != SIM_OP_IDLE)
        {
            *cell(PIC_REG_SSP1CON1) |= PIC_SSP1CON1_WCOL;
            return;
        }
        sim_op = SIM_OP_SPI;
        sim_op_data = value;
        sim_op_left = 8UL * sim_ssp_period();
    }
    else if (sim_ssp_is_i2c() != 0U)
    {
        if ((sim_op != SIM_OP_IDLE) ||
            ((*cell(PIC_REG_SSP1CON2) & PIC_SSP1CON2_OP_MASK) != 0U))
        {
            *cell(PIC_REG_SSP1CON1) |= PIC_SSP1CON1_WCOL;
            return;
        }
        sim_op = SIM_OP_WRITE;
        sim_op_data = value;
        sim_op_left = 9UL * sim_ssp_period();
        *cell(PIC_REG_SSP1STAT) |= (uint8_t)(PIC_SSP1STAT_RW | PIC_SSP1STAT_BF);
    }
}

/**
 * @brief Write an SFR through the simulator, applying write side effects.
 * @details TX1REG load, SSP1BUF transfer start and the PPSLOCK key
 * sequence are actions; TX1STA, RC1STA, BAUD1CON, SSP1STAT, SSP1CON1 and
 * SSP1CON2 keep their read-only or clear-only bits; PIR3 keeps TXIF and
 * RCIF. The NVM and NCO registers go to pic16f188xx_sim_sys_write; every
 * other address is a plain store.
 * @param addr SFR address.
 * @param value Value written.
 */
void pic16f188xx_sim_sfr_write8(uint16_t addr, uint8_t value)
{
    uint8_t old = *cell(addr);

    switch (addr)
    {
    case PIC_REG_TX1REG:
        *cell(PIC_REG_TX1REG) = value;
        sim_tx_hold = value;
        if (((*cell(PIC_REG_TX1STA) & PIC_TX1STA_TX9) != 0U) &&
            ((*cell(PIC_REG_TX1STA) & PIC_TX1STA_TX9D) != 0U))
        {
            sim_tx_hold |= 0x100U;
        }
        sim_tx_full = 1U;
        sim_usart_flags();
        break;
    case PIC_REG_TX1STA:
        *cell(addr) = (uint8_t)((value & (uint8_t)~PIC_TX1STA_TRMT) |
                                (old & PIC_TX1STA_TRMT));
        sim_usart_flags();
        break;
    case PIC_REG_RC1STA:
    {
        uint8_t ro = (uint8_t)(PIC_RC1STA_RX9D | PIC_RC1STA_OERR | PIC_RC1STA_FERR);
        uint8_t nv = (uint8_t)((value & (uint8_t)~ro) | (old & ro));

        if ((value & PIC_RC1STA_SPEN) == 0U)
        {
            sim_rx_n = 0U;
            nv &= (uint8_t)~ro;
        }
        if ((value & PIC_RC1STA_CREN) == 0U)
        {
            nv &= (uint8_t)~PIC_RC1STA_OERR;
        }
        *cell(addr) = nv;
        sim_rx_sync();
        sim_usart_flags();
        break;
    }
    case PIC_REG_BAUD1CON:
        *cell(addr) = (uint8_t)((value & 0x3FU) | (old & 0xC0U));
        break;
    case PIC_REG_PIR3:
        *cell(addr) = (uint8_t)((value & (uint8_t)~(PIC_PIR3_TXIF | PIC_PIR3_RCIF)) |
                                (old & (PIC_PIR3_TXIF | PIC_PIR3_RCIF)));
        break;
    case PIC_REG_SSP1BUF:
        sim_ssp_write(value);
        break;
    case PIC_REG_SSP1STAT:
        *cell(addr) = (uint8_t)((value & PIC_SSP1STAT_WRITE_MASK) |
                                (old & (uint8_t)~PIC_SSP1STAT_WRITE_MASK));
        break;
    case PIC_REG_SSP1CON1:
    {
        uint8_t clr = (uint8_t)(PIC_SSP1CON1_WCOL | PIC_SSP1CON1_SSPOV);

        *cell(addr) = (uint8_t)((value & (uint8_t)~clr) | (old & value & clr));
        if ((value & PIC_SSP1CON1_SSPEN) == 0U)
        {
            sim_op = SIM_OP_IDLE;
            *cell(PIC_REG_SSP1STAT) &= PIC_SSP1STAT_WRITE_MASK;
            *cell(PIC_REG_SSP1CON2) &= (uint8_t)~PIC_SSP1CON2_OP_MASK;
        }
        break;
    }
    case PIC_REG_SSP1CON2:
        *cell(addr) = (uint8_t)((value & (uint8_t)~PIC_SSP1CON2_ACKSTAT) |
                                (old & PIC_SSP1CON2_ACKSTAT));
        break;
    case PIC_REG_PPSLOCK:
        if ((sim_pps_state == 0U) && (value == 0x55U))
        {
            sim_pps_state = 1U;
        }
        else if ((sim_pps_state == 1U) && (value == 0xAAU))
        {
            sim_pps_state = 2U;
        }
        else
        {
            if (sim_pps_state == 2U)
            {
                *cell(addr) = (uint8_t)(value & PIC_PPSLOCK_PPSLOCKED);
            }
            sim_pps_state = 0U;
        }
        break;
    default:
        if (pic16f188xx_sim_sys_write(addr, value) == 0U)
        {
            *cell(addr) = value;
        }
        break;
    }
}

/**
 * @brief Read an SFR through the simulator, applying read side effects.
 * @details An RC1REG read pops the receive FIFO; an SSP1BUF read clears BF.
 * @param addr SFR address.
 * @return The register value.
 */
uint8_t pic16f188xx_sim_sfr_read8(uint16_t addr)
{
    uint8_t value = *cell(addr);

    if (addr == PIC_REG_RC1REG)
    {
        if (sim_rx_n > 0U)
        {
            sim_rx_fifo[0] = sim_rx_fifo[1];
            sim_rx_n--;
            sim_rx_sync();
        }
    }
    else if (addr == PIC_REG_SSP1BUF)
    {
        *cell(PIC_REG_SSP1STAT) &= (uint8_t)~PIC_SSP1STAT_BF;
    }
    return value;
}

/**
 * @brief Clear the serial peripheral state and load the POR values of its SFRs.
 * @details TX1STA powers up 0x02 (TRMT), BAUD1CON 0x40 (RCIDL), SSP1MSK
 * 0xFF, and the PPS input selects their EDC POR pins; every other register
 * is already zero.
 */
void pic16f188xx_sim_serial_reset(void)
{
    sim_tx_full = 0U;
    sim_tx_hold = 0U;
    sim_tx_busy = 0U;
    sim_tx_tsr = 0U;
    sim_tx_left = 0U;
    sim_tx_log_n = 0U;
    sim_rx_n = 0U;
    sim_op = SIM_OP_IDLE;
    sim_op_left = 0U;
    sim_op_data = 0U;
    sim_spi_miso = 0U;
    sim_spi_loop = 0U;
    sim_spi_log_n = 0U;
    sim_i2c_present = 0U;
    sim_i2c_addr = 0U;
    sim_i2c_tx = NULL;
    sim_i2c_tx_len = 0U;
    sim_i2c_tx_idx = 0U;
    sim_i2c_expect_addr = 0U;
    sim_i2c_addressed = 0U;
    sim_i2c_read = 0U;
    sim_i2c_log_n = 0U;
    sim_pps_state = 0U;
    *cell(PIC_REG_TX1STA) = PIC_TX1STA_POR_VALUE;
    *cell(PIC_REG_BAUD1CON) = PIC_BAUD1CON_RCIDL;
    *cell(PIC_REG_SSP1MSK) = 0xFFU;
    *cell(PIC_REG_T0CKIPPS) = 0x04U;
    *cell(PIC_REG_SSP1CLKPPS) = 0x13U;
    *cell(PIC_REG_SSP1DATPPS) = 0x14U;
    *cell(PIC_REG_SSP1SSPPS) = 0x05U;
    *cell(PIC_REG_RXPPS) = 0x17U;
}

/**
 * @brief Finish the running MSSP operation and apply its bus effects.
 */
static void sim_ssp_complete(void)
{
    uint8_t ack;

    switch (sim_op)
    {
    case SIM_OP_SPI:
        if (sim_spi_log_n < SIM_LOG_SIZE)
        {
            sim_spi_log[sim_spi_log_n++] = sim_op_data;
        }
        if ((*cell(PIC_REG_SSP1STAT) & PIC_SSP1STAT_BF) != 0U)
        {
            *cell(PIC_REG_SSP1CON1) |= PIC_SSP1CON1_SSPOV;
        }
        else
        {
            *cell(PIC_REG_SSP1BUF) = (sim_spi_loop != 0U) ? sim_op_data : sim_spi_miso;
            *cell(PIC_REG_SSP1STAT) |= PIC_SSP1STAT_BF;
        }
        break;
    case SIM_OP_START:
    case SIM_OP_RESTART:
        *cell(PIC_REG_SSP1STAT) = (uint8_t)((*cell(PIC_REG_SSP1STAT) | PIC_SSP1STAT_S) &
                                            (uint8_t)~PIC_SSP1STAT_P);
        *cell(PIC_REG_SSP1CON2) &= (uint8_t)~((sim_op == SIM_OP_START) ?
            PIC_SSP1CON2_SEN : PIC_SSP1CON2_RSEN);
        sim_i2c_note((sim_op == SIM_OP_START) ? PIC16F188XX_SIM_I2C_START :
                                                PIC16F188XX_SIM_I2C_RESTART);
        sim_i2c_expect_addr = 1U;
        sim_i2c_addressed = 0U;
        break;
    case SIM_OP_STOP:
        *cell(PIC_REG_SSP1STAT) = (uint8_t)((*cell(PIC_REG_SSP1STAT) | PIC_SSP1STAT_P) &
                                            (uint8_t)~PIC_SSP1STAT_S);
        *cell(PIC_REG_SSP1CON2) &= (uint8_t)~PIC_SSP1CON2_PEN;
        sim_i2c_note(PIC16F188XX_SIM_I2C_STOP);
        sim_i2c_expect_addr = 0U;
        sim_i2c_addressed = 0U;
        break;
    case SIM_OP_WRITE:
        ack = 0U;
        if (sim_i2c_expect_addr != 0U)
        {
            sim_i2c_expect_addr = 0U;
            sim_i2c_note((uint16_t)(PIC16F188XX_SIM_I2C_ADDRESS | sim_op_data));
            if ((sim_i2c_present != 0U) && ((sim_op_data >> 1) == sim_i2c_addr))
            {
                sim_i2c_addressed = 1U;
                sim_i2c_read = (uint8_t)(sim_op_data & 1U);
                sim_i2c_tx_idx = 0U;
                ack = 1U;
            }
            else
            {
                sim_i2c_addressed = 0U;
            }
        }
        else
        {
            sim_i2c_note((uint16_t)(PIC16F188XX_SIM_I2C_DATA | sim_op_data));
            ack = ((sim_i2c_addressed != 0U) && (sim_i2c_read == 0U)) ? 1U : 0U;
        }
        if (ack != 0U)
        {
            *cell(PIC_REG_SSP1CON2) &= (uint8_t)~PIC_SSP1CON2_ACKSTAT;
        }
        else
        {
            *cell(PIC_REG_SSP1CON2) |= PIC_SSP1CON2_ACKSTAT;
        }
        *cell(PIC_REG_SSP1STAT) &= (uint8_t)~(PIC_SSP1STAT_RW | PIC_SSP1STAT_BF);
        break;
    case SIM_OP_RECEIVE:
    {
        uint8_t byte = 0xFFU;

        if ((sim_i2c_addressed != 0U) && (sim_i2c_read != 0U) &&
            (sim_i2c_tx_idx < sim_i2c_tx_len))
        {
            byte = sim_i2c_tx[sim_i2c_tx_idx++];
        }
        if ((*cell(PIC_REG_SSP1STAT) & PIC_SSP1STAT_BF) != 0U)
        {
            *cell(PIC_REG_SSP1CON1) |= PIC_SSP1CON1_SSPOV;
        }
        else
        {
            *cell(PIC_REG_SSP1BUF) = byte;
            *cell(PIC_REG_SSP1STAT) |= PIC_SSP1STAT_BF;
        }
        *cell(PIC_REG_SSP1CON2) &= (uint8_t)~PIC_SSP1CON2_RCEN;
        sim_i2c_note((uint16_t)(PIC16F188XX_SIM_I2C_READ | byte));
        break;
    }
    case SIM_OP_ACK:
        sim_i2c_note((uint16_t)(PIC16F188XX_SIM_I2C_ACK |
            (((*cell(PIC_REG_SSP1CON2) & PIC_SSP1CON2_ACKDT) != 0U) ? 1U : 0U)));
        *cell(PIC_REG_SSP1CON2) &= (uint8_t)~PIC_SSP1CON2_ACKEN;
        break;
    default:
        break;
    }
    *cell(PIC_REG_PIR3) |= PIC_PIR3_SSP1IF;
    sim_op = SIM_OP_IDLE;
}

/**
 * @brief Pick the next pending SSP1CON2 operation and start its countdown.
 * @details Priority is SEN, RSEN, PEN, RCEN, ACKEN. A receive lasts eight
 * SCL periods, every other operation one.
 */
static void sim_i2c_pick(void)
{
    uint8_t con2 = *cell(PIC_REG_SSP1CON2);
    uint32_t period = sim_ssp_period();

    if ((con2 & PIC_SSP1CON2_SEN) != 0U)
    {
        sim_op = SIM_OP_START;
        sim_op_left = period;
    }
    else if ((con2 & PIC_SSP1CON2_RSEN) != 0U)
    {
        sim_op = SIM_OP_RESTART;
        sim_op_left = period;
    }
    else if ((con2 & PIC_SSP1CON2_PEN) != 0U)
    {
        sim_op = SIM_OP_STOP;
        sim_op_left = period;
    }
    else if ((con2 & PIC_SSP1CON2_RCEN) != 0U)
    {
        sim_op = SIM_OP_RECEIVE;
        sim_op_left = 8UL * period;
    }
    else if ((con2 & PIC_SSP1CON2_ACKEN) != 0U)
    {
        sim_op = SIM_OP_ACK;
        sim_op_left = period;
    }
}

/**
 * @brief Advance the EUSART transmitter and the MSSP engine by one tick.
 * @details The transmitter frames start, 8 or 9 data bits and stop; a
 * finished frame lands in the TX log. The MSSP counts its running
 * operation down and completes it, setting SSP1IF.
 */
void pic16f188xx_sim_serial_step(void)
{
    if (sim_usart_tx_on() == 0U)
    {
        sim_tx_busy = 0U;
        sim_tx_full = 0U;
    }
    else
    {
        if ((sim_tx_busy == 0U) && (sim_tx_full != 0U))
        {
            uint32_t bits = ((*cell(PIC_REG_TX1STA) & PIC_TX1STA_TX9) != 0U) ? 11UL : 10UL;
            uint32_t per_bit = sim_usart_bit_ticks();

            sim_tx_busy = 1U;
            sim_tx_full = 0U;
            sim_tx_tsr = sim_tx_hold;
            sim_tx_left = per_bit * bits;
        }
        if (sim_tx_busy != 0U)
        {
            sim_tx_left--;
            if (sim_tx_left == 0U)
            {
                if (sim_tx_log_n < SIM_LOG_SIZE)
                {
                    sim_tx_log[sim_tx_log_n++] = sim_tx_tsr;
                }
                sim_tx_busy = 0U;
            }
        }
    }
    sim_usart_flags();

    if ((sim_ssp_is_spi() == 0U) && (sim_ssp_is_i2c() == 0U))
    {
        sim_op = SIM_OP_IDLE;
        return;
    }
    if ((sim_op == SIM_OP_IDLE) && (sim_ssp_is_i2c() != 0U))
    {
        sim_i2c_pick();
    }
    if (sim_op != SIM_OP_IDLE)
    {
        sim_op_left--;
        if (sim_op_left == 0U)
        {
            sim_ssp_complete();
        }
    }
}

/**
 * @brief Deliver a received frame to the EUSART as if it arrived on RX.
 * @param frame Data byte; bit 8 is the ninth bit in 9-bit mode.
 * @return 1 when accepted, 0 when the receiver is off, in overrun, or the
 *         frame is filtered by address detect or dropped for a full FIFO
 *         (a third byte also raises OERR).
 */
uint8_t pic16f188xx_sim_usart_rx_inject(uint16_t frame)
{
    uint8_t rcsta = *cell(PIC_REG_RC1STA);

    if (((rcsta & PIC_RC1STA_SPEN) == 0U) || ((rcsta & PIC_RC1STA_CREN) == 0U) ||
        ((rcsta & PIC_RC1STA_OERR) != 0U))
    {
        return 0U;
    }
    if ((rcsta & PIC_RC1STA_RX9) == 0U)
    {
        frame &= 0xFFU;
    }
    else if (((rcsta & PIC_RC1STA_ADDEN) != 0U) && ((frame & 0x100U) == 0U))
    {
        return 0U;
    }
    if (sim_rx_n >= SIM_RX_FIFO_DEPTH)
    {
        *cell(PIC_REG_RC1STA) |= PIC_RC1STA_OERR;
        return 0U;
    }
    sim_rx_fifo[sim_rx_n++] = frame;
    sim_rx_sync();
    return 1U;
}

/**
 * @brief Count the frames the transmitter has shifted out.
 * @return Frames in the TX log (at most 64).
 */
uint16_t pic16f188xx_sim_usart_tx_count(void)
{
    return sim_tx_log_n;
}

/**
 * @brief Read one shifted-out frame from the TX log.
 * @param index Frame index, 0 is the oldest.
 * @return The frame (bit 8 is TX9D), or 0 for an index past the count.
 */
uint16_t pic16f188xx_sim_usart_tx_frame(uint16_t index)
{
    return (index < sim_tx_log_n) ? sim_tx_log[index] : 0U;
}

/**
 * @brief Empty the TX log.
 */
void pic16f188xx_sim_usart_tx_clear(void)
{
    sim_tx_log_n = 0U;
}

/**
 * @brief Set the byte an SPI transfer shifts in when loopback is off.
 * @param byte MISO byte.
 */
void pic16f188xx_sim_spi_set_miso(uint8_t byte)
{
    sim_spi_miso = byte;
}

/**
 * @brief Tie SDI to SDO: each transfer reads back the byte it sent.
 * @param on Nonzero enables loopback, zero returns to the MISO byte.
 */
void pic16f188xx_sim_spi_set_loopback(uint8_t on)
{
    sim_spi_loop = on;
}

/**
 * @brief Count the SPI bytes the master has shifted out.
 * @return Bytes in the MOSI log (at most 64).
 */
uint16_t pic16f188xx_sim_spi_count(void)
{
    return sim_spi_log_n;
}

/**
 * @brief Read one shifted-out SPI byte from the MOSI log.
 * @param index Byte index, 0 is the oldest.
 * @return The byte, or 0 for an index past the count.
 */
uint8_t pic16f188xx_sim_spi_mosi(uint16_t index)
{
    return (index < sim_spi_log_n) ? sim_spi_log[index] : 0U;
}

/**
 * @brief Attach a scripted I2C slave to the bus.
 * @param addr7 Seven-bit slave address.
 * @param tx Bytes the slave returns to the master, or NULL for none.
 * @param tx_len Number of bytes in tx; reads past it return 0xFF.
 */
void pic16f188xx_sim_i2c_set_slave(uint8_t addr7, const uint8_t *tx, uint8_t tx_len)
{
    sim_i2c_present = 1U;
    sim_i2c_addr = (uint8_t)(addr7 & 0x7FU);
    sim_i2c_tx = tx;
    sim_i2c_tx_len = (tx == NULL) ? 0U : tx_len;
    sim_i2c_tx_idx = 0U;
}

/**
 * @brief Empty the I2C bus trace.
 */
void pic16f188xx_sim_i2c_clear_trace(void)
{
    sim_i2c_log_n = 0U;
}

/**
 * @brief Count the events in the I2C bus trace.
 * @return Events recorded (at most 64).
 */
uint16_t pic16f188xx_sim_i2c_trace_count(void)
{
    return sim_i2c_log_n;
}

/**
 * @brief Read one event from the I2C bus trace.
 * @param index Event index, 0 is the oldest.
 * @return The event code (PIC16F188XX_SIM_I2C_*), or 0 past the count.
 */
uint16_t pic16f188xx_sim_i2c_trace(uint16_t index)
{
    return (index < sim_i2c_log_n) ? sim_i2c_log[index] : 0U;
}
