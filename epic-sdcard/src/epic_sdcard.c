/** Binds M-Stack's mmc.c to this repo's SSP/GPIO HAL and epic-tick: SPI
 *  byte transfer, CS control, clock-speed selection, and timeout timing,
 *  against one owned mmc_card. */

#include "epic_sdcard.h"
#include "epic_hal.h"
#include "epic_tick.h"
#include "mmc.h"

#include <stdbool.h>

static struct mmc_card    g_card;
static epic_sdcard_pins_t g_pins;
static uint32_t           g_fosc_hz;
static uint32_t           g_timer_start_tick;
static uint16_t           g_timer_timeout_ms;

/**
 * @brief Exchange one byte over the SSP in SPI mode.
 *
 * SPI byte-level primitive: writes out, clearing any write collision,
 * waits for the buffer to fill, and returns the received byte.
 *
 * @param out byte to transmit
 * @return byte received from the card
 */
static uint8_t spi_byte(uint8_t out)
{
    uint16_t w;
    do {
        w = EPIC_SSP_WriteByte(out);
        if (w == 0xFFFFu) {
            EPIC_SSP_ClearWriteCollision();     /* must be cleared in software, DS39632E §19.2.2 */
        }
    } while (w == 0xFFFFu);
    while (!EPIC_SSP_IsBufferFull()) {
    }
    return EPIC_SSP_ReadByte();
}

/* MMC_SPI_* callbacks, named in src/target/mmc_config.h */

/**
 * @brief MMC_SPI_TRANSFER callback: move len bytes between the card and
 *        the caller.
 *
 * Reads clock junk (0xFF) when out_buf is NULL, per SD-over-SPI.
 *
 * @param instance SPI instance (unused, single card)
 * @param out_buf  bytes to send, or NULL to clock out 0xFF
 * @param in_buf   buffer to receive bytes, or NULL to discard
 * @param len      number of bytes to transfer
 */
void epic_sdcard_spi_transfer(uint8_t instance, const uint8_t *out_buf,
                              uint8_t *in_buf, uint16_t len)
{
    (void)instance;
    for (uint16_t i = 0; i < len; i++) {
        uint8_t out = out_buf ? out_buf[i] : 0xFFu;   /* SD-over-SPI: clock junk while reading */
        uint8_t in = spi_byte(out);
        if (in_buf) {
            in_buf[i] = in;
        }
    }
}

/**
 * @brief MMC_SPI_CS callback: assert or deassert the card's CS pin.
 *
 * @param instance SPI instance (unused, single card)
 * @param value    0 = asserted, nonzero = deasserted, per mmc.h
 */
void epic_sdcard_spi_set_cs(uint8_t instance, uint8_t value)
{
    (void)instance;
    EPIC_GPIO_WritePin(g_pins.cs_port, g_pins.cs_pin,
                      value ? GPIO_PIN_SET : GPIO_PIN_RESET);   /* 0 = asserted, per mmc.h */
}

/**
 * @brief Pick the fastest SSP divisor meeting target_hz.
 *
 * The SSP has 3 fixed divisors (Fosc/4, /16, /64); the fastest that
 * still meets target_hz is chosen, falling back to the slowest if none
 * do. Known gap: at 48 MHz (this family's USB clock), the slowest
 * available divisor is 750 kHz, above the SD spec's 400 kHz bring-up
 * ceiling; unverified whether real cards tolerate that without a board
 * to test on.
 *
 * @param target_hz   desired SPI clock rate
 * @param achieved_hz receives the achieved rate
 * @return the SSP mode for the chosen divisor
 */
static SSP_ModeTypeDef pick_divisor(uint32_t target_hz, uint32_t *achieved_hz)
{
    static const uint8_t          divs[3]  = {4u, 16u, 64u};
    static const SSP_ModeTypeDef  modes[3] = {SSP_MODE_SPI_MASTER_FOSC_4,
                                              SSP_MODE_SPI_MASTER_FOSC_16,
                                              SSP_MODE_SPI_MASTER_FOSC_64};
    int8_t best = -1;

    for (uint8_t i = 0; i < 3u; i++) {
        uint32_t hz = g_fosc_hz / divs[i];
        if (hz <= target_hz) {
            if (best < 0 || hz > (g_fosc_hz / divs[(uint8_t)best])) {
                best = (int8_t)i;
            }
        }
    }
    if (best < 0) {
        best = 2;   /* none met target; slowest available is the safest fallback */
    }

    *achieved_hz = g_fosc_hz / divs[(uint8_t)best];
    return modes[(uint8_t)best];
}

