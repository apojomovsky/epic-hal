/**
 * Host model of the data EEPROM (NVM block), NCO1 and the WDT/Sleep
 * helpers. Side effects hang off the epic_sfr_write8 hook (called from
 * pic16f188xx_sim_sfr_write8's default case); time is instruction ticks.
 * The EEPROM completes a write after a settable tick count (default 0:
 * inside the write itself). NCO1 adds one increment per tick for the FOSC
 * and HFINTOSC clocks; the LCx clocks freeze it. Not modelled: program
 * Flash, user ID and configuration NVM access (a write there sets WRERR),
 * the EEPROM read stall, the WDT time-out and Sleep itself, which only
 * count.
 */
#include <stdint.h>
#include "pic16f188xx.h"
#include "pic16f188xx_sim.h"
#include "pic16f188xx_sim_priv.h"
#include "peripherals/pic16f188xx_nco.h"

#define SIM_EEPROM_BASE 0xF000U
#define SIM_EEPROM_SIZE 256U

#define SIM_NCO_ACC_BITS 20U

static uint8_t  sim_eeprom[SIM_EEPROM_SIZE];
static uint32_t sim_eeprom_ticks;
static uint32_t sim_wr_left;
static uint8_t  sim_wr_addr;
static uint8_t  sim_wr_data;
/* NVMCON2 key state: 0 idle, 1 after 0x55, 2 after 0xAA. */
static uint8_t  sim_nvm_unlock;

static uint32_t sim_nco_inc_buf;
static uint8_t  sim_nco_inc_pending;
static uint8_t  sim_nco_raw;
static uint16_t sim_nco_pfm_left;

/* CONFIG3 WDTCPS model: -1 leaves WDTPS software-writable (the XC8
 * default), 0..18 pins it to that value. */
static int8_t   sim_wdt_fixed;
static uint32_t sim_wdt_refreshes;
static uint32_t sim_sleeps;
static uint32_t sim_idles;

/**
 * @brief Shorthand for one SFR cell.
 * @param addr SFR address.
 * @return Pointer to the raw cell.
 */
static uint8_t *cell(uint16_t addr)
{
    return &pic16f188xx_sim_sfr[addr];
}

/**
 * @brief Assemble a 20-bit value from three SFR bytes.
 * @param lo Address of the low byte.
 * @param hi Address of the high byte.
 * @param up Address of the upper byte (low nibble used).
 * @return The 20-bit value.
 */
static uint32_t read20(uint16_t lo, uint16_t hi, uint16_t up)
{
    return (uint32_t)*cell(lo) | ((uint32_t)*cell(hi) << 8) |
           ((uint32_t)(*cell(up) & 0x0FU) << 16);
}

/**
 * @brief Store a 20-bit value into three SFR bytes.
 * @param lo Address of the low byte.
 * @param hi Address of the high byte.
 * @param up Address of the upper byte.
 * @param value Value to store.
 */
static void write20(uint16_t lo, uint16_t hi, uint16_t up, uint32_t value)
{
    *cell(lo) = (uint8_t)value;
    *cell(hi) = (uint8_t)(value >> 8);
    *cell(up) = (uint8_t)((value >> 16) & 0x0FU);
}

/**
 * @brief Test whether NVMADR with NVMREGS set points into the data EEPROM.
 * @return 1 for 0xF000..0xF0FF with NVMREGS set, 0 otherwise.
 */
static uint8_t nvm_in_eeprom(void)
{
    uint16_t addr = (uint16_t)(((uint16_t)*cell(PIC_REG_NVMADRH) << 8) |
        *cell(PIC_REG_NVMADRL));

    return (((*cell(PIC_REG_NVMCON1) & PIC_NVMCON1_NVMREGS) != 0U) &&
            (addr >= SIM_EEPROM_BASE) &&
            (addr < (uint16_t)(SIM_EEPROM_BASE + SIM_EEPROM_SIZE))) ? 1U : 0U;
}

/**
 * @brief Finish an EEPROM write: store the latched byte and flag NVMIF.
 */
static void nvm_finish_write(void)
{
    sim_eeprom[sim_wr_addr] = sim_wr_data;
    *cell(PIC_REG_NVMCON1) &= (uint8_t)~PIC_NVMCON1_WR;
    *cell(PIC_REG_PIR7) |= PIC_PIR7_NVMIF;
}

