/**
 * @file    example_ccp.c
 * @brief   CCP1/CCP2 smoke test: both instances in compare-set mode,
 *          confirm control-register + compare-value state. The §4 gate
 *          payload for the CCP1/CCP2 driver.
 *
 * @details
 *   This is a pure register-state check (no ISR, no loop): init both
 *   instances in compare-set mode with distinct compare values, read
 *   the registers back, and verify they match. No EventCallback is set
 *   so no ISRs fire and the MPLAB SIM target is not starved.
 *
 *   Expected register image (after init):
 *     CCP1CON = 0x08   (CCP1M<3:0> = 1000, compare-set)
 *     CCPR1H  = 0x01, CCPR1L = 0x00  (CompareValue = 0x0100)
 *     CCP2CON = 0x08
 *     CCPR2H  = 0x02, CCPR2L = 0x00  (CompareValue = 0x0200)
 *     PIE1    = 0x00   (CCP1IE disabled, no callback)
 *     PIE2    = 0x00   (CCP2IE disabled, no callback)
 */

#include "pic16f193x.h"
#include "pic16f193x_sfr.h"
#include "peripherals/pic16f193x_ccp.h"
#include "core/epic_harness.h"

extern void pic16f193x_harness_halt(void);

int main(void)
{
    epic_harness_init(1UL);

    CCP_HandleTypeDef ccp1 = CCP_HANDLE_DEFAULT;
    ccp1.Instance = CCP_INSTANCE_1;
    ccp1.Mode = CCP_MODE_COMPARE_SET;
    ccp1.CompareValue = 0x0100U;
    EPIC_CCP_Init(&ccp1);

    CCP_HandleTypeDef ccp2 = CCP_HANDLE_DEFAULT;
    ccp2.Instance = CCP_INSTANCE_2;
    ccp2.Mode = CCP_MODE_COMPARE_SET;
    ccp2.CompareValue = 0x0200U;
    EPIC_CCP_Init(&ccp2);

    uint8_t c1con = PIC8_REG8(PIC_REG_CCP1CON);
    uint8_t c1h = PIC8_REG8(PIC_REG_CCPR1H);
    uint8_t c1l = PIC8_REG8(PIC_REG_CCPR1L);
    uint8_t c2con = PIC8_REG8(PIC_REG_CCP2CON);
    uint8_t c2h = PIC8_REG8(PIC_REG_CCPR2H);
    uint8_t c2l = PIC8_REG8(PIC_REG_CCPR2L);

    epic_harness_log("CCP1CON=0x%02X CCPR1=0x%02X%02X CCP2CON=0x%02X CCPR2=0x%02X%02X\n",
                      c1con, c1h, c1l, c2con, c2h, c2l);
    int pass = (c1con == 0x08U) && (c1h == 0x01U) && (c1l == 0x00U)
            && (c2con == 0x08U) && (c2h == 0x02U) && (c2l == 0x00U);
    int rc = epic_harness_report(pass);
    pic16f193x_harness_halt();
    return rc;
}
