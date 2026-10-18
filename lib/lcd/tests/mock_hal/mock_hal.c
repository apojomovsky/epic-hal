/** Call recorder behind the mock_hal stubs: GPIO, SSP, tick logs. */

#include "mock_hal.h"

#include <string.h>

#define GPIO_INIT_CAP  32u
#define GPIO_WRITE_CAP 1024u
#define SSP_WRITE_CAP  512u
#define TICK_MS_CAP    64u

static mock_gpio_init_t  g_inits[GPIO_INIT_CAP];
static unsigned          g_init_len;
static mock_gpio_write_t g_writes[GPIO_WRITE_CAP];
static unsigned          g_write_len;
static uint8_t           g_ssp[SSP_WRITE_CAP];
static unsigned          g_ssp_len;
static unsigned          g_ssp_inits;
static SSP_ModeTypeDef   g_ssp_mode;
static uint32_t          g_tick_ms[TICK_MS_CAP];
static unsigned          g_tick_len;

/** @brief Drop every recorded call. */
void mock_hal_reset(void)
{
    memset(g_inits, 0, sizeof(g_inits));
    memset(g_writes, 0, sizeof(g_writes));
    memset(g_ssp, 0, sizeof(g_ssp));
    memset(g_tick_ms, 0, sizeof(g_tick_ms));
    g_init_len = 0u;
    g_write_len = 0u;
    g_ssp_len = 0u;
    g_ssp_inits = 0u;
    g_ssp_mode = SSP_MODE_SPI_MASTER_FOSC_4;
    g_tick_len = 0u;
}

/** @brief Record a pin direction setup. */
void EPIC_GPIO_Init(GPIO_TypeDef port, uint16_t pins, GPIO_ModeTypeDef mode)
{
    if (g_init_len < GPIO_INIT_CAP)
    {
        g_inits[g_init_len].port = port;
        g_inits[g_init_len].pins = pins;
        g_inits[g_init_len].mode = mode;
        g_init_len++;
    }
}

/** @brief Record a pin level drive. */
void EPIC_GPIO_WritePin(GPIO_TypeDef port, uint16_t pins, GPIO_PinState state)
{
    if (g_write_len < GPIO_WRITE_CAP)
    {
        g_writes[g_write_len].port = port;
        g_writes[g_write_len].pins = pins;
        g_writes[g_write_len].state = state;
        g_write_len++;
    }
}

/** @brief Record an SSP init and its mode. */
EPIC_StatusTypeDef EPIC_SSP_Init(const SSP_HandleTypeDef *h)
{
    g_ssp_inits++;
    if (h != NULL)
    {
        g_ssp_mode = h->Mode;
    }
    return EPIC_OK;
}

/** @brief Record a transmitted byte. */
uint16_t EPIC_SSP_WriteByte(uint8_t data)
{
    if (g_ssp_len < SSP_WRITE_CAP)
    {
        g_ssp[g_ssp_len] = data;
        g_ssp_len++;
    }
    return 0u;
}

/** @brief Record a millisecond delay request. */
void epic_tick_delay_ms(uint32_t ms)
{
    if (g_tick_len < TICK_MS_CAP)
    {
        g_tick_ms[g_tick_len] = ms;
        g_tick_len++;
    }
}

/** @brief Return the number of recorded GPIO init calls. */
unsigned mock_hal_init_len(void)
{
    return g_init_len;
}

/** @brief Return the i-th GPIO init record, or NULL past the end. */
const mock_gpio_init_t *mock_hal_init(unsigned i)
{
    if (i >= g_init_len)
    {
        return NULL;
    }
    return &g_inits[i];
}

/** @brief Return the number of recorded GPIO writes. */
unsigned mock_hal_write_len(void)
{
    return g_write_len;
}

/** @brief Return the i-th GPIO write record, or NULL past the end. */
const mock_gpio_write_t *mock_hal_write(unsigned i)
{
    if (i >= g_write_len)
    {
        return NULL;
    }
    return &g_writes[i];
}

/** @brief Return the number of recorded SSP inits. */
unsigned mock_hal_ssp_init_len(void)
{
    return g_ssp_inits;
}

/** @brief Return the mode of the last recorded SSP init. */
SSP_ModeTypeDef mock_hal_ssp_mode(void)
{
    return g_ssp_mode;
}

/** @brief Return the number of recorded SSP bytes. */
unsigned mock_hal_ssp_len(void)
{
    return g_ssp_len;
}

/** @brief Return the i-th recorded SSP byte (0 past the end). */
uint8_t mock_hal_ssp(unsigned i)
{
    if (i >= g_ssp_len)
    {
        return 0u;
    }
    return g_ssp[i];
}

/** @brief Return the number of recorded delay_ms calls. */
unsigned mock_hal_tick_len(void)
{
    return g_tick_len;
}

/** @brief Return the i-th recorded delay_ms value (0 past the end). */
uint32_t mock_hal_tick(unsigned i)
{
    if (i >= g_tick_len)
    {
        return 0u;
    }
    return g_tick_ms[i];
}
