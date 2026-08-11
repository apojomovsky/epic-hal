/* CCP1 / CCP2 driver implementation (DS39582B §8.0). */

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

/**
 * @brief Look up the address table for a CCP instance.
 * @param inst CCP_INSTANCE_1 or CCP_INSTANCE_2.
 * @return pointer to the matching address entry (instance 1 on
 *         invalid input).
 */
static const ccp_addrs_t *ccp_sel(CCP_InstanceTypeDef inst)
{
    if (inst == CCP_INSTANCE_2) return &addrs[1];
    return &addrs[0];
}
/* Driver-owned callback storage, one slot per CCP instance (index 0
 * unused). The IRQ handlers call the stored callback directly: XC8
 * v4.00 bakes handle derefs to IRP=1 (banks 2/3 only), so reading
 * through a caller-provided handle would misread the slot when the
 * handle lives in bank 0/1. A direct array access is bank-correct
 * regardless of where the array lands; the caller's handle is only
 * dereferenced inside EPIC_CCP_Init, where XC8 emits the runtime
 * bank-select form. */
static void (*g_ccp_callbacks[3])(void) = { NULL, NULL, NULL };

/* public API. */

/**
 * @brief Configure the CCP module: program the mode/compare/PWM
 *        registers and arm the interrupt if an event callback is set.
 * @param h handle with Instance, Mode, CompareValue, PWM, EventCallback.
 * @return EPIC_OK on success, EPIC_INVALID on NULL or bad instance.
 */
EPIC_StatusTypeDef EPIC_CCP_Init(const CCP_HandleTypeDef *h)
{
    if (!h) return EPIC_INVALID;
    if (h->Instance != CCP_INSTANCE_1 && h->Instance != CCP_INSTANCE_2) {
        return EPIC_INVALID;
    }
    const ccp_addrs_t *a = ccp_sel(h->Instance);
    /* Copy the callback into driver-owned storage: the IRQ handlers
     * call this copy directly and the caller's handle is not retained
     * (it may be a stack object whose lifetime ends at return). */
    g_ccp_callbacks[h->Instance] = h->EventCallback;

    /* Clear the IRQ before reconfiguring. */
    EPIC_IRQ_ClearFlag(a->irq);
    if (g_ccp_callbacks[h->Instance]) {
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
        EPIC_REG8(a->cprl) = (uint8_t)(duty >> 2);
        EPIC_REG8(a->cprh) = 0U;
        EPIC_REG8(a->con)  = con;
    } else {
        /* For compare / capture, write the 16-bit value then enable mode. */
        EPIC_REG8(a->cprl) = (uint8_t)(h->CompareValue & 0xFFU);
        EPIC_REG8(a->cprh) = (uint8_t)(h->CompareValue >> 8);
        EPIC_REG8(a->con)  = (uint8_t)(h->Mode & 0x0FU);
    }

    return EPIC_OK;
}

/**
 * @brief Reset the CCP module: disable the interrupt, clear the flag
 *        and zero CCPxCON.
 * @param inst which CCP module to de-initialize.
 * @return EPIC_OK on success, EPIC_INVALID on bad instance.
 */
EPIC_StatusTypeDef EPIC_CCP_DeInit(CCP_InstanceTypeDef inst)
{
    if (inst != CCP_INSTANCE_1 && inst != CCP_INSTANCE_2) {
        return EPIC_INVALID;
    }
    const ccp_addrs_t *a = ccp_sel(inst);
    EPIC_IRQ_DisableSrc(a->irq);
    EPIC_IRQ_ClearFlag(a->irq);
    EPIC_REG8(a->con) = 0x00U;
    g_ccp_callbacks[inst] = NULL;
    return EPIC_OK;
}

/**
 * @brief Set the 16-bit CCPRx value (high byte first to avoid a
 *        spurious compare match).
 * @param inst which CCP module to program.
 * @param value the 16-bit compare/capture value.
 */