/**
 * @brief MMC_SPI_SET_SPEED callback: reconfigure the SPI clock rate.
 *
 * @param instance SPI instance (unused, single card)
 * @param speed_hz desired SPI clock rate
 */
void epic_sdcard_spi_set_speed(uint8_t instance, uint32_t speed_hz)
{
    (void)instance;
    uint32_t achieved;
    SSP_ModeTypeDef mode = pick_divisor(speed_hz, &achieved);

    SSP_HandleTypeDef h = SSP_HANDLE_DEFAULT;   /* CKP=idle-low, CKE=idle->active: SPI mode 0,0, matches SD-over-SPI */
    h.Mode = mode;
    EPIC_SSP_Init(&h);
}

/* MMC_TIMER_* callbacks: real wall-clock timeouts via epic-tick */

/**
 * @brief MMC_TIMER_START callback: arm the card timeout.
 *
 * @param instance   timer instance (unused, single card)
 * @param timeout_ms timeout duration in milliseconds
 */
void epic_sdcard_timer_start(uint8_t instance, uint16_t timeout_ms)
{
    (void)instance;
    g_timer_start_tick = epic_tick_get();
    g_timer_timeout_ms = timeout_ms;
}

/**
 * @brief MMC_TIMER_EXPIRED callback: report whether the timeout elapsed.
 *
 * @param instance timer instance (unused, single card)
 * @return true when timeout_ms have elapsed since timer_start
 */
bool epic_sdcard_timer_expired(uint8_t instance)
{
    (void)instance;
    return epic_tick_elapsed_since(g_timer_start_tick) >= g_timer_timeout_ms;
}

/**
 * @brief MMC_TIMER_STOP callback: disarm the card timeout.
 *
 * @param instance timer instance (unused, single card)
 */
void epic_sdcard_timer_stop(uint8_t instance)
{
    (void)instance;
}

/**
 * @brief Configure SPI mode 0,0, assert CS idle, and run the SD/MMC
 *        bring-up sequence (CMD0/CMD8/ACMD41/CMD58/CMD9).
 *
 * Blocks until the card responds or M-Stack's retry/timeout bounds are
 * hit.
 *
 * @param pins     CS pin assignment
 * @param fosc_hz  system oscillator frequency in Hz
 * @return true when read/write/num_blocks are usable
 */
bool epic_sdcard_init(const epic_sdcard_pins_t *pins, uint32_t fosc_hz)
{
    g_pins = *pins;
    g_fosc_hz = fosc_hz;

    EPIC_GPIO_Init(g_pins.cs_port, g_pins.cs_pin, GPIO_MODE_OUTPUT);
    epic_sdcard_spi_set_cs(0, 1);   /* deasserted before the SSP is even configured */

    SSP_HandleTypeDef h = SSP_HANDLE_DEFAULT;
    h.Mode = SSP_MODE_SPI_MASTER_FOSC_64;   /* slow starting point; mmc_init_card re-speeds via MMC_SPI_SET_SPEED */
    EPIC_SSP_Init(&h);

    g_card.max_speed_hz = 20000000UL;       /* SSP/board ceiling; MIN()'d against the card's own negotiated speed */
    g_card.spi_instance = 0u;
    mmc_init(&g_card, 1u);

    return mmc_init_card(&g_card) == 0;
}

/**
 * @brief Re-query the card's status (SEND_STATUS-adjacent, see mmc_ready()
 *        in the vendored mmc.h).
 *
 * @return true when the card reports ready
 */
bool epic_sdcard_ready(void)
{
    return mmc_ready(&g_card);
}

/**
 * @brief Return the number of 512-byte blocks on the card, cached from
 *        init.
 *
 * @return block count, or 0 if not initialized
 */
uint32_t epic_sdcard_num_blocks(void)
{
    return mmc_get_num_blocks(&g_card);
}

/**
 * @brief Read one 512-byte block into data (must be at least 512 bytes).
 *
 * @param block_addr block address to read
 * @param data       destination buffer (>= 512 bytes)
 * @return true on success, including the CRC16 check passing
 */
bool epic_sdcard_read_block(uint32_t block_addr, uint8_t *data)
{
    return mmc_read_block(&g_card, block_addr, data) == 0;
}

/**
 * @brief Write one 512-byte block from data (must be exactly 512 bytes).
 *
 * @param block_addr block address to write
 * @param data       source buffer (exactly 512 bytes)
 * @return true if the card accepted the data and reported no write error
 *         via the follow-up SEND_STATUS check
 */
bool epic_sdcard_write_block(uint32_t block_addr, const uint8_t *data)
{
    return mmc_write_block(&g_card, block_addr, (uint8_t *)data) == 0;
}
