/**
 * @file    pic16f193x_timer246.c
 * @brief   Timer2/Timer4/Timer6 driver, implementation (DS41364B section 17.0).
 *
 * @details
 *   One driver, three instances. Every macro below branches on `inst`
 *   before touching any SFR, so each branch's own register access is a
 *   literal PIC_REG_* token, mirroring pic18fxx5x_ccp.c's CCP_WRITE_*
 *   and CCP_READ_* shape (docs/adding-a-device.md section 4.8's
 *   proven pattern for runtime-selected-instance dispatch).
 */

#include "peripherals/pic16f193x_timer246.h"
#include "core/pic16f193x_irq.h"

/* Per-instance register access. Branch before touching any SFR. */
#define TIMER246_WRITE_TMR(inst, value)                                    \
    do {                                                                   \
        if ((inst) == TIMER246_INSTANCE_2)      EPIC_REG8(PIC_REG_TMR2) = (uint8_t)(value); \
        else if ((inst) == TIMER246_INSTANCE_4) EPIC_REG8(PIC_REG_TMR4) = (uint8_t)(value); \
        else                                     EPIC_REG8(PIC_REG_TMR6) = (uint8_t)(value); \
    } while (0)

#define TIMER246_READ_TMR(inst, out)                                       \
    do {                                                                   \
        if ((inst) == TIMER246_INSTANCE_2)      (out) = EPIC_REG8(PIC_REG_TMR2); \
        else if ((inst) == TIMER246_INSTANCE_4) (out) = EPIC_REG8(PIC_REG_TMR4); \
        else                                     (out) = EPIC_REG8(PIC_REG_TMR6); \
    } while (0)

#define TIMER246_WRITE_PR(inst, value)                                     \
    do {                                                                   \
        if ((inst) == TIMER246_INSTANCE_2)      EPIC_REG8(PIC_REG_PR2) = (uint8_t)(value); \
        else if ((inst) == TIMER246_INSTANCE_4) EPIC_REG8(PIC_REG_PR4) = (uint8_t)(value); \
        else                                     EPIC_REG8(PIC_REG_PR6) = (uint8_t)(value); \
    } while (0)

#define TIMER246_READ_PR(inst, out)                                        \
    do {                                                                   \
        if ((inst) == TIMER246_INSTANCE_2)      (out) = EPIC_REG8(PIC_REG_PR2); \
        else if ((inst) == TIMER246_INSTANCE_4) (out) = EPIC_REG8(PIC_REG_PR4); \
        else                                     (out) = EPIC_REG8(PIC_REG_PR6); \
    } while (0)

#define TIMER246_WRITE_CON(inst, value)                                    \
    do {                                                                   \
        if ((inst) == TIMER246_INSTANCE_2)      EPIC_REG8(PIC_REG_T2CON) = (uint8_t)(value); \
        else if ((inst) == TIMER246_INSTANCE_4) EPIC_REG8(PIC_REG_T4CON) = (uint8_t)(value); \
        else                                     EPIC_REG8(PIC_REG_T6CON) = (uint8_t)(value); \
    } while (0)

#define TIMER246_STOP(inst)                                                \
    do {                                                                   \
        if ((inst) == TIMER246_INSTANCE_2)      EPIC_BIT_CLR(EPIC_REG8(PIC_REG_T2CON), PIC_T2CON_TMR2ON); \
        else if ((inst) == TIMER246_INSTANCE_4) EPIC_BIT_CLR(EPIC_REG8(PIC_REG_T4CON), PIC_T4CON_TMR4ON); \
        else                                     EPIC_BIT_CLR(EPIC_REG8(PIC_REG_T6CON), PIC_T6CON_TMR6ON); \
    } while (0)

/* T*CON prescaler, DS41364B §17.0: 00 -> 1:1, 01 -> 1:4, 1x -> 1:16.
 * Same ratio table shape as pic16f87xa_timer2.c, independently
 * re-derived for this family, not copied. */
static const uint8_t pre_ratio[4] = { 1, 4, 16, 16 };

/* T*CON postscaler, DS41364B §17.0: 1:(N+1), linear. */
static const uint8_t post_ratio[16] = {
    1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16
};

/* One handle slot per instance. idx_of() is a plain small-enum lookup,
 * not an SFR address, so a runtime branch here is safe (same
 * reasoning as pic18fxx5x_ccp.c's ccp_irq() helper). */
static const TIMER246_HandleTypeDef *g_handle[3] = { NULL, NULL, NULL };

static int idx_of(TIMER246_InstanceTypeDef inst)
{
    if (inst == TIMER246_INSTANCE_2) return 0;
    if (inst == TIMER246_INSTANCE_4) return 1;
    return 2;
}

static PIC16F193X_IRQn timer246_irq(TIMER246_InstanceTypeDef inst)
{
    if (inst == TIMER246_INSTANCE_2) return PIC16F193X_IRQ_TMR2;
    if (inst == TIMER246_INSTANCE_4) return PIC16F193X_IRQ_TMR4;
    return PIC16F193X_IRQ_TMR6;
}

static uint8_t con_por_value(TIMER246_InstanceTypeDef inst)
{
    if (inst == TIMER246_INSTANCE_2) return PIC_T2CON_POR_VALUE;
    if (inst == TIMER246_INSTANCE_4) return PIC_T4CON_POR_VALUE;
    return PIC_T6CON_POR_VALUE;
}

uint8_t EPIC_TIMER246_ReadCounter(TIMER246_InstanceTypeDef inst)
{
    uint8_t v;
    TIMER246_READ_TMR(inst, v);
    return v;
}

void EPIC_TIMER246_WriteCounter(TIMER246_InstanceTypeDef inst, uint8_t value)
{
    TIMER246_WRITE_TMR(inst, value);
}

