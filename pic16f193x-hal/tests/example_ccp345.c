/**
 * @file    example_ccp345.c
 * @brief   CCP3/CCP4/CCP5 smoke test: all three in compare-set mode,
 *          confirm control-register + compare-value state.
 *
 * @details
 *   Expected register image (after init):
 *     CCP3CON = 0x08, CCPR3 = 0x0300
 *     CCP4CON = 0x08, CCPR4 = 0x0400
 *     CCP5CON = 0x08, CCPR5 = 0x0500
 */

#include "pic16f193x.h"
#include "pic16f193x_sfr.h"
#include "peripherals/pic16f193x_ccp.h"
#include "core/pic8_harness.h"

extern void pic16f193x_harness_halt(void);

int main(void)
{
    pic8_harness_init(1UL);

    CCP_HandleTypeDef ccp3 = CCP_HANDLE_DEFAULT;
    ccp3.Instance = CCP_INSTANCE_3;
    ccp3.Mode = CCP_MODE_COMPARE_SET;
    ccp3.CompareValue = 0x0300U;
    HAL_CCP_Init(&ccp3);

    CCP_HandleTypeDef ccp4 = CCP_HANDLE_DEFAULT;
    ccp4.Instance = CCP_INSTANCE_4;
    ccp4.Mode = CCP_MODE_COMPARE_SET;
    ccp4.CompareValue = 0x0400U;
    HAL_CCP_Init(&ccp4);

    CCP_HandleTypeDef ccp5 = CCP_HANDLE_DEFAULT;
    ccp5.Instance = CCP_INSTANCE_5;
    ccp5.Mode = CCP_MODE_COMPARE_SET;
    ccp5.CompareValue = 0x0500U;
    HAL_CCP_Init(&ccp5);

    uint8_t c3con = PIC8_REG8(PIC_REG_CCP3CON);
    uint8_t c3h = PIC8_REG8(PIC_REG_CCPR3H);
    uint8_t c3l = PIC8_REG8(PIC_REG_CCPR3L);
    uint8_t c4con = PIC8_REG8(PIC_REG_CCP4CON);
    uint8_t c4h = PIC8_REG8(PIC_REG_CCPR4H);
    uint8_t c4l = PIC8_REG8(PIC_REG_CCPR4L);
    uint8_t c5con = PIC8_REG8(PIC_REG_CCP5CON);
    uint8_t c5h = PIC8_REG8(PIC_REG_CCPR5H);
    uint8_t c5l = PIC8_REG8(PIC_REG_CCPR5L);

    pic8_harness_log("CCP3=0x%02X%02X%02X CCP4=0x%02X%02X%02X CCP5=0x%02X%02X%02X\n",
                      c3con, c3h, c3l, c4con, c4h, c4l, c5con, c5h, c5l);
    int pass = (c3con == 0x08U) && (c3h == 0x03U) && (c3l == 0x00U)
            && (c4con == 0x08U) && (c4h == 0x04U) && (c4l == 0x00U)
            && (c5con == 0x08U) && (c5h == 0x05U) && (c5l == 0x00U);
    int rc = pic8_harness_report(pass);
    pic16f193x_harness_halt();
    return rc;
}
