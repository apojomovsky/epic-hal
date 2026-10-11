/**
 * PIC16F1508/1509 CLC contract. Four configurable logic cells, each with four
 * data inputs (D1..D4), four gates built from those inputs (CLCxGLSg), and one
 * output. Bit layout from the pinned DFP 1.9.258 header, not yet checked
 * against the datasheet. Input-source codes (LCxDyS) and LCxMODE stay raw 0..7
 * until the per-device tables are verified.
 */
#ifndef PIC16F1508_CLC_H
#define PIC16F1508_CLC_H

#include <stdint.h>
#include "pic16f1508.h"

typedef enum
{
    CLC1 = 0,
    CLC2 = 1,
    CLC3 = 2,
    CLC4 = 3
} CLC_InstanceTypeDef;

typedef struct
{
    CLC_InstanceTypeDef Instance; /* required: no default handle exists */
    uint8_t Mode;                 /* LCxMODE, 0..7 */
    uint8_t Source[4];            /* D1S..D4S, 0..7 each */
    uint8_t Gls[4];               /* CLCxGLS0..GLS3 images, see EPIC_CLC_GLSInput */
    uint8_t GatePolarity;         /* G1POL..G4POL in bits 3:0, 0..0x0F */
    uint8_t OutputPolarity;       /* nonzero inverts the cell output (POL) */
    uint8_t OutputEnable;         /* nonzero sets LCxOE; pin PPS/TRIS not touched */
} CLC_HandleTypeDef;

/**
 * @brief Configure one CLC cell and enable it.
 * @details Validates the whole handle before writing any register. The cell
 * is disabled while it is reprogrammed, so no half-written configuration
 * reaches the output.
 * @param h Handle to apply. NULL, a bad instance, a field out of range, or
 * a gate polarity above 0x0F returns EPIC_INVALID with no register written.
 * @return EPIC_OK on success, EPIC_INVALID otherwise.
 */
EPIC_StatusTypeDef EPIC_CLC_Init(const CLC_HandleTypeDef *h);

/**
 * @brief Disable one CLC cell and clear its configuration registers.
 * @param instance Cell to clear (CLC1..CLC4).
 * @return EPIC_OK on success, EPIC_INVALID for an unknown instance.
 */
EPIC_StatusTypeDef EPIC_CLC_DeInit(CLC_InstanceTypeDef instance);

/**
 * @brief Read the live output of one CLC cell from CLCDATA.
 * @param instance Cell to read (CLC1..CLC4).
 * @param level Receives 0 or 1. Must not be NULL.
 * @return EPIC_OK on success, EPIC_INVALID for an unknown instance or NULL level.
 */
EPIC_StatusTypeDef EPIC_CLC_ReadOutput(CLC_InstanceTypeDef instance, uint8_t *level);

/**
 * @brief Acknowledge CLC1IF when CLC1 is enabled and pending.
 * @details Called from epic_dispatch_all_irqs only when CLC1IF and CLC1IE are
 * both set. Clears CLC1IF so the vector does not re-enter on the same event.
 */
void CLC1_IRQHandler(void);

/**
 * @brief Acknowledge CLC2IF when CLC2 is enabled and pending.
 * @details Called from epic_dispatch_all_irqs only when CLC2IF and CLC2IE are
 * both set. Clears CLC2IF so the vector does not re-enter on the same event.
 */
void CLC2_IRQHandler(void);

/**
 * @brief Acknowledge CLC3IF when CLC3 is enabled and pending.
 * @details Called from epic_dispatch_all_irqs only when CLC3IF and CLC3IE are
 * both set. Clears CLC3IF so the vector does not re-enter on the same event.
 */
void CLC3_IRQHandler(void);

/**
 * @brief Acknowledge CLC4IF when CLC4 is enabled and pending.
 * @details Called from epic_dispatch_all_irqs only when CLC4IF and CLC4IE are
 * both set. Clears CLC4IF so the vector does not re-enter on the same event.
 */
void CLC4_IRQHandler(void);

#endif