uint8_t EPIC_TIMER246_ReadPeriod(TIMER246_InstanceTypeDef inst)
{
    uint8_t v;
    TIMER246_READ_PR(inst, v);
    return v;
}

void EPIC_TIMER246_WritePeriod(TIMER246_InstanceTypeDef inst, uint8_t period)
{
    TIMER246_WRITE_PR(inst, period);
}

uint16_t EPIC_TIMER246_PrescalerToRatio(TIMER246_PrescalerTypeDef p)
{
    if ((unsigned)p > 3U) return 1U;
    return pre_ratio[p];
}

uint16_t EPIC_TIMER246_PostscalerToRatio(TIMER246_PostscalerTypeDef p)
{
    if ((unsigned)p > 15U) return 1U;
    return post_ratio[p];
}

EPIC_StatusTypeDef EPIC_TIMER246_Init(const TIMER246_HandleTypeDef *h)
{
    if (!h) return EPIC_INVALID;
    if (h->Instance != TIMER246_INSTANCE_2 &&
        h->Instance != TIMER246_INSTANCE_4 &&
        h->Instance != TIMER246_INSTANCE_6) {
        return EPIC_INVALID;
    }

    /* Stop the timer before reconfiguring. */
    TIMER246_STOP(h->Instance);

    PIC16F193X_IRQn irq = timer246_irq(h->Instance);
    EPIC_IRQ_ClearFlag(irq);
    if (h->OverflowCallback) {
        EPIC_IRQ_Enable(irq);
    } else {
        EPIC_IRQ_DisableSrc(irq);
    }

    g_handle[idx_of(h->Instance)] = h;
    return EPIC_OK;
}

EPIC_StatusTypeDef EPIC_TIMER246_DeInit(TIMER246_InstanceTypeDef inst)
{
    PIC16F193X_IRQn irq = timer246_irq(inst);
    EPIC_IRQ_DisableSrc(irq);
    EPIC_IRQ_ClearFlag(irq);
    TIMER246_WRITE_CON(inst, con_por_value(inst));
    TIMER246_WRITE_TMR(inst, 0x00U);
    TIMER246_WRITE_PR(inst, 0xFFU);   /* PRx POR value, DS41364B §17.0. */
    g_handle[idx_of(inst)] = NULL;
    return EPIC_OK;
}

EPIC_StatusTypeDef EPIC_TIMER246_Start(const TIMER246_HandleTypeDef *h)
{
    if (!h) return EPIC_INVALID;
    if (h->Instance != TIMER246_INSTANCE_2 &&
        h->Instance != TIMER246_INSTANCE_4 &&
        h->Instance != TIMER246_INSTANCE_6) {
        return EPIC_INVALID;
    }

    /* Period register first, same rationale as pic16f87xa_timer2.c:
     * set PRx before enabling TMRxON to avoid a spurious first match. */
    TIMER246_WRITE_PR(h->Instance, h->Period);
    TIMER246_WRITE_TMR(h->Instance, 0x00U);

    /* Build T*CON: T*OUTPS<3:0> -> bits 6:3, TMR*ON -> bit 2,
     * T*CKPS<1:0> -> bits 1:0. */
    uint8_t v = 0U;
    v |= (uint8_t)((h->Postscaler & 0xFU) << 3);
    v |= (uint8_t)(1U << 2);              /* TMR*ON. Same bit (2) on all
                                            * three registers, so this
                                            * literal is safe without an
                                            * instance branch. */
    v |= (uint8_t)(h->Prescaler & 0x3U);
    TIMER246_WRITE_CON(h->Instance, v);

    return EPIC_OK;
}

EPIC_StatusTypeDef EPIC_TIMER246_Stop(TIMER246_InstanceTypeDef inst)
{
    TIMER246_STOP(inst);
    return EPIC_OK;
}

static void timer246_irq_common(TIMER246_InstanceTypeDef inst, PIC16F193X_IRQn irq)
{
    /* Direct flag ops (class-F: the table route clobbers PCLATH in ISR
     * context; the ccp_irq_common switch is the reference). TMR2IF is
     * PIR1 bit 1, TMR4IF/TMR6IF are PIR3 bits 1/5. */
    switch (irq) {
    case PIC16F193X_IRQ_TMR2:
        if (!(EPIC_REG8(PIC_REG_PIR1) & PIC_PIR1_TMR2IF)) return;
        EPIC_BIT_CLR(EPIC_REG8(PIC_REG_PIR1), PIC_PIR1_TMR2IF);
        break;
    case PIC16F193X_IRQ_TMR4:
        if (!(EPIC_REG8(PIC_REG_PIR3) & PIC_PIR3_TMR4IF)) return;
        EPIC_BIT_CLR(EPIC_REG8(PIC_REG_PIR3), PIC_PIR3_TMR4IF);
        break;
    default:
        if (!(EPIC_REG8(PIC_REG_PIR3) & PIC_PIR3_TMR6IF)) return;
        EPIC_BIT_CLR(EPIC_REG8(PIC_REG_PIR3), PIC_PIR3_TMR6IF);
        break;
    }
    const TIMER246_HandleTypeDef *h = g_handle[idx_of(inst)];
    if (h && h->OverflowCallback) {
        h->OverflowCallback();
    }
}

void TIMER2_IRQHandler(void) { timer246_irq_common(TIMER246_INSTANCE_2, PIC16F193X_IRQ_TMR2); }
void TIMER4_IRQHandler(void) { timer246_irq_common(TIMER246_INSTANCE_4, PIC16F193X_IRQ_TMR4); }
void TIMER6_IRQHandler(void) { timer246_irq_common(TIMER246_INSTANCE_6, PIC16F193X_IRQ_TMR6); }