/**
 * @brief Apply an NVMCON1 write: RD and WR are set-only actions.
 * @details RD loads NVMDATL at once (EEPROM) or zeroes the data pair
 * (outside the EEPROM, the datasheet's out-of-range read). WR needs WREN
 * and the 0x55/0xAA key state, else it is refused with WRERR. A write
 * already running ignores a second WR and a zero write keeps WR.
 * @param value Value written.
 */
static void nvm_con1_write(uint8_t value)
{
    uint8_t old = *cell(PIC_REG_NVMCON1);
    uint8_t con = (uint8_t)((value & 0x7FU) &
        (uint8_t)~(PIC_NVMCON1_RD | PIC_NVMCON1_WR));
    uint8_t keys = sim_nvm_unlock;

    con |= (uint8_t)(old & PIC_NVMCON1_WR);
    sim_nvm_unlock = 0U;
    *cell(PIC_REG_NVMCON1) = con;

    if ((value & PIC_NVMCON1_RD) != 0U)
    {
        if (nvm_in_eeprom() != 0U)
        {
            *cell(PIC_REG_NVMDATL) = sim_eeprom[*cell(PIC_REG_NVMADRL)];
        }
        else
        {
            *cell(PIC_REG_NVMDATL) = 0U;
            *cell(PIC_REG_NVMDATH) = 0U;
        }
    }
    if (((value & PIC_NVMCON1_WR) == 0U) || ((old & PIC_NVMCON1_WR) != 0U) ||
        ((con & PIC_NVMCON1_WREN) == 0U))
    {
        return;
    }
    if ((keys != 2U) || (nvm_in_eeprom() == 0U))
    {
        *cell(PIC_REG_NVMCON1) |= PIC_NVMCON1_WRERR;
        return;
    }
    sim_wr_addr = *cell(PIC_REG_NVMADRL);
    sim_wr_data = *cell(PIC_REG_NVMDATL);
    if (sim_eeprom_ticks == 0U)
    {
        nvm_finish_write();
    }
    else
    {
        sim_wr_left = sim_eeprom_ticks;
        *cell(PIC_REG_NVMCON1) |= PIC_NVMCON1_WR;
    }
}

/**
 * @brief Recompute NCO1's output bit from the raw level and polarity.
 */
static void nco_update_out(void)
{
    uint8_t con = *cell(PIC_REG_NCO1CON);
    uint8_t level = (uint8_t)((con & PIC_NCO1CON_N1EN) != 0U ? sim_nco_raw : 0U);

    if (((con & PIC_NCO1CON_N1EN) != 0U) && ((con & PIC_NCO1CON_N1POL) != 0U))
    {
        level ^= 1U;
    }
    if (level != 0U)
    {
        *cell(PIC_REG_NCO1CON) |= PIC_NCO1CON_N1OUT;
    }
    else
    {
        *cell(PIC_REG_NCO1CON) &= (uint8_t)~PIC_NCO1CON_N1OUT;
    }
}

/**
 * @brief Apply an NCO1CON write: N1OUT is read-only, N1EN edges reload the
 * increment buffer or drop the output.
 * @param value Value written.
 */
static void nco_con_write(uint8_t value)
{
    uint8_t old = *cell(PIC_REG_NCO1CON);

    *cell(PIC_REG_NCO1CON) = (uint8_t)((value & PIC_NCO1CON_WRITE_MASK) |
        (old & PIC_NCO1CON_N1OUT));
    if (((old & PIC_NCO1CON_N1EN) == 0U) && ((value & PIC_NCO1CON_N1EN) != 0U))
    {
        sim_nco_inc_buf = read20(PIC_REG_NCO1INCL, PIC_REG_NCO1INCH, PIC_REG_NCO1INCU);
        sim_nco_inc_pending = 0U;
    }
    if ((value & PIC_NCO1CON_N1EN) == 0U)
    {
        sim_nco_raw = 0U;
        sim_nco_pfm_left = 0U;
    }
    nco_update_out();
}

/**
 * @brief Apply a write to one NCO1 increment byte.
 * @details Disabled, the double buffer loads at once. Enabled, only the low
 * byte triggers the load, two NCO clocks later (§24.1.4).
 * @param addr Increment register written.
 * @param value Value written.
 */
static void nco_inc_write(uint16_t addr, uint8_t value)
{
    *cell(addr) = (addr == PIC_REG_NCO1INCU) ? (uint8_t)(value & 0x0FU) : value;
    if ((*cell(PIC_REG_NCO1CON) & PIC_NCO1CON_N1EN) == 0U)
    {
        sim_nco_inc_buf = read20(PIC_REG_NCO1INCL, PIC_REG_NCO1INCH, PIC_REG_NCO1INCU);
    }
    else if (addr == PIC_REG_NCO1INCL)
    {
        sim_nco_inc_pending = 2U;
    }
}

