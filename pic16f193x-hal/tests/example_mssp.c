/**
 * MSSP SPI-master smoke test: init at Fosc/4, confirm control-register
 * state. Expected register image (after init):
 *   SSPSTAT = 0x40   (CKE=1 at bit 6, SMP=0)
 *   SSPCON1 = 0x20   (SSPM=0000 Fosc/4, CKP=0, SSPEN=1 at bit 5)
 */

#include "pic16f193x.h"
#include "pic16f193x_sfr.h"
#include "peripherals/pic16f193x_ssp.h"
#include "core/epic_harness.h"

/**
 * @brief Freeze the target so the harness PASS marker stays set; no-op
 * on the host build.
 */
extern void pic16f193x_harness_halt(void);

/**
 * @brief MSSP SPI-master smoke test: init at Fosc/4 and verify the
 * SSPSTAT/SSPCON1 register state.
 */
int main(void)
{
    epic_harness_init(1UL);

    SSP_HandleTypeDef ssp = SSP_HANDLE_DEFAULT;
    EPIC_SSP_Init(&ssp);

    uint8_t stat = EPIC_REG8(PIC_REG_SSPSTAT);
    uint8_t con1 = EPIC_REG8(PIC_REG_SSPCON1);

    epic_harness_log("SSPSTAT=0x%02X SSPCON1=0x%02X\n", stat, con1);
    int pass = (stat == 0x40U) && (con1 == 0x20U);
    int rc = epic_harness_report(pass);
    pic16f193x_harness_halt();
    return rc;
}
