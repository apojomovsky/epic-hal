/*
 * M-Stack mmc.h config for the host test build: binds MMC_SPI_* to
 * epic_sdcard_mock_spi.c instead of real hardware. MMC_USE_TIMER is
 * deliberately not defined: a host mock has no real SPI latency to time
 * out against, so mmc.c's bounded-retry-count fallback is sufficient
 * here (the real target's mmc_config.h does define it, bound to
 * epic-tick).
 */

#ifndef EPIC_SDCARD_MOCK_MMC_CONFIG_H__
#define EPIC_SDCARD_MOCK_MMC_CONFIG_H__

#define MMC_SPI_TRANSFER  epic_sdcard_mock_spi_transfer
#define MMC_SPI_SET_CS    epic_sdcard_mock_spi_set_cs
#define MMC_SPI_SET_SPEED epic_sdcard_mock_spi_set_speed

#endif /* EPIC_SDCARD_MOCK_MMC_CONFIG_H__ */
