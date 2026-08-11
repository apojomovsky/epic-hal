/* Bounded, self-reporting HARNESS=sim `mdb` gate for epic-bus: runs the
 * compiled epic_bus.c under MPLAB SIM on a 16F877A, then reports
 * PASS/FAIL over the target's real USART (pic16_harness_sim_target.c).
 * MPLAB SIM has no slave to inject on the bus (probed: SEN stays
 * latched, SSPIF/BF never set, i.e. the model has the MSSP register
 * file but not the data path), so the gate covers the master side in
 * three parts: (a) real init config readback, (b) bounded real SSP
 * register traffic, (c) MEM transaction logic through the mock ops
 * seam, mirroring examples/example_bus.c. */

#include "epic_bus.h"
#include "core/epic_harness.h"
#include "core/pic16_irq.h"
#include "target/pic16f87xa_platform.h"
#include "pic16f87xa_sfr.h"
#include "peripherals/pic16f87xa_ssp.h"

#include <stdint.h>

#ifndef FOSC_HZ
#define FOSC_HZ 20000000UL
#endif

/** Loop-iteration bound, not a real time unit (see core/epic_harness.h).
 *  All hardware waits below are bounded SSPIF polls, so the post-work
 *  idle loop is only a formality; 2000 is far more than enough. */
#define SIM_ITERATIONS 2000UL

/** Bounded SSPIF poll budget: at 100 kHz SCL one byte takes ~90 us of
 *  simulated time (~450 instruction cycles at 20 MHz/4), so 20000 polls
 *  (~240k cycles) is a wide margin; a model that never completes just
 *  burns the poll loop and reports the failure. */
#define SSP_POLLS 20000UL

#define MOCK_DEV 0x50u
#define I2C_ADDR_BYTE(dev, rd) ((uint8_t)(((dev) << 1) | (rd)))
#define SSPADD_100K 49u   /* 20e6 / (4 * 100e3) - 1 */

/* mock MEM device for the ops-seam phase (mirrors example_bus.c) */

static uint8_t g_reg[16];
static uint8_t g_seq_op[20];
static uint8_t g_seq_val[20];
static uint8_t g_seq_n;

enum { OP_START = 1u, OP_RSTART = 2u, OP_STOP = 3u, OP_WRITE = 4u, OP_READ = 5u };
enum { I_IDLE, I_ADDR, I_REG, I_DATA };
static uint8_t g_i2c_state;
static uint8_t g_i2c_reg;

static void mock_i2c_start(void)          { g_seq_op[g_seq_n] = OP_START;  g_seq_n++; g_i2c_state = I_ADDR; }
static void mock_i2c_repeated_start(void) { g_seq_op[g_seq_n] = OP_RSTART; g_seq_n++; g_i2c_state = I_ADDR; }
static void mock_i2c_stop(void)           { g_seq_op[g_seq_n] = OP_STOP;   g_seq_n++; g_i2c_state = I_IDLE; }
static int  mock_i2c_write_byte(uint8_t b)
{
    g_seq_op[g_seq_n] = OP_WRITE; g_seq_val[g_seq_n] = b; g_seq_n++;
    if (g_i2c_state == I_ADDR) {
        if ((b >> 1) != MOCK_DEV) return 0;          /* wrong device: NACK */
        g_i2c_state = I_REG;
        return 1;
    }
    if (g_i2c_state == I_REG)  { g_i2c_reg = b; g_i2c_state = I_DATA; return 1; }
    if (g_i2c_state == I_DATA) { g_reg[g_i2c_reg & 0x0Fu] = b; g_i2c_reg++; return 1; }
    return 0;
}
static uint8_t mock_i2c_read_byte(int ack)
{
    g_seq_op[g_seq_n] = OP_READ; g_seq_val[g_seq_n] = (uint8_t)ack; g_seq_n++;
    uint8_t b = g_reg[g_i2c_reg & 0x0Fu];
    g_i2c_reg++;
    return b;
}
static const epic_bus_i2c_ops_t mock_i2c = {
    mock_i2c_start, mock_i2c_repeated_start, mock_i2c_stop,
    mock_i2c_write_byte, mock_i2c_read_byte
};

