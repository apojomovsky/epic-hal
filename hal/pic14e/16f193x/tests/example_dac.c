#include "pic16f193x.h"
#include "pic16f193x_sfr.h"
#include "peripherals/pic16f193x_dac.h"
#include "core/epic_harness.h"
/**
 * @brief Freeze the target so the harness PASS marker stays set; no-op
 * on the host build.
 */
extern void pic16f193x_harness_halt(void);
/**
 * @brief DAC smoke test: init and verify the DACCON0/DACCON1 register
 * state.
 */
int main(void)
{
    epic_harness_init(1UL);
    DAC_HandleTypeDef dac = DAC_HANDLE_DEFAULT;
    EPIC_DAC_Init(&dac);
    uint8_t con0 = EPIC_REG8(PIC_REG_DACCON0);
    uint8_t con1 = EPIC_REG8(PIC_REG_DACCON1);
    epic_harness_log("DACCON0=0x%02X DACCON1=0x%02X\n", con0, con1);
    int pass = (con0 == 0x80U) && (con1 == 0x0FU);
    int rc = epic_harness_report(pass);
    pic16f193x_harness_halt();
    return rc;
}
