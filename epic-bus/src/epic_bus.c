/**
 * @file    epic_bus.c
 * @brief   I2C/SPI MEM register-access transactions on the MSSP/SSP HAL,
 *          family-neutral via a small "bus ops" interface. The default
 *          ops wrap the HAL's SSP primitives plus a poll for SSPIF (the
 *          HAL's own calls only set control bits and return). The host
 *          sim has no SSP slave model, so `epic_bus_set_i2c_ops`/
 *          `_set_spi_ops` let the host test inject a mock device instead.
 */

#include "epic_bus.h"
#include "epic_hal.h"               /* SSP, GPIO, SFR, IRQ, platform */

#if defined(PIC18F2455) || defined(PIC18F2550) || defined(PIC18F4455) || defined(PIC18F4550)
  #define BUS_IS_PIC18         1
  #define BUS_IRQ_SSP          PIC18_IRQ_SSP
  #define BUS_SSPCON2_READ()   epic_sfr_read8(PIC_REG_SSPCON2)
  #define BUS_SSPCON2_WRITE(c) epic_sfr_write8(PIC_REG_SSPCON2, (uint8_t)(c))
#else
  #define BUS_IS_PIC18         0
  #define BUS_IRQ_SSP          PIC16_IRQ_SSP
  #define BUS_SSPCON2_READ()   PIC8_REG8(PIC_REG_SSPCON2)
  #define BUS_SSPCON2_WRITE(c) (PIC8_REG8(PIC_REG_SSPCON2) = (uint8_t)(c))
#endif

/* ─── default I2C ops (HAL SSP + ACKDT + SSPIF wait) ────────────── */
static void i2c_wait_ssp(void)
{
    while (!EPIC_IRQ_GetFlag(BUS_IRQ_SSP)) { }   /* block until the SSP op completes */
    EPIC_IRQ_ClearFlag(BUS_IRQ_SSP);
}

static void i2c_set_ackdt(int ack)   /* ack=1 -> ACK (ACKDT=0); ack=0 -> NACK (ACKDT=1) */
{
    uint8_t c = BUS_SSPCON2_READ();
    if (ack) { c &= (uint8_t)~PIC_SSPCON2_ACKDT; }
    else     { c |= (uint8_t) PIC_SSPCON2_ACKDT; }
    BUS_SSPCON2_WRITE(c);
}

static void i2c_real_start(void)          { EPIC_SSP_Start();          i2c_wait_ssp(); }
static void i2c_real_repeated_start(void) { EPIC_SSP_RepeatedStart();  i2c_wait_ssp(); }
static void i2c_real_stop(void)           { EPIC_SSP_Stop();           i2c_wait_ssp(); }
static int  i2c_real_write_byte(uint8_t b)
{
    (void)EPIC_SSP_WriteByte(b);
    i2c_wait_ssp();
    return (EPIC_SSP_AcknowledgeStatus() == 0u) ? 1 : 0;   /* ACKSTAT=0 -> ACK */
}
static uint8_t i2c_real_read_byte(int ack)
{
    i2c_set_ackdt(ack);
    EPIC_SSP_ReceiveEnable();
    i2c_wait_ssp();
    uint8_t b = EPIC_SSP_ReadByte();
    EPIC_SSP_AcknowledgeEnable();
    i2c_wait_ssp();
    return b;
}

static const epic_bus_i2c_ops_t g_i2c_default = {
    i2c_real_start, i2c_real_repeated_start, i2c_real_stop,
    i2c_real_write_byte, i2c_real_read_byte
};
static const epic_bus_i2c_ops_t *g_i2c_ops = &g_i2c_default;

void epic_bus_set_i2c_ops(const epic_bus_i2c_ops_t *ops)
{
    g_i2c_ops = ops ? ops : &g_i2c_default;
}

void epic_bus_i2c_init(uint32_t fosc_hz, uint32_t fscl_hz)
{
    static SSP_HandleTypeDef s_ssp;        /* static: Init may store the pointer */
    SSP_HandleTypeDef h = SSP_HANDLE_DEFAULT;
    h.Mode   = SSP_MODE_I2C_MASTER_FOSC;
    h.SSPADD = (uint8_t)SSP_ComputeSSPADD(fosc_hz, fscl_hz);
    s_ssp = h;
    EPIC_SSP_Init(&s_ssp);
    g_i2c_ops = &g_i2c_default;
}

/* ─── default SPI ops (HAL SSP exchange + GPIO CS) ──────────────── */
static uint8_t s_cs_port;
static uint8_t s_cs_pin;

