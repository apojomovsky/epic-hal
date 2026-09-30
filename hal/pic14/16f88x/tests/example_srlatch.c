/* SR latch driver smoke test on the sim backend (DS40001291H §8.9):
 * SRCON output configuration, software set/reset pulses. */

#include "pic16f88x.h"
#include "pic16f88x_sim.h"
#include "pic16f88x_sfr.h"
#include "peripherals/pic16f88x_srlatch.h"
#include <stdio.h>

#define CHECK(cond, msg) do { \
    if (!(cond)) { printf("FAIL: %s\n", msg); return 1; } \
} while (0)

/**
 * @brief Smoke-test the SR latch driver: init config, pulse set/reset.
 */
int main(void)
{
    pic16f88x_sim_reset();

    /* 1. Route latch Q to the C1OUT pin, C1 sets, C2 resets. */
    SRLATCH_HandleTypeDef h = SRLATCH_HANDLE_DEFAULT;
    h.Output         = SRLATCH_OUT_C1_Q;
    h.C1SetEnable    = true;
    h.C2ResetEnable  = true;
    CHECK(EPIC_SRLATCH_Init(&h) == EPIC_OK, "SRLATCH_Init failed");

    /* SRCON = SR0(bit6) | C1SEN(bit5) | C2REN(bit4) = 0x70. */
    {
        uint8_t prev = (EPIC_REG8(PIC_REG_STATUS) >> 5) & 0x03U;
        pic_select_bank(3);
        uint8_t srcon = EPIC_REG8(PIC_REG_SRCON);
        pic_select_bank(prev);
        CHECK((srcon & 0x70U) == 0x70U, "SRCON not programmed for Q-out|C1SEN|C2REN");
    }

    /* 2. Software set pulse (PULSS, self-clearing). */
    EPIC_SRLATCH_Set();
    {
        uint8_t prev = (EPIC_REG8(PIC_REG_STATUS) >> 5) & 0x03U;
        pic_select_bank(3);
        uint8_t srcon = EPIC_REG8(PIC_REG_SRCON);
        pic_select_bank(prev);
        CHECK((srcon & PIC_SRCON_PULSS) != 0U, "PULSS not set after SRLATCH_Set");
    }

    /* 3. Software reset pulse (PULSR). */
    EPIC_SRLATCH_Reset();
    {
        uint8_t prev = (EPIC_REG8(PIC_REG_STATUS) >> 5) & 0x03U;
        pic_select_bank(3);
        uint8_t srcon = EPIC_REG8(PIC_REG_SRCON);
        pic_select_bank(prev);
        CHECK((srcon & PIC_SRCON_PULSR) != 0U, "PULSR not set after SRLATCH_Reset");
    }

    /* 4. DeInit returns SRCON to reset. */
    EPIC_SRLATCH_DeInit();
    {
        uint8_t prev = (EPIC_REG8(PIC_REG_STATUS) >> 5) & 0x03U;
        pic_select_bank(3);
        uint8_t srcon = EPIC_REG8(PIC_REG_SRCON);
        pic_select_bank(prev);
        CHECK(srcon == 0x00U, "SRCON not zero after DeInit");
    }

    printf("OK: SR latch driver, config, set/reset pulses, deinit all pass.\n");
    return 0;
}
