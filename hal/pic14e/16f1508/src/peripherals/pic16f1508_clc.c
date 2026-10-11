#include <stddef.h>
#include "peripherals/pic16f1508_clc.h"

/* Each cell is an 8-register block: CON, POL, SEL0, SEL1, GLS0..GLS3. */
#define CLC_CELL_STRIDE  8U
#define CLC_OFF_CON      0U
#define CLC_OFF_POL      1U
#define CLC_OFF_SEL0     2U
#define CLC_OFF_SEL1     3U
#define CLC_OFF_GLS0     4U
#define CLC_FIELD_MAX    7U
#define CLC_GATE_POL_MAX 0x0FU

/**
 * @brief Check that an instance is CLC1..CLC4.
 * @param instance Value to check.
 * @return 1 if valid, 0 otherwise.
 */
static uint8_t instance_ok(CLC_InstanceTypeDef instance)
{
    return ((unsigned int)instance <= (unsigned int)CLC4) ? 1U : 0U;
}

/**
 * @brief Address of one register inside a cell block.
 * @param instance Cell (validated by the caller).
 * @param offset Register offset from the cell's CON register.
 * @return SFR address.
 */
static uint16_t cell_reg(CLC_InstanceTypeDef instance, uint8_t offset)
{
    return (uint16_t)(PIC_REG_CLC1CON + ((uint16_t)instance * CLC_CELL_STRIDE) + offset);
}

/**
 * @brief Check every field of a handle against its register width.
 * @param h Handle to check; may be NULL.
 * @return 1 if the handle can be written, 0 otherwise.
 */
static uint8_t handle_ok(const CLC_HandleTypeDef *h)
{
    uint8_t i;

    if (h == NULL)
    {
        return 0U;
    }
    if (!instance_ok(h->Instance) || (h->Mode > CLC_FIELD_MAX))
    {
        return 0U;
    }
    for (i = 0U; i < 4U; i++)
    {
        if (h->Source[i] > CLC_FIELD_MAX)
        {
            return 0U;
        }
    }
    return (h->GatePolarity <= CLC_GATE_POL_MAX) ? 1U : 0U;
}

/**
 * @brief Configure one CLC cell and enable it.
 * @param h Handle to apply.
 * @return EPIC_OK on success, EPIC_INVALID for a bad handle.
 */
EPIC_StatusTypeDef EPIC_CLC_Init(const CLC_HandleTypeDef *h)
{
    uint8_t con;
    uint8_t g;

    if (!handle_ok(h))
    {
        return EPIC_INVALID;
    }

    /* Disabled while reprogrammed, so no half-written configuration reaches the output. */
    EPIC_REG8(cell_reg(h->Instance, CLC_OFF_CON)) = 0U;

    EPIC_REG8(cell_reg(h->Instance, CLC_OFF_POL)) =
        (uint8_t)(h->GatePolarity | ((h->OutputPolarity != 0U) ? PIC_CLCPOL_POL : 0U));
    EPIC_REG8(cell_reg(h->Instance, CLC_OFF_SEL0)) =
        (uint8_t)(h->Source[0] | (uint8_t)(h->Source[1] << PIC_CLCSEL_HIGH_SHIFT));
    EPIC_REG8(cell_reg(h->Instance, CLC_OFF_SEL1)) =
        (uint8_t)(h->Source[2] | (uint8_t)(h->Source[3] << PIC_CLCSEL_HIGH_SHIFT));
    for (g = 0U; g < 4U; g++)
    {
        EPIC_REG8(cell_reg(h->Instance, (uint8_t)(CLC_OFF_GLS0 + g))) = h->Gls[g];
    }

    con = (uint8_t)((h->Mode & PIC_CLCCON_MODE_MASK) | PIC_CLCCON_EN);
    if (h->OutputEnable != 0U)
    {
        con = (uint8_t)(con | PIC_CLCCON_OE);
    }
    EPIC_REG8(cell_reg(h->Instance, CLC_OFF_CON)) = con;
    return EPIC_OK;
}

/**
 * @brief Disable one CLC cell and clear its configuration registers.
 * @param instance Cell to clear.
 * @return EPIC_OK on success, EPIC_INVALID for an unknown instance.
 */
EPIC_StatusTypeDef EPIC_CLC_DeInit(CLC_InstanceTypeDef instance)
{
    uint8_t g;

    if (!instance_ok(instance))
    {
        return EPIC_INVALID;
    }

    /* CON first: the cell stops before its inputs and gates are cleared. */
    EPIC_REG8(cell_reg(instance, CLC_OFF_CON)) = 0U;
    EPIC_REG8(cell_reg(instance, CLC_OFF_POL)) = 0U;
    EPIC_REG8(cell_reg(instance, CLC_OFF_SEL0)) = 0U;
    EPIC_REG8(cell_reg(instance, CLC_OFF_SEL1)) = 0U;
    for (g = 0U; g < 4U; g++)
    {
        EPIC_REG8(cell_reg(instance, (uint8_t)(CLC_OFF_GLS0 + g))) = 0U;
    }
    return EPIC_OK;
}

/**
 * @brief Read the live output of one CLC cell from CLCDATA.
 * @param instance Cell to read.
 * @param level Receives 0 or 1.
 * @return EPIC_OK on success, EPIC_INVALID for an unknown instance or NULL level.
 */
EPIC_StatusTypeDef EPIC_CLC_ReadOutput(CLC_InstanceTypeDef instance, uint8_t *level)
{
    if ((level == NULL) || !instance_ok(instance))
    {
        return EPIC_INVALID;
    }

    *level = (uint8_t)((EPIC_REG8(PIC_REG_CLCDATA) >> (unsigned int)instance) & 1U);
    return EPIC_OK;
}