static void spi_real_select(void)
{
    EPIC_GPIO_WritePin((GPIO_TypeDef)s_cs_port, (uint16_t)PIC8_BIT(s_cs_pin), GPIO_PIN_RESET);
}
static void spi_real_deselect(void)
{
    EPIC_GPIO_WritePin((GPIO_TypeDef)s_cs_port, (uint16_t)PIC8_BIT(s_cs_pin), GPIO_PIN_SET);
}
static uint8_t spi_real_exchange(uint8_t b)
{
    (void)EPIC_SSP_WriteByte(b);
    while (!EPIC_SSP_IsBufferFull()) { }    /* wait for the shift to complete */
    return EPIC_SSP_ReadByte();
}

static const epic_bus_spi_ops_t g_spi_default = {
    spi_real_select, spi_real_deselect, spi_real_exchange
};
static const epic_bus_spi_ops_t *g_spi_ops = &g_spi_default;

void epic_bus_set_spi_ops(const epic_bus_spi_ops_t *ops)
{
    g_spi_ops = ops ? ops : &g_spi_default;
}

void epic_bus_spi_init(uint32_t fosc_hz, uint32_t f_sclk_hz, uint8_t cs_port, uint8_t cs_pin)
{
    static SSP_HandleTypeDef s_ssp;
    SSP_HandleTypeDef h = SSP_HANDLE_DEFAULT;
    /* Pick the standard SPI clock divider closest to f_sclk_hz. */
    SSP_ModeTypeDef mode;
    if (f_sclk_hz == 0u || f_sclk_hz >= (fosc_hz / 8u))      mode = SSP_MODE_SPI_MASTER_FOSC_4;
    else if (f_sclk_hz >= (fosc_hz / 32u))                   mode = SSP_MODE_SPI_MASTER_FOSC_16;
    else                                                     mode = SSP_MODE_SPI_MASTER_FOSC_64;
    h.Mode = mode;
    s_ssp = h;
    EPIC_SSP_Init(&s_ssp);
    s_cs_port = cs_port;
    s_cs_pin  = cs_pin;
    EPIC_GPIO_Init((GPIO_TypeDef)cs_port, (uint16_t)PIC8_BIT(cs_pin), GPIO_MODE_OUTPUT);
    EPIC_GPIO_WritePin((GPIO_TypeDef)cs_port, (uint16_t)PIC8_BIT(cs_pin), GPIO_PIN_SET);
    g_spi_ops = &g_spi_default;
}

/* ─── MEM transactions (family-neutral, via the ops interface) ──── */

int epic_bus_i2c_mem_write(uint8_t dev, uint8_t reg, const uint8_t *data, int n)
{
    const epic_bus_i2c_ops_t *o = g_i2c_ops;
    o->start();
    if (!o->write_byte((uint8_t)((dev << 1) | 0u))) { o->stop(); return -1; }
    if (!o->write_byte(reg))                         { o->stop(); return -1; }
    for (int i = 0; i < n; i++) {
        if (!o->write_byte(data[i])) { o->stop(); return -1; }
    }
    o->stop();
    return n;
}

int epic_bus_i2c_mem_read(uint8_t dev, uint8_t reg, uint8_t *buf, int n)
{
    const epic_bus_i2c_ops_t *o = g_i2c_ops;
    o->start();
    if (!o->write_byte((uint8_t)((dev << 1) | 0u))) { o->stop(); return -1; }
    if (!o->write_byte(reg))                         { o->stop(); return -1; }
    o->repeated_start();
    if (!o->write_byte((uint8_t)((dev << 1) | 1u))) { o->stop(); return -1; }
    for (int i = 0; i < n; i++) {
        buf[i] = o->read_byte(i < (n - 1) ? 1 : 0);   /* ACK all but the last */
    }
    o->stop();
    return n;
}

int epic_bus_spi_mem_write(uint8_t reg, const uint8_t *data, int n)
{
    const epic_bus_spi_ops_t *o = g_spi_ops;
    o->select();
    (void)o->exchange(reg);
    for (int i = 0; i < n; i++) { (void)o->exchange(data[i]); }
    o->deselect();
    return n;
}

int epic_bus_spi_mem_read(uint8_t reg, uint8_t *buf, int n)
{
    const epic_bus_spi_ops_t *o = g_spi_ops;
    o->select();
    (void)o->exchange(reg);
    for (int i = 0; i < n; i++) { buf[i] = o->exchange(0u); }
    o->deselect();
    return n;
}