void EPIC_CCP_SetCompare(CCP_InstanceTypeDef inst, uint16_t value)
{
    if (inst != CCP_INSTANCE_1 && inst != CCP_INSTANCE_2) return;
    const ccp_addrs_t *a = ccp_sel(inst);
    /* DS39582B §8.x: in compare mode a write to CCPRxH could
     * trigger a spurious compare match if the low byte wrote first.
     * Standard PIC16 idiom: write high then low. */
    EPIC_REG8(a->cprh) = (uint8_t)(value >> 8);
    EPIC_REG8(a->cprl) = (uint8_t)(value & 0xFFU);
}

/**
 * @brief Change only the CCPxCON mode field.
 * @param inst which CCP module to reprogram.
 * @param mode the new mode.
 */
void EPIC_CCP_SetMode(CCP_InstanceTypeDef inst, CCP_ModeTypeDef mode)
{
    if (inst != CCP_INSTANCE_1 && inst != CCP_INSTANCE_2) return;
    const ccp_addrs_t *a = ccp_sel(inst);
    EPIC_REG8(a->con) = (uint8_t)(mode & 0x0FU);
}

/**
 * @brief Atomically read the 16-bit CCPRx value.
 * @param inst which CCP module to read.
 * @return the captured/compare value, 0 on invalid instance.
 */
uint16_t EPIC_CCP_GetCapture(CCP_InstanceTypeDef inst)
{
    if (inst != CCP_INSTANCE_1 && inst != CCP_INSTANCE_2) return 0U;
    const ccp_addrs_t *a = ccp_sel(inst);
    /* Same atomic-read idiom as Timer1. */
    uint8_t lo, hi1, hi2;
    do {
        hi1 = EPIC_REG8(a->cprh);
        lo  = EPIC_REG8(a->cprl);
        hi2 = EPIC_REG8(a->cprh);
    } while (hi1 != hi2);
    return (uint16_t)(((uint16_t)hi2 << 8) | lo);
}

/**
 * @brief Set the PWM duty in 10-bit units. Writes the duty LSBs into
 *        CCPxCON<5:4> first, then CCPRxL, to avoid a glitch.
 * @param inst which CCP module to configure.
 * @param duty the 10-bit duty value, 0..1023.
 */
void EPIC_CCP_SetPWMDuty(CCP_InstanceTypeDef inst, uint16_t duty)
{
    if (inst != CCP_INSTANCE_1 && inst != CCP_INSTANCE_2) return;
    const ccp_addrs_t *a = ccp_sel(inst);
    duty &= 0x03FFU;
    /* The duty LSBs go into CCPxCON<5:4>; CCPRxL holds bits 9:2.
     * DS39582B §8.3.2: latch ordering is important; write LSBs
     * of duty first (in CCPxCON), then CCPRxL, to avoid a glitch. */
    uint8_t con = (uint8_t)(EPIC_REG8(a->con) & 0xCFU); /* keep mode bits, clear Y:X. */
    con |= (uint8_t)((duty & 0x03U) << 4);
    EPIC_REG8(a->con)  = con;
    EPIC_REG8(a->cprl) = (uint8_t)(duty >> 2);
}

/* ISRs. */

/**
 * @brief Weak CCP1 ISR: clears CCP1IF and fires the stored callback.
 */
void CCP1_IRQHandler(void)
{
    EPIC_BIT_CLR(EPIC_REG8(PIC_REG_PIR1), PIC_PIR1_CCP1IF);
    if (g_ccp_callbacks[CCP_INSTANCE_1]) {
        g_ccp_callbacks[CCP_INSTANCE_1]();
    }
}

/**
 * @brief Weak CCP2 ISR: clears CCP2IF and fires the stored callback.
 */
void CCP2_IRQHandler(void)
{
    EPIC_BIT_CLR(EPIC_REG8(PIC_REG_PIR2), PIC_PIR2_CCP2IF);
    if (g_ccp_callbacks[CCP_INSTANCE_2]) {
        g_ccp_callbacks[CCP_INSTANCE_2]();
    }
}
