/**
 * @file    pic16f87xa_ccp.c
 * @brief   CCP1 / CCP2 driver, implementation (DS39582B §8.0).
 */

#include "peripherals/pic16f87xa_ccp.h"
#include "core/pic16_irq.h"

/**
 * @brief Per-instance address tables. Pulling them out keeps every
 *        function below free of conditionals.
 */
typedef struct {
    uint8_t cprl;   /**< CCPRxL address. */
    uint8_t cprh;   /**< CCPRxH address. */
    uint8_t con;    /**< CCPxCON address. */
    PIC16_IRQn irq;   /**< Interrupt ID. */
} ccp_addrs_t;

static const ccp_addrs_t addrs[2] = {
    /* CCP1, DS39582B Figure 2-3 / §8.0. */
    { 0x15U, 0x16U, 0x17U, PIC16_IRQ_CCP1 },
    /* CCP2, §8.0. */
    { 0x1BU, 0x1CU, 0x1DU, PIC16_IRQ_CCP2 },
};

static const ccp_addrs_t *ccp_sel(CCP_InstanceTypeDef inst)
{
    if (inst == CCP_INSTANCE_2) return &addrs[1];
    return &addrs[0];
}

/* Static handle storage, one per CCP instance. */
static const CCP_HandleTypeDef *g_ccp_handles[3] = { NULL, NULL, NULL };

/* ───────────────────────── public API ───────────────────────────── */

EPIC_StatusTypeDef EPIC_CCP_Init(const CCP_HandleTypeDef *h)
{
    if (!h) return EPIC_INVALID;
    if (h->Instance != CCP_INSTANCE_1 && h->Instance != CCP_INSTANCE_2) {
        return EPIC_INVALID;
    }
    const ccp_addrs_t *a = ccp_sel(h->Instance);
    g_ccp_handles[h->Instance] = h;

    /* Clear the IRQ before reconfiguring. */
    EPIC_IRQ_ClearFlag(a->irq);
    if (h->EventCallback) {
        EPIC_IRQ_Enable(a->irq);
    } else {
        EPIC_IRQ_DisableSrc(a->irq);
    }

    /* For PWM, program duty (10-bit) into CCPRxL + CCPxCON<5:4>.
     * DS39582B §8.3.3 step 2: set the PWM duty BEFORE enabling PWM. */
    if (h->Mode == CCP_MODE_PWM) {
        uint16_t duty = h->PWM.Duty & 0x03FFU;       /* 10-bit clamp. */
        uint8_t  con  = (uint8_t)(h->Mode & 0x0FU);  /* mode 1100, also handles 1101/1110/1111. */
        con |= (uint8_t)((duty & 0x03U) << 4);        /* CCPxY:CCPxX = duty[1:0]. */
        PIC8_REG8(a->cprl) = (uint8_t)(duty >> 2);
        PIC8_REG8(a->cprh) = 0U;
        PIC8_REG8(a->con)  = con;
    } else {
        /* For compare / capture, write the 16-bit value then enable mode. */
        PIC8_REG8(a->cprl) = (uint8_t)(h->CompareValue & 0xFFU);
        PIC8_REG8(a->cprh) = (uint8_t)(h->CompareValue >> 8);
        PIC8_REG8(a->con)  = (uint8_t)(h->Mode & 0x0FU);
    }

    return EPIC_OK;
}

EPIC_StatusTypeDef EPIC_CCP_DeInit(CCP_InstanceTypeDef inst)
{
    if (inst != CCP_INSTANCE_1 && inst != CCP_INSTANCE_2) {
        return EPIC_INVALID;
    }
    const ccp_addrs_t *a = ccp_sel(inst);
    EPIC_IRQ_DisableSrc(a->irq);
    EPIC_IRQ_ClearFlag(a->irq);
    PIC8_REG8(a->con) = 0x00U;
    g_ccp_handles[inst] = NULL;
    return EPIC_OK;
}

void EPIC_CCP_SetCompare(CCP_InstanceTypeDef inst, uint16_t value)
{
    if (inst != CCP_INSTANCE_1 && inst != CCP_INSTANCE_2) return;
    const ccp_addrs_t *a = ccp_sel(inst);
    /* DS39582B §8.x: in compare mode a write to CCPRxH could
     * trigger a spurious compare match if the low byte wrote first.
     * Standard PIC16 idiom: write high then low. */
    PIC8_REG8(a->cprh) = (uint8_t)(value >> 8);
    PIC8_REG8(a->cprl) = (uint8_t)(value & 0xFFU);
}

uint16_t EPIC_CCP_GetCapture(CCP_InstanceTypeDef inst)
{
    if (inst != CCP_INSTANCE_1 && inst != CCP_INSTANCE_2) return 0U;
    const ccp_addrs_t *a = ccp_sel(inst);
    /* Same atomic-read idiom as Timer1. */
    uint8_t lo, hi1, hi2;
    do {
        hi1 = PIC8_REG8(a->cprh);
        lo  = PIC8_REG8(a->cprl);
        hi2 = PIC8_REG8(a->cprh);
    } while (hi1 != hi2);
    return (uint16_t)(((uint16_t)hi2 << 8) | lo);
}

void EPIC_CCP_SetPWMDuty(CCP_InstanceTypeDef inst, uint16_t duty)
{
    if (inst != CCP_INSTANCE_1 && inst != CCP_INSTANCE_2) return;
    const ccp_addrs_t *a = ccp_sel(inst);
    duty &= 0x03FFU;
    /* The duty LSBs go into CCPxCON<5:4>; CCPRxL holds bits 9:2.
     * DS39582B §8.3.2: latch ordering is important; write LSBs
     * of duty first (in CCPxCON), then CCPRxL, to avoid a glitch. */
    uint8_t con = (uint8_t)(PIC8_REG8(a->con) & 0xCFU); /* keep mode bits, clear Y:X. */
    con |= (uint8_t)((duty & 0x03U) << 4);
    PIC8_REG8(a->con)  = con;
    PIC8_REG8(a->cprl) = (uint8_t)(duty >> 2);
}

/* ───────────────────────── ISRs ─────────────────────────────────── */

void CCP1_IRQHandler(void)
{
    if (!EPIC_IRQ_GetFlag(PIC16_IRQ_CCP1)) return;
    EPIC_IRQ_ClearFlag(PIC16_IRQ_CCP1);
    if (g_ccp_handles[CCP_INSTANCE_1] &&
        g_ccp_handles[CCP_INSTANCE_1]->EventCallback) {
        g_ccp_handles[CCP_INSTANCE_1]->EventCallback();
    }
}

void CCP2_IRQHandler(void)
{
    if (!EPIC_IRQ_GetFlag(PIC16_IRQ_CCP2)) return;
    EPIC_IRQ_ClearFlag(PIC16_IRQ_CCP2);
    if (g_ccp_handles[CCP_INSTANCE_2] &&
        g_ccp_handles[CCP_INSTANCE_2]->EventCallback) {
        g_ccp_handles[CCP_INSTANCE_2]->EventCallback();
    }
}
