#include "epic_lcd_mock_transport.h"
#include <string.h>

static mock_ctx_t g_mock;

/**
 * @brief Record a sent byte with its RS state into the mock log.
 */
static void mock_send(void *ctx, uint8_t rs, uint8_t byte)
{
    (void)ctx;
    if (g_mock.log_len < MOCK_LOG_CAP) {
        g_mock.log[g_mock.log_len].rs   = rs;
        g_mock.log[g_mock.log_len].byte = byte;
        g_mock.log_len++;
    }
}

/** @brief Mock delay: no-op. */
static void mock_delay_us(void *ctx, uint32_t us) { (void)ctx; (void)us; }
/** @brief Mock delay: no-op. */
static void mock_delay_ms(void *ctx, uint32_t ms) { (void)ctx; (void)ms; }

/**
 * @brief Clear the mock log and state.
 */
void mock_reset(void)
{
    memset(&g_mock, 0, sizeof(g_mock));
}

/**
 * @brief Bind the mock send/delay ops into an ops struct.
 */
void mock_ops_init(epic_lcd_ops_t *ops, void **ctx)
{
    (void)ctx;
    ops->send     = mock_send;
    ops->delay_us = mock_delay_us;
    ops->delay_ms = mock_delay_ms;
}

/**
 * @brief Return the number of bytes recorded in the mock log.
 */
uint16_t mock_log_len(void)
{
    return g_mock.log_len;
}

/**
 * @brief Return the i-th recorded log entry, or NULL past the end.
 */
const mock_entry_t *mock_log_entry(uint16_t i)
{
    if (i >= g_mock.log_len) return NULL;
    return &g_mock.log[i];
}
