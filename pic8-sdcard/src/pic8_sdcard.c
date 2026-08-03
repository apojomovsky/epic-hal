/**
 * @file    pic8_sdcard.c
 * @brief   Binds M-Stack's mmc.c to this repo's SSP/GPIO HAL and
 *          pic8-tick: SPI byte transfer, CS control, clock-speed
 *          selection, and timeout timing, against one owned mmc_card.
 */

#include "pic8_sdcard.h"
#include "pic8_hal.h"
#include "pic8_tick.h"
#include "mmc.h"

#include <stdbool.h>

static struct mmc_card    g_card;
static pic8_sdcard_pins_t g_pins;
static uint32_t           g_fosc_hz;
static uint32_t           g_timer_start_tick;
static uint16_t           g_timer_timeout_ms;

/* ---- SPI byte-level primitive ---- */

static uint8_t spi_byte(uint8_t out)
{
    uint16_t w;
    do {
        w = HAL_SSP_WriteByte(out);
        if (w == 0xFFFFu) {
            HAL_SSP_ClearWriteCollision();     /* must be cleared in software, DS39632E §19.2.2 */
        }
    } while (w == 0xFFFFu);
    while (!HAL_SSP_IsBufferFull()) {
    }
    return HAL_SSP_ReadByte();
}

/* ---- MMC_SPI_* callbacks (named in src/target/mmc_config.h) ---- */

void pic8_sdcard_spi_transfer(uint8_t instance, const uint8_t *out_buf,
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

void pic8_sdcard_spi_set_cs(uint8_t instance, uint8_t value)
{
    (void)instance;
    HAL_GPIO_WritePin(g_pins.cs_port, g_pins.cs_pin,
                      value ? GPIO_PIN_SET : GPIO_PIN_RESET);   /* 0 = asserted, per mmc.h */
}

/* Picks the fastest of the SSP's 3 fixed divisors (Fosc/4, /16, /64) that
 * still meets target_hz, falling back to the slowest if none do. Known
 * gap: at 48 MHz (this family's USB clock), the slowest available divisor
 * is 750 kHz, above the SD spec's 400 kHz bring-up ceiling; unverified
 * whether real cards tolerate that without a board to test on. */
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

void pic8_sdcard_spi_set_speed(uint8_t instance, uint32_t speed_hz)
{
    (void)instance;
    uint32_t achieved;
    SSP_ModeTypeDef mode = pick_divisor(speed_hz, &achieved);

    SSP_HandleTypeDef h = SSP_HANDLE_DEFAULT;   /* CKP=idle-low, CKE=idle->active: SPI mode 0,0, matches SD-over-SPI */
    h.Mode = mode;
    HAL_SSP_Init(&h);
}

/* ---- MMC_TIMER_* callbacks: real wall-clock timeouts via pic8-tick ---- */

void pic8_sdcard_timer_start(uint8_t instance, uint16_t timeout_ms)
{
    (void)instance;
    g_timer_start_tick = pic8_tick_get();
    g_timer_timeout_ms = timeout_ms;
}

bool pic8_sdcard_timer_expired(uint8_t instance)
{
    (void)instance;
    return pic8_tick_elapsed_since(g_timer_start_tick) >= g_timer_timeout_ms;
}

void pic8_sdcard_timer_stop(uint8_t instance)
{
    (void)instance;
}

/* ---- public API ---- */

bool pic8_sdcard_init(const pic8_sdcard_pins_t *pins, uint32_t fosc_hz)
{
    g_pins = *pins;
    g_fosc_hz = fosc_hz;

    HAL_GPIO_Init(g_pins.cs_port, g_pins.cs_pin, GPIO_MODE_OUTPUT);
    pic8_sdcard_spi_set_cs(0, 1);   /* deasserted before the SSP is even configured */

    SSP_HandleTypeDef h = SSP_HANDLE_DEFAULT;
    h.Mode = SSP_MODE_SPI_MASTER_FOSC_64;   /* slow starting point; mmc_init_card re-speeds via MMC_SPI_SET_SPEED */
    HAL_SSP_Init(&h);

    g_card.max_speed_hz = 20000000UL;       /* SSP/board ceiling; MIN()'d against the card's own negotiated speed */
    g_card.spi_instance = 0u;
    mmc_init(&g_card, 1u);

    return mmc_init_card(&g_card) == 0;
}

bool pic8_sdcard_ready(void)
{
    return mmc_ready(&g_card);
}

uint32_t pic8_sdcard_num_blocks(void)
{
    return mmc_get_num_blocks(&g_card);
}

bool pic8_sdcard_read_block(uint32_t block_addr, uint8_t *data)
{
    return mmc_read_block(&g_card, block_addr, data) == 0;
}

bool pic8_sdcard_write_block(uint32_t block_addr, const uint8_t *data)
{
    return mmc_write_block(&g_card, block_addr, (uint8_t *)data) == 0;
}