/**
 * @brief Apply the simulator's write side effects for the NVM, NCO and WDT
 * blocks.
 * @details NVMCON2 collects the key sequence, NVMCON1 runs RD and WR, the
 * NCO control, clock and increment registers keep their unimplemented and
 * read-only bits clear, and WDTCON0 holds WDTPS at a pinned period. Every other address is not handled here.
 * @param addr SFR address.
 * @param value Value written.
 * @return 1 when handled (the cell is already updated), 0 for a plain store.
 */
uint8_t pic16f188xx_sim_sys_write(uint16_t addr, uint8_t value)
{
    switch (addr)
    {
    case PIC_REG_NVMCON2:
        if ((value == 0x55U) && (sim_nvm_unlock == 0U))
        {
            sim_nvm_unlock = 1U;
        }
        else if ((value == 0xAAU) && (sim_nvm_unlock == 1U))
        {
            sim_nvm_unlock = 2U;
        }
        else
        {
            sim_nvm_unlock = 0U;
        }
        return 1U;
    case PIC_REG_NVMCON1:
        nvm_con1_write(value);
        return 1U;
    case PIC_REG_NCO1CON:
        nco_con_write(value);
        return 1U;
    case PIC_REG_NCO1CLK:
        *cell(addr) = (uint8_t)(value & (PIC_NCO1CLK_N1PWS_MASK | PIC_NCO1CLK_N1CKS_MASK));
        return 1U;
    case PIC_REG_NCO1INCL:
    case PIC_REG_NCO1INCH:
    case PIC_REG_NCO1INCU:
        nco_inc_write(addr, value);
        return 1U;
    case PIC_REG_WDTCON0:
        if (sim_wdt_fixed >= 0)
        {
            value = (uint8_t)((value & (uint8_t)~PIC_WDTCON0_WDTPS_MASK) |
                (uint8_t)((uint8_t)sim_wdt_fixed << PIC_WDTCON0_WDTPS_SHIFT));
        }
        *cell(addr) = (uint8_t)(value & 0x3FU);
        return 1U;
    default:
        return 0U;
    }
}

/**
 * @brief Advance NCO1 by one tick.
 * @details Adds the buffered increment to the 20-bit accumulator; a carry
 * sets NCO1IF and toggles the FDC output or starts a PFM pulse of
 * 2^N1PWS ticks, active from the next tick. Polarity is applied last.
 */
static void sim_step_nco(void)
{
    uint8_t con = *cell(PIC_REG_NCO1CON);
    uint8_t clk = *cell(PIC_REG_NCO1CLK);
    uint32_t acc;
    uint8_t pulse;

    if ((con & PIC_NCO1CON_N1EN) == 0U)
    {
        return;
    }
    if ((uint8_t)(clk & PIC_NCO1CLK_N1CKS_MASK) > (uint8_t)NCO_CLK_HFINTOSC)
    {
        return;
    }
    if ((sim_nco_inc_pending > 0U) && (--sim_nco_inc_pending == 0U))
    {
        sim_nco_inc_buf = read20(PIC_REG_NCO1INCL, PIC_REG_NCO1INCH, PIC_REG_NCO1INCU);
    }
    pulse = (uint8_t)(sim_nco_pfm_left > 0U);
    if (sim_nco_pfm_left > 0U)
    {
        sim_nco_pfm_left--;
    }
    acc = read20(PIC_REG_NCO1ACCL, PIC_REG_NCO1ACCH, PIC_REG_NCO1ACCU) + sim_nco_inc_buf;
    if (acc > NCO_VALUE_MAX)
    {
        *cell(PIC_REG_PIR7) |= PIC_PIR7_NCO1IF;
        if ((con & PIC_NCO1CON_N1PFM) != 0U)
        {
            sim_nco_pfm_left = (uint16_t)(1U << ((unsigned int)(clk &
                PIC_NCO1CLK_N1PWS_MASK) >> PIC_NCO1CLK_N1PWS_SHIFT));
        }
        else
        {
            sim_nco_raw ^= 1U;
        }
    }
    if ((con & PIC_NCO1CON_N1PFM) != 0U)
    {
        sim_nco_raw = pulse;
    }
    write20(PIC_REG_NCO1ACCL, PIC_REG_NCO1ACCH, PIC_REG_NCO1ACCU,
        acc & NCO_VALUE_MAX);
    nco_update_out();
}

