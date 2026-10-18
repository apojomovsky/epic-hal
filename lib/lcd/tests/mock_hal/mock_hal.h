/** Query API over the mock_hal call recorder. */

#ifndef MOCK_HAL_H
#define MOCK_HAL_H

#include "epic_hal.h"
#include "epic_tick.h"

typedef struct {
    GPIO_TypeDef    port;
    uint16_t        pins;
    GPIO_ModeTypeDef mode;
} mock_gpio_init_t;

typedef struct {
    GPIO_TypeDef  port;
    uint16_t      pins;
    GPIO_PinState state;
} mock_gpio_write_t;

/** @brief Drop every recorded call. */
void mock_hal_reset(void);
/** @brief Return the number of recorded GPIO init calls. */
unsigned mock_hal_init_len(void);
/** @brief Return the i-th GPIO init record, or NULL past the end. */
const mock_gpio_init_t *mock_hal_init(unsigned i);
/** @brief Return the number of recorded GPIO writes. */
unsigned mock_hal_write_len(void);
/** @brief Return the i-th GPIO write record, or NULL past the end. */
const mock_gpio_write_t *mock_hal_write(unsigned i);
/** @brief Return the number of recorded SSP inits. */
unsigned mock_hal_ssp_init_len(void);
/** @brief Return the mode of the last recorded SSP init. */
SSP_ModeTypeDef mock_hal_ssp_mode(void);
/** @brief Return the number of recorded SSP bytes. */
unsigned mock_hal_ssp_len(void);
/** @brief Return the i-th recorded SSP byte (0 past the end). */
uint8_t mock_hal_ssp(unsigned i);
/** @brief Return the number of recorded delay_ms calls. */
unsigned mock_hal_tick_len(void);
/** @brief Return the i-th recorded delay_ms value (0 past the end). */
uint32_t mock_hal_tick(unsigned i);

#endif /* MOCK_HAL_H */