enum { S_IDLE, S_REG, S_XFER };
static uint8_t g_spi_state;
static uint8_t g_spi_reg;
static void mock_spi_select(void)   { g_seq_op[g_seq_n] = OP_START;  g_seq_n++; g_spi_state = S_REG; }
static void mock_spi_deselect(void) { g_seq_op[g_seq_n] = OP_STOP;   g_seq_n++; g_spi_state = S_IDLE; }
static uint8_t mock_spi_exchange(uint8_t b)
{
    g_seq_op[g_seq_n] = OP_WRITE; g_seq_val[g_seq_n] = b; g_seq_n++;
    if (g_spi_state == S_REG) { g_spi_reg = b; g_spi_state = S_XFER; return 0xFFu; }
    uint8_t out = g_reg[g_spi_reg & 0x0Fu];
    g_reg[g_spi_reg & 0x0Fu] = b;   /* write what shifted in (MOSI) to the map */
    g_spi_reg++;
    return out;
}
static const epic_bus_spi_ops_t mock_spi = {
    mock_spi_select, mock_spi_deselect, mock_spi_exchange
};

/* register diagnostics (the sim harness prints raw strings) */

static void log_reg(const char *label, uint8_t v)
{
    static const char hex[] = "0123456789ABCDEF";
    char buf[3];
    buf[0] = hex[(v >> 4) & 0x0Fu];
    buf[1] = hex[v & 0x0Fu];
    buf[2] = '\0';
    epic_harness_log(label);
    epic_harness_log("=0x");
    epic_harness_log(buf);
    epic_harness_log("\n");
}

/* bounded SSPIF poll (the default ops' wait, made bounded) */

static uint8_t ssp_wait(uint32_t polls)
{
    uint32_t i;
    for (i = 0; i < polls; i++) {
        if (EPIC_IRQ_GetFlag(PIC16_IRQ_SSP)) {
            EPIC_IRQ_ClearFlag(PIC16_IRQ_SSP);
            return 1u;
        }
    }
    return 0u;
}

