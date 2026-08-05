/**
 * @file    epic_lcd_mock_transport.h
 * @brief   Host-only mock transport for epic_lcd tests. Records every
 *          command and data byte sent, with RS state, for assertion.
 */

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

void    mock_reset(void);
void    mock_ops_init(epic_lcd_ops_t *ops, void **ctx);

uint16_t       mock_log_len(void);
const mock_entry_t *mock_log_entry(uint16_t i);

#endif /* EPIC_LCD_MOCK_TRANSPORT_H */
