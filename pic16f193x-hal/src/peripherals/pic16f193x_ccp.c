/**
 * @file    pic16f193x_ccp.c
 * @brief   PIC16F193X CCP1/CCP2 driver implementation (DS41364B §15.0).
 *
 * @details
 *   One driver, two instances. Every macro below branches on `inst`
 *   before touching any SFR, so each branch's own register access is a
 *   literal PIC_REG_* token, mirroring pic18fxx5x_ccp.c's CCP_WRITE_*
 *   and CCP_READ_* shape (docs/adding-a-device.md section 4.8's proven
 *   pattern for runtime-selected-instance dispatch). The plan's draft
 *   used an instance_regs() helper that returned runtime SFR addresses
 *   via output parameters; that violates the Global Constraint (every
 *   register access is a compile-time-constant PIC_REG_* token), so
 *   this implementation uses the branch-before-touch macro shape
 *   instead, matching the established codebase convention.
 */

#include "peripherals/pic16f193x_ccp.h"
#include "core/pic16f193x_irq.h"

/* Per-instance register access. Branch before touching any SFR. */
#define CCP_WRITE_CPRL(inst, value)                                     \
    do {                                                                \
        if ((inst) == CCP_INSTANCE_1) PIC8_REG8(PIC_REG_CCPR1L) = (uint8_t)(value); \
        else                          PIC8_REG8(PIC_REG_CCPR2L) = (uint8_t)(value); \
    } while (0)

#define CCP_WRITE_CPRH(inst, value)                                     \
    do {                                                                \
        if ((inst) == CCP_INSTANCE_1) PIC8_REG8(PIC_REG_CCPR1H) = (uint8_t)(value); \
        else                          PIC8_REG8(PIC_REG_CCPR2H) = (uint8_t)(value); \
    } while (0)

#define CCP_WRITE_CON(inst, value)                                      \
    do {                                                                \
        if ((inst) == CCP_INSTANCE_1) PIC8_REG8(PIC_REG_CCP1CON) = (uint8_t)(value); \
        else                          PIC8_REG8(PIC_REG_CCP2CON) = (uint8_t)(value); \
    } while (0)

#define CCP_READ_CPRL(inst, out)                                        \
    do {                                                                \
        if ((inst) == CCP_INSTANCE_1) (out) = PIC8_REG8(PIC_REG_CCPR1L); \
        else                          (out) = PIC8_REG8(PIC_REG_CCPR2L); \
    } while (0)

#define CCP_READ_CPRH(inst, out)                                        \
    do {                                                                \
        if ((inst) == CCP_INSTANCE_1) (out) = PIC8_REG8(PIC_REG_CCPR1H); \
        else                          (out) = PIC8_REG8(PIC_REG_CCPR2H); \
    } while (0)

/* The IRQ source isn't an SFR address (just a small enum passed into
 * the already-populated dispatch table), so a plain lookup is safe. */
static PIC16F193X_IRQn ccp_irq(CCP_InstanceTypeDef inst)
{
    return (inst == CCP_INSTANCE_1) ? PIC16F193X_IRQ_CCP1 : PIC16F193X_IRQ_CCP2;
}

/* One handle slot per instance. The weak ISRs read from these. */
static const CCP_HandleTypeDef *g_handle[2] = { NULL, NULL };

static int idx_of(CCP_InstanceTypeDef inst)
{
    return (inst == CCP_INSTANCE_1) ? 0 : 1;
}

HAL_StatusTypeDef HAL_CCP_Init(const CCP_HandleTypeDef *h)
{
    if (!h) return HAL_INVALID;
    if (h->Mode == CCP_MODE_PWM) return HAL_INVALID;  /* Not this phase. */

    /* High byte first, then low: matches this family's atomic-write
     * idiom (pic16f193x_timer1.c's HAL_TIMER1_WriteCounter). */
    CCP_WRITE_CPRH(h->Instance, (uint8_t)(h->CompareValue >> 8));
    CCP_WRITE_CPRL(h->Instance, (uint8_t)(h->CompareValue & 0xFFU));
    CCP_WRITE_CON(h->Instance, (uint8_t)((uint8_t)h->Mode & PIC_CCP1CON_CCPM_MASK));

    PIC16F193X_IRQn irq = ccp_irq(h->Instance);
    HAL_IRQ_ClearFlag(irq);
    if (h->EventCallback) {
        HAL_IRQ_Enable(irq);
    } else {
        HAL_IRQ_DisableSrc(irq);
    }

    g_handle[idx_of(h->Instance)] = h;
    return HAL_OK;
}

HAL_StatusTypeDef HAL_CCP_DeInit(CCP_InstanceTypeDef inst)
{
    PIC16F193X_IRQn irq = ccp_irq(inst);
    HAL_IRQ_DisableSrc(irq);
    HAL_IRQ_ClearFlag(irq);
    CCP_WRITE_CON(inst, 0x00U);
    g_handle[idx_of(inst)] = NULL;
    return HAL_OK;
}

HAL_StatusTypeDef HAL_CCP_SetCompare(CCP_InstanceTypeDef inst, uint16_t value)
{
    CCP_WRITE_CPRH(inst, (uint8_t)(value >> 8));
    CCP_WRITE_CPRL(inst, (uint8_t)(value & 0xFFU));
    return HAL_OK;
}

uint16_t HAL_CCP_GetCapture(CCP_InstanceTypeDef inst)
{
    /* High-low-high retry idiom (pic16f193x_timer1.c's
     * HAL_TIMER1_ReadCounter): guards against a rollover between the
     * two byte reads. */
    uint8_t hi1, lo, hi2;
    do {
        CCP_READ_CPRH(inst, hi1);
        CCP_READ_CPRL(inst, lo);
        CCP_READ_CPRH(inst, hi2);
    } while (hi1 != hi2);
    return (uint16_t)(((uint16_t)hi1 << 8) | lo);
}

static void ccp_irq_common(CCP_InstanceTypeDef inst, PIC16F193X_IRQn irq)
{
    if (!HAL_IRQ_GetFlag(irq)) return;
    HAL_IRQ_ClearFlag(irq);
    const CCP_HandleTypeDef *h = g_handle[idx_of(inst)];
    if (h && h->EventCallback) {
        h->EventCallback();
    }
}

void CCP1_IRQHandler(void) { ccp_irq_common(CCP_INSTANCE_1, PIC16F193X_IRQ_CCP1); }
void CCP2_IRQHandler(void) { ccp_irq_common(CCP_INSTANCE_2, PIC16F193X_IRQ_CCP2); }
