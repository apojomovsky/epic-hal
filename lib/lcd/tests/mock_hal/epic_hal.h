/** Host stub of the HAL GPIO/SSP surface the LCD transports use.

Resolves in place of epic_hal.h for the host transport tests only
(CMake puts tests/mock_hal first on those targets' include path).
Records every call so the tests can assert the exact pin and SPI
sequence a transport emits. Values mirror the real headers. */

#ifndef EPIC_HAL_H
#define EPIC_HAL_H

#include <stdbool.h>
#include <stdint.h>

typedef enum {
    EPIC_OK    = 0,
    EPIC_ERROR = 1,
} EPIC_StatusTypeDef;

typedef enum {
    GPIOA = 0,
    GPIOB = 1,
    GPIOC = 2,
    GPIOD = 3,
    GPIOE = 4,
} GPIO_TypeDef;

#define GPIO_PIN_0   (0x01u)
#define GPIO_PIN_1   (0x02u)
#define GPIO_PIN_2   (0x04u)
#define GPIO_PIN_3   (0x08u)
#define GPIO_PIN_4   (0x10u)
#define GPIO_PIN_5   (0x20u)
#define GPIO_PIN_6   (0x40u)
#define GPIO_PIN_7   (0x80u)
#define GPIO_PIN_All (0xFFu)

typedef enum {
    GPIO_PIN_RESET = 0u,
    GPIO_PIN_SET   = 1u,
} GPIO_PinState;

typedef enum {
    GPIO_MODE_INPUT  = 0x1u,
    GPIO_MODE_OUTPUT = 0x2u,
    GPIO_MODE_ANALOG = 0x3u,
} GPIO_ModeTypeDef;

/** @brief Record a pin direction setup. */
void EPIC_GPIO_Init(GPIO_TypeDef port, uint16_t pins, GPIO_ModeTypeDef mode);
/** @brief Record a pin level drive. */
void EPIC_GPIO_WritePin(GPIO_TypeDef port, uint16_t pins, GPIO_PinState state);

typedef enum {
    SSP_MODE_SPI_MASTER_FOSC_4  = 0x0u,
    SSP_MODE_SPI_MASTER_FOSC_16 = 0x1u,
    SSP_MODE_SPI_MASTER_FOSC_64 = 0x2u,
    SSP_MODE_SPI_MASTER_TMR2    = 0x3u,
} SSP_ModeTypeDef;

typedef struct {
    SSP_ModeTypeDef Mode;
} SSP_HandleTypeDef;

#define SSP_HANDLE_DEFAULT { .Mode = SSP_MODE_SPI_MASTER_FOSC_4 }

/** @brief Record an SSP init and its mode. */
EPIC_StatusTypeDef EPIC_SSP_Init(const SSP_HandleTypeDef *h);
/** @brief Record a transmitted byte. */
uint16_t EPIC_SSP_WriteByte(uint8_t data);

#endif /* EPIC_HAL_H */
