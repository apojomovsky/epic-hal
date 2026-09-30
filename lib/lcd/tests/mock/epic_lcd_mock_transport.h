/** Host-only mock transport for epic_lcd tests: records every command and
 *  data byte sent, with RS state, for assertion. */

#ifndef EPIC_LCD_MOCK_TRANSPORT_H
#define EPIC_LCD_MOCK_TRANSPORT_H

#include "epic_lcd.h"
#include <stdint.h>
#include <stdbool.h>

#define MOCK_LOG_CAP 256u

typedef struct {
    uint8_t rs;
    uint8_t byte;
} mock_entry_t;

typedef struct {
    mock_entry_t log[MOCK_LOG_CAP];
    uint16_t     log_len;
} mock_ctx_t;

/**
 * @brief Clear the mock log and state.
 */
void    mock_reset(void);
/**
 * @brief Bind the mock send/delay ops into an ops struct.
 */
void    mock_ops_init(epic_lcd_ops_t *ops, void **ctx);

/**
 * @brief Return the number of bytes recorded in the mock log.
 */
uint16_t       mock_log_len(void);
/**
 * @brief Return the i-th recorded log entry, or NULL past the end.
 */
const mock_entry_t *mock_log_entry(uint16_t i);

#endif /* EPIC_LCD_MOCK_TRANSPORT_H */
