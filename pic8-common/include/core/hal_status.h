/**
 * @file    core/hal_status.h
 * @brief   Status codes and bit macros shared, unmodified, by every 8-bit
 *          PIC HAL family: architecture-blind, mirrors STM32Cube's
 *          `HAL_StatusTypeDef` so consumer code never sees a
 *          family-specific status enum or bit macro.
 */

#ifndef HAL_STATUS_H
#define HAL_STATUS_H

#include <stdint.h>

/* ───────────────────────── HAL status / error codes ─────────────── */

/**
 * @brief   Standard HAL status codes. Mirrors `HAL_StatusTypeDef` from
 *          STM32Cube so users familiar with that HAL get the same flow.
 *          Identical on every 8-bit PIC family.
 */
typedef enum {
    HAL_OK      = 0x00U, /**< Operation completed successfully. */
    HAL_ERROR   = 0x01U, /**< Generic error. */
    HAL_BUSY    = 0x02U, /**< Resource busy with ongoing operation. */
    HAL_TIMEOUT = 0x03U, /**< Operation timed out. */
    HAL_INVALID = 0x04U  /**< Invalid parameter or state. */
} HAL_StatusTypeDef;

/* ───────────────────────── bit / register helpers ───────────────── */

/**
 * @name    Bit manipulation
 * @brief   Standard set/clr/test helpers, preferred over hand-rolled masks.
 *          The `PIC8_` prefix marks them as shared across every family.
 * @{
 */
#define PIC8_BIT(n)                  (1U << (n))
#define PIC8_BIT_SET(reg, mask)     ((reg) |=  (uint8_t)(mask))
#define PIC8_BIT_CLR(reg, mask)     ((reg) &= ~(uint8_t)(mask))
#define PIC8_BIT_TGL(reg, mask)      ((reg) ^=  (uint8_t)(mask))
#define PIC8_BIT_READ(reg, mask)     ((reg) &   (uint8_t)(mask))
/** @} */

#endif /* HAL_STATUS_H */
