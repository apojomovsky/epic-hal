/* Real-driver epic-cc footprint probe: links epic_sdcard.c and the
 * vendored m-stack mmc.c/crc.c against the HAL slice, runs the same
 * GPIO + SSP bring-up epic_sdcard_init performs, and lands the card
 * state on LATB for an mdb-hex register read. The card bring-up itself
 * (mmc_init_card) is not run: its SPI transfers spin on SSPSTAT<BF>,
 * which no simulator drives, and the 512-byte block buffer the full
 * example needs exceeds epic-cc's array cap. XC8 keeps the full
 * example. */
#include "epic_sdcard.h"
#include "epic_hal.h"
#include "mmc.h"

#include <stdint.h>

/* No manifest config words on this path; keep the WDT off so the
 * MPLAB SIM gate is not reset mid-run. usbdiv/cpudiv/plldiv match the
 * manifest's USBDIV = "2", CPUDIV = "OSC1_PLL2" and PLLDIV = "5" (the
 * 4550's config data requires the fields). */
EPIC_CONFIG("osc=hs, wdt=off, xtal_hz=20000000, usbdiv=on, cpudiv=div1, plldiv=div5");

/* PORTB as output, value written below (the mdb-hex execution gate). */
#define TRISB_REG PIC_REG_TRISB
#define LATB_REG  PIC_REG_LATB

static struct mmc_card g_card;

/** @brief Main. @return 0. */
int main(void)
{
    EPIC_GPIO_Init(GPIOB, GPIO_PIN_0, GPIO_MODE_OUTPUT);

    /* The same SSP setup epic_sdcard_init performs before bring-up:
     * SPI mode 0,0 at the slow starting divisor. */
    SSP_HandleTypeDef h = SSP_HANDLE_DEFAULT;
    h.Mode = SSP_MODE_SPI_MASTER_FOSC_64;
    EPIC_SSP_Init(&h);

    g_card.max_speed_hz = 20000000UL;
    g_card.spi_instance = 0u;
    mmc_init(&g_card, 1u);

    /* Observable result: 1 when the card state machine is idle (the
     * post-init state), 0 otherwise. */
    EPIC_REG8(TRISB_REG) = (uint8_t)0x00u;
    EPIC_REG8(LATB_REG) = (uint8_t)(g_card.state == MMC_STATE_IDLE);
    for (;;) {
    }
}
