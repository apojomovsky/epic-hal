/*
 * Streaming Parallel Port driver smoke test on the PIC18 host sim,
 * 40/44-pin parts only (DS39632E §18.0): init register programming,
 * byte read/write through SPPDATA/SPPEPS, sim-driven status/flag
 * helpers, deinit.
 */

#include "epic_hal.h"
#include "core/epic_harness.h"
#include "pic18fxx5x_sim.h"

#define CHECK(cond, msg) do { \
    if (!(cond)) { epic_harness_log("FAIL: %s\n", msg); return epic_harness_report(0); } \
} while (0)

int main(void)
{
    epic_harness_init(16U);
    pic18_sim_set_irq_callback(NULL);

    /* 1. Init: MCU ownership, CLKCFG=addr-write/data-rw, no CS/CLK1, WS=0, ep=0.
     *    SPPCON = SPPEN(0x01) | SPPOWN=0 (MCU)  = 0x01
     *    SPPCFG = (0<<6) | 0 | 0 | 0            = 0x00
     *    SPPEPS = ep 0                          = 0x00 */
    SPP_HandleTypeDef h = SPP_HANDLE_DEFAULT;
    h.Ownership   = SPP_OWN_MICROCONTROLLER;
    h.ClockConfig = SPP_CLKCFG_ADDR_WRITE_DATA_RW;
    h.Endpoint    = 0U;
    EPIC_SPP_Init(&h);

    CHECK(epic_sfr_read8(PIC_REG_SPPCON) == 0x01U, "SPPCON not 0x01 (SPPEN, MCU-owned)");
    CHECK(epic_sfr_read8(PIC_REG_SPPCFG) == 0x00U, "SPPCFG not 0x00 for default config");
    CHECK(epic_sfr_read8(PIC_REG_SPPEPS) == 0x00U, "SPPEPS not 0x00 for ep 0");

    /* 2. Write a byte to endpoint 2. */
    EPIC_SPP_WriteByte(2U, 0xA5U);
    CHECK((epic_sfr_read8(PIC_REG_SPPEPS) & PIC_SPPEPS_ADDR_MASK) == 0x02U,
          "SPPEPS addr not 2 after WriteByte");
    CHECK(epic_sfr_read8(PIC_REG_SPPDATA) == 0xA5U, "SPPDATA not 0xA5 after WriteByte");

    /* 3. Sim a write event: WRSPP set + SPPIF set. */
    pic18_sim_drive_spp(1U, 0U);
    CHECK(EPIC_SPP_HasWriteOccurred() == 1U, "WRSPP not set after drive_spp(1,0)");
    CHECK(EPIC_SPP_HasReadOccurred() == 0U, "RDSPP set after drive_spp(1,0)");
    CHECK(EPIC_SPP_IsInterruptFlag() == 1U, "SPPIF not set after drive_spp");
    EPIC_SPP_ClearITFlag();
    CHECK(EPIC_SPP_IsInterruptFlag() == 0U, "SPPIF not cleared");

    /* 4. Sim a read event. */
    pic18_sim_drive_spp(0U, 1U);
    CHECK(EPIC_SPP_HasReadOccurred() == 1U, "RDSPP not set after drive_spp(0,1)");

    /* 5. ReadByte returns SPPDATA. */
    uint8_t got = EPIC_SPP_ReadByte(2U);
    CHECK(got == 0xA5U, "ReadByte did not return 0xA5");

    /* 6. USB ownership + CLKCFG=write/read + CS + CLK1 + WS=4 + ep 5.
     *    SPPCON = SPPEN(0x01) | SPPOWN(0x02) = 0x03
     *    SPPCFG = (1<<6=0x40) | CSEN(0x20) | CLK1EN(0x10) | WS=4(0x04) = 0x74
     *    SPPEPS = ep 5 */
    h.Ownership   = SPP_OWN_USB;
    h.ClockConfig = SPP_CLKCFG_WRITE_READ;
    h.CSEnable    = true;
    h.CLK1Enable = true;
    h.WaitStates  = 4U;
    h.Endpoint   = 5U;
    EPIC_SPP_Init(&h);
    CHECK(epic_sfr_read8(PIC_REG_SPPCON) == 0x03U, "SPPCON not 0x03 for USB-owned");
    CHECK(epic_sfr_read8(PIC_REG_SPPCFG) == 0x74U, "SPPCFG not 0x74 for write-read/CS/CLK1/WS4");
    CHECK((epic_sfr_read8(PIC_REG_SPPEPS) & PIC_SPPEPS_ADDR_MASK) == 0x05U,
          "SPPEPS addr not 5 after init");

    /* 7. DeInit restores 0x00. */
    EPIC_SPP_DeInit();
    CHECK(epic_sfr_read8(PIC_REG_SPPCON) == 0x00U, "SPPCON not 0x00 after DeInit");
    CHECK(epic_sfr_read8(PIC_REG_SPPCFG) == 0x00U, "SPPCFG not 0x00 after DeInit");

    epic_harness_log("OK: SPP driver, init (SPPCON/CFG/EPS), write/read, status flags all pass.\n");
    return epic_harness_report(1);
}