/**
 * @brief Advance the NVM write countdown and NCO1 by one tick.
 */
void pic16f188xx_sim_sys_step(void)
{
    if ((sim_wr_left > 0U) && (--sim_wr_left == 0U))
    {
        nvm_finish_write();
    }
    sim_step_nco();
}

/**
 * @brief Load the POR values of the NVM, NCO, WDT and power SFRs and
 * clear the model state; the EEPROM is erased to 0xFF (a blank part).
 * @details NVMADRH 0x80, NCO1INCL 0x01, PCON0 0x3D (POR occurred, BOR
 * not), WDTCON0 0x16 (WDTPS 2 s) and WDTCON1 0x07 (window open) follow
 * the EDC por values with the XC8-default CONFIG3.
 */
void pic16f188xx_sim_sys_reset(void)
{
    uint16_t i;

    for (i = 0U; i < SIM_EEPROM_SIZE; i++)
    {
        sim_eeprom[i] = 0xFFU;
    }
    sim_eeprom_ticks = 0U;
    sim_wr_left = 0U;
    sim_wr_addr = 0U;
    sim_wr_data = 0U;
    sim_nvm_unlock = 0U;
    sim_nco_inc_buf = 1UL;
    sim_nco_inc_pending = 0U;
    sim_nco_raw = 0U;
    sim_nco_pfm_left = 0U;
    sim_wdt_fixed = -1;
    sim_wdt_refreshes = 0U;
    sim_sleeps = 0U;
    sim_idles = 0U;
    *cell(PIC_REG_NVMADRH) = 0x80U;
    *cell(PIC_REG_NCO1INCL) = 0x01U;
    *cell(PIC_REG_PCON0) = 0x3DU;
    *cell(PIC_REG_WDTCON0) = 0x16U;
    *cell(PIC_REG_WDTCON1) = 0x07U;
}

/**
 * @brief Count one EPIC_WDT_Refresh call.
 */
void pic16f188xx_sim_wdt_note_refresh(void)
{
    sim_wdt_refreshes++;
}

/**
 * @brief Count one EPIC_Sleep_Enter call, and one idle entry when
 * CPUDOZE<IDLEN> is set.
 */
void pic16f188xx_sim_sleep_note(void)
{
    sim_sleeps++;
    if ((*cell(PIC_REG_CPUDOZE) & PIC_CPUDOZE_IDLEN) != 0U)
    {
        sim_idles++;
    }
}

/**
 * @brief Set how many ticks an EEPROM write takes.
 * @param ticks Ticks from WR set to WR clear; 0 completes inside the write.
 */
void pic16f188xx_sim_eeprom_set_write_ticks(uint32_t ticks)
{
    sim_eeprom_ticks = ticks;
}

/**
 * @brief Read the EEPROM backing store directly.
 * @param addr Byte address 0..255.
 * @return The stored byte.
 */
uint8_t pic16f188xx_sim_eeprom_peek(uint8_t addr)
{
    return sim_eeprom[addr];
}

/**
 * @brief Pin the WDT period as CONFIG3 WDTCPS does.
 * @param wdtps Period code 0..18 the WDTPS bits stay at, or -1 to make them
 *              writable again.
 */
void pic16f188xx_sim_wdt_set_fixed_period(int8_t wdtps)
{
    sim_wdt_fixed = wdtps;
    if (wdtps >= 0)
    {
        *cell(PIC_REG_WDTCON0) = (uint8_t)((*cell(PIC_REG_WDTCON0) &
            (uint8_t)~PIC_WDTCON0_WDTPS_MASK) |
            (uint8_t)((uint8_t)wdtps << PIC_WDTCON0_WDTPS_SHIFT));
    }
}

/**
 * @brief Count the EPIC_WDT_Refresh calls since reset.
 * @return Refresh count.
 */
uint32_t pic16f188xx_sim_wdt_refresh_count(void)
{
    return sim_wdt_refreshes;
}

/**
 * @brief Count the EPIC_Sleep_Enter calls since reset (Idle entries included).
 * @return Sleep entry count.
 */
uint32_t pic16f188xx_sim_sleep_count(void)
{
    return sim_sleeps;
}

/**
 * @brief Count the sleeps entered with CPUDOZE<IDLEN> set since reset.
 * @return Idle entry count.
 */
uint32_t pic16f188xx_sim_idle_count(void)
{
    return sim_idles;
}