int main(void)
{
    epic_harness_init(SIM_ITERATIONS);

    uint8_t ok = 1u;

    /* (a) real init: I2C master config readback */
    epic_bus_i2c_init(FOSC_HZ, 100000UL);

    uint8_t sspcon = EPIC_REG8(PIC_REG_SSPCON);
    uint8_t sspadd = 0u;
    EPIC_BANK1_READ8(SSPADD, sspadd);
    uint8_t sspstat = 0u;
    EPIC_BANK1_READ8(SSPSTAT, sspstat);

    uint8_t i2c_cfg_ok = (uint8_t)(((sspcon & PIC_SSPCON_SSPM_MASK) == SSP_MODE_I2C_MASTER_FOSC) &&
                                    (sspcon & PIC_SSPCON_SSPEN) &&
                                    (sspadd == SSPADD_100K) &&
                                    ((sspstat & (PIC_SSPSTAT_BF | PIC_SSPSTAT_UA)) == 0u));
    if (i2c_cfg_ok) { epic_harness_log("bus sim: i2c master config ok\n"); }
    else            { epic_harness_log("bus sim: i2c master config WRONG\n"); }
    ok &= i2c_cfg_ok;

    /* (b) real SSP register traffic (bounded) */
    /* Start condition: SEN is latched by the hardware write. Whether
     * it ever completes is MPLAB SIM model behavior, logged below as
     * evidence (probed: SEN stays 1, SSPIF stays 0, BF stays 0 for
     * both I2C and SPI - this sim does not model the MSSP data path
     * at all), so the advance is diagnostic only. */
    EPIC_SSP_Start();
    uint8_t st_adv = ssp_wait(SSP_POLLS);
    EPIC_BANK1_READ8(SSPCON2, sspcon);
    log_reg("bus sim: after start SSPCON2", sspcon);
    log_reg("bus sim: after start PIR1", EPIC_REG8(PIC_REG_PIR1));

    /* Byte: the written byte must land in SSPBUF with no write
     * collision (WCOL). BF/SSPIF state is logged as model evidence. */
    EPIC_IRQ_ClearFlag(PIC16_IRQ_SSP);
    uint16_t wrc = EPIC_SSP_WriteByte(0xA0u);
    uint8_t wr_adv = ssp_wait(SSP_POLLS);
    uint8_t sspbuf = EPIC_REG8(PIC_REG_SSPBUF);
    EPIC_BANK1_READ8(SSPSTAT, sspstat);
    log_reg("bus sim: after write SSPBUF", sspbuf);
    log_reg("bus sim: after write SSPCON2", sspcon);
    log_reg("bus sim: after write SSPSTAT", sspstat);
    log_reg("bus sim: after write PIR1", EPIC_REG8(PIC_REG_PIR1));
    uint8_t land_ok = (uint8_t)(wrc == 0u && sspbuf == 0xA0u);

    /* Stop: PEN is latched; completion is diagnostic only. */
    (void)EPIC_SSP_ReadByte();                 /* clear BF before the stop */
    EPIC_IRQ_ClearFlag(PIC16_IRQ_SSP);
    EPIC_SSP_Stop();
    uint8_t sp_adv = ssp_wait(SSP_POLLS);
    EPIC_BANK1_READ8(SSPCON2, sspcon);
    log_reg("bus sim: after stop SSPCON2", sspcon);

    if (st_adv && wr_adv && sp_adv) {
        epic_harness_log("bus sim: ssp state machine advanced\n");
    } else {
        epic_harness_log("bus sim: ssp state machine NOT modeled by MPLAB SIM\n");
    }
    if (land_ok) { epic_harness_log("bus sim: i2c byte landed in SSPBUF\n"); }
    else         { epic_harness_log("bus sim: i2c byte did NOT land in SSPBUF\n"); }
    ok &= land_ok;

    /* Full mem_write through the DEFAULT (real SSP) ops, only when the
     * model just proved it completes every phase (the default ops' own
     * SSPIF waits are unbounded; this guard keeps the gate bounded).
     * Documented contract: n on success, -1 on address NACK, with the
     * return value and ACKSTAT required to agree. */
    if (st_adv && wr_adv && sp_adv) {
        uint8_t wr[2] = { 0x11u, 0x22u };
        int n = epic_bus_i2c_mem_write(MOCK_DEV, 0x00u, wr, 2);
        EPIC_BANK1_READ8(SSPCON2, sspcon);
        uint8_t ack = (uint8_t)((sspcon & PIC_SSPCON2_ACKSTAT) ? 1u : 0u);
        sspbuf = EPIC_REG8(PIC_REG_SSPBUF);
        uint8_t ctrl_idle = (uint8_t)((sspcon & (PIC_SSPCON2_SEN | PIC_SSPCON2_RSEN |
                                                 PIC_SSPCON2_PEN | PIC_SSPCON2_RCEN |
                                                 PIC_SSPCON2_ACKEN)) == 0u);
        /* ACKed: return 2 and SSPBUF holds the last data byte; NACKed
         * at the address: return -1 and SSPBUF holds the address byte. */
        uint8_t mem_ok = 0u;
        if (n == 2 && ack == 0u && sspbuf == 0x22u)      { mem_ok = 1u; }
        else if (n == -1 && ack == 1u && sspbuf == 0xA0u) { mem_ok = 1u; }
        mem_ok = (uint8_t)(mem_ok && ctrl_idle);
        if (mem_ok) { epic_harness_log("bus sim: default-ops mem_write ok\n"); }
        else        { epic_harness_log("bus sim: default-ops mem_write MISMATCH\n"); }
        ok &= mem_ok;
    } else {
        epic_harness_log("bus sim: default-ops mem_write skipped (no SSP model)\n");
    }

    /* (c) transaction logic through the ops seam */
    epic_bus_set_i2c_ops(&mock_i2c);
    g_seq_n = 0u;
    for (uint8_t i = 0; i < 16u; i++) { g_reg[i] = (uint8_t)(0x10u + i); }

    uint8_t buf[4];
    int n = epic_bus_i2c_mem_read(MOCK_DEV, 0x00u, buf, 4);
    uint8_t rd_ok = (uint8_t)(n == 4 && buf[0] == 0x10u && buf[1] == 0x11u &&
                              buf[2] == 0x12u && buf[3] == 0x13u &&
                              g_seq_n == 10u &&
                              g_seq_op[0] == OP_START && g_seq_val[0] == 0u &&
                              g_seq_op[1] == OP_WRITE && g_seq_val[1] == I2C_ADDR_BYTE(MOCK_DEV, 0u) &&
                              g_seq_op[2] == OP_WRITE && g_seq_val[2] == 0x00u &&
                              g_seq_op[3] == OP_RSTART &&
                              g_seq_op[4] == OP_WRITE && g_seq_val[4] == I2C_ADDR_BYTE(MOCK_DEV, 1u) &&
                              g_seq_op[5] == OP_READ && g_seq_val[5] == 1u &&
                              g_seq_op[6] == OP_READ && g_seq_val[6] == 1u &&
                              g_seq_op[7] == OP_READ && g_seq_val[7] == 1u &&
                              g_seq_op[8] == OP_READ && g_seq_val[8] == 0u);
    if (rd_ok) { epic_harness_log("bus sim: i2c mem_read seq ok\n"); }
    else       { epic_harness_log("bus sim: i2c mem_read seq WRONG\n"); }
    ok &= rd_ok;

    uint8_t wd[3] = { 0xA0u, 0xA1u, 0xA2u };
    g_seq_n = 0u;
    n = epic_bus_i2c_mem_write(MOCK_DEV, 0x05u, wd, 3);
    uint8_t wr2_ok = (uint8_t)(n == 3 && g_reg[5] == 0xA0u && g_reg[6] == 0xA1u &&
                               g_reg[7] == 0xA2u && g_seq_n == 7u &&
                               g_seq_op[0] == OP_START &&
                               g_seq_op[1] == OP_WRITE && g_seq_val[1] == I2C_ADDR_BYTE(MOCK_DEV, 0u) &&
                               g_seq_op[2] == OP_WRITE && g_seq_val[2] == 0x05u &&
                               g_seq_op[3] == OP_WRITE && g_seq_val[3] == 0xA0u &&
                               g_seq_op[4] == OP_WRITE && g_seq_val[4] == 0xA1u &&
                               g_seq_op[5] == OP_WRITE && g_seq_val[5] == 0xA2u &&
                               g_seq_op[6] == OP_STOP);
    /* START, W(addr), W(reg), W x3, STOP */
    if (wr2_ok) { epic_harness_log("bus sim: i2c mem_write seq ok\n"); }
    else        { epic_harness_log("bus sim: i2c mem_write seq WRONG\n"); }
    ok &= wr2_ok;

    g_seq_n = 0u;
    n = epic_bus_i2c_mem_write(0x77u, 0x00u, wd, 1);   /* wrong device */
    uint8_t nak_ok = (uint8_t)(n == -1 && g_seq_n == 3u &&
                               g_seq_op[0] == OP_START &&
                               g_seq_op[1] == OP_WRITE && g_seq_val[1] == I2C_ADDR_BYTE(0x77u, 0u) &&
                               g_seq_op[2] == OP_STOP);
    if (nak_ok) { epic_harness_log("bus sim: i2c addr NACK -> -1 ok\n"); }
    else        { epic_harness_log("bus sim: i2c addr NACK WRONG\n"); }
    ok &= nak_ok;

    /* SPI: real init readback + ops-seam transaction logic */
    epic_bus_spi_init(FOSC_HZ, 0UL, 1u, 0u);     /* SPI master, CS = GPIOB0 */
    sspcon = EPIC_REG8(PIC_REG_SSPCON);
    uint8_t spi_cfg_ok = (uint8_t)(((sspcon & PIC_SSPCON_SSPM_MASK) == SSP_MODE_SPI_MASTER_FOSC_4) &&
                                   (sspcon & PIC_SSPCON_SSPEN));
    if (spi_cfg_ok) { epic_harness_log("bus sim: spi master config ok\n"); }
    else            { epic_harness_log("bus sim: spi master config WRONG\n"); }
    ok &= spi_cfg_ok;

    /* Real SPI register traffic (bounded BF poll, same guard as the
     * I2C phase: the default ops' own BF wait is unbounded). The byte
     * must land in SSPBUF with no write collision; whether the shift
     * completes (BF) is model evidence, logged below. */
    EPIC_IRQ_ClearFlag(PIC16_IRQ_SSP);
    uint16_t swc = EPIC_SSP_WriteByte(0x55u);
    uint8_t spi_adv = 0u;
    for (uint32_t pi = 0; pi < SSP_POLLS; pi++) {
        if (EPIC_SSP_IsBufferFull()) { spi_adv = 1u; break; }
    }
    sspbuf = EPIC_REG8(PIC_REG_SSPBUF);
    log_reg("bus sim: spi SSPBUF after write", sspbuf);
    uint8_t spi_land = (uint8_t)(swc == 0u && sspbuf == 0x55u);
    ok &= spi_land;
    if (spi_land) { epic_harness_log("bus sim: spi byte landed in SSPBUF\n"); }
    else          { epic_harness_log("bus sim: spi byte did NOT land in SSPBUF\n"); }
    if (spi_adv) {
        uint8_t got = EPIC_SSP_ReadByte();
        ok &= (uint8_t)(got == 0x55u);
        epic_harness_log("bus sim: spi byte shift advanced\n");
    } else {
        epic_harness_log("bus sim: spi byte shift NOT modeled by MPLAB SIM\n");
    }

    epic_bus_set_spi_ops(&mock_spi);
    for (uint8_t i = 0; i < 16u; i++) { g_reg[i] = (uint8_t)(0x80u + i); }
    g_seq_n = 0u;
    n = epic_bus_spi_mem_read(0x02u, buf, 3);
    uint8_t srd_ok = (uint8_t)(n == 3 && buf[0] == 0x82u && buf[1] == 0x83u &&
                               buf[2] == 0x84u && g_seq_n == 6u &&
                               g_seq_op[0] == OP_START &&
                               g_seq_op[1] == OP_WRITE && g_seq_val[1] == 0x02u &&
                               g_seq_op[2] == OP_WRITE && g_seq_val[2] == 0x00u &&
                               g_seq_op[3] == OP_WRITE && g_seq_val[3] == 0x00u &&
                               g_seq_op[4] == OP_WRITE && g_seq_val[4] == 0x00u &&
                               g_seq_op[5] == OP_STOP);
    /* select, W(reg), W x3, deselect */
    if (srd_ok) { epic_harness_log("bus sim: spi mem_read seq ok\n"); }
    else        { epic_harness_log("bus sim: spi mem_read seq WRONG\n"); }
    ok &= srd_ok;

    uint8_t sw[2] = { 0xC0u, 0xC1u };
    g_seq_n = 0u;
    n = epic_bus_spi_mem_write(0x08u, sw, 2);
    uint8_t swr_ok = (uint8_t)(n == 2 && g_reg[8] == 0xC0u && g_reg[9] == 0xC1u &&
                               g_seq_n == 5u &&
                               g_seq_op[0] == OP_START &&
                               g_seq_op[1] == OP_WRITE && g_seq_val[1] == 0x08u &&
                               g_seq_op[2] == OP_WRITE && g_seq_val[2] == 0xC0u &&
                               g_seq_op[3] == OP_WRITE && g_seq_val[3] == 0xC1u &&
                               g_seq_op[4] == OP_STOP);
    if (swr_ok) { epic_harness_log("bus sim: spi mem_write seq ok\n"); }
    else        { epic_harness_log("bus sim: spi mem_write seq WRONG\n"); }
    ok &= swr_ok;

    for (uint32_t i = 0; epic_harness_running(i); i++) {
        epic_harness_tick();
    }

    return epic_harness_report(ok);
}
