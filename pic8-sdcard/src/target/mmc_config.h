/*
 * M-Stack mmc.h config for the real PIC18Fxx5x target, private to this
 * module (included only by the vendored mmc.c for real-silicon builds;
 * tests/mock/mmc_config.h is the host-build equivalent). Binds
 * the MMC_SPI_ and timer macros to pic8_sdcard.c's HAL-backed functions, using
 * pic8-tick for real wall-clock timeouts instead of mmc.c's bounded-retry
 * fallback.
 */

#ifndef PIC8_SDCARD_MMC_CONFIG_H__
#define PIC8_SDCARD_MMC_CONFIG_H__

#define MMC_SPI_TRANSFER  pic8_sdcard_spi_transfer
#define MMC_SPI_SET_CS    pic8_sdcard_spi_set_cs
#define MMC_SPI_SET_SPEED pic8_sdcard_spi_set_speed

#define MMC_USE_TIMER
#define MMC_TIMER_START   pic8_sdcard_timer_start
#define MMC_TIMER_EXPIRED pic8_sdcard_timer_expired
#define MMC_TIMER_STOP    pic8_sdcard_timer_stop

#endif /* PIC8_SDCARD_MMC_CONFIG_H__ */
