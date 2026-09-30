/*
 * CCP driver (DS39609B §16.0, DS39661 §16.0 for the ECAN quads):
 * five modules, CCP1-2 only on the quads (same addresses). Every SFR
 * access branches on the instance first, so each branch stays a literal
 * `PIC_REG_*` token (the §4 rule, same shape as the 193x/2520 drivers).
 */

#include "peripherals/pic18f6520_ccp.h"
#include "core/pic18_irq.h"

/* Per-instance register access. Each macro branches on `inst` before
 * touching any SFR, so the address is always a literal `PIC_REG_*` token,
 * never a runtime value (see `pic18_irq.c`'s file header for why that
 * matters on PIC18). */
#define CCP_WRITE_CPRL(inst, value)                                     \
    do {                                                                \
        switch (inst)                                                   \
        {                                                               \
            case CCP_INSTANCE_1: EPIC_REG8(PIC_REG_CCPR1L) = (uint8_t)(value); break; \
            case CCP_INSTANCE_2: EPIC_REG8(PIC_REG_CCPR2L) = (uint8_t)(value); break; \
            case CCP_INSTANCE_3: EPIC_REG8(PIC_REG_CCPR3L) = (uint8_t)(value); break; \
            case CCP_INSTANCE_4: EPIC_REG8(PIC_REG_CCPR4L) = (uint8_t)(value); break; \
            case CCP_INSTANCE_5: EPIC_REG8(PIC_REG_CCPR5L) = (uint8_t)(value); break; \
            default: break;                                             \
        }                                                               \
    } while (0)
#define CCP_WRITE_CPRH(inst, value)                                     \
    do {                                                                \
        switch (inst)                                                   \
        {                                                               \
            case CCP_INSTANCE_1: EPIC_REG8(PIC_REG_CCPR1H) = (uint8_t)(value); break; \
            case CCP_INSTANCE_2: EPIC_REG8(PIC_REG_CCPR2H) = (uint8_t)(value); break; \
            case CCP_INSTANCE_3: EPIC_REG8(PIC_REG_CCPR3H) = (uint8_t)(value); break; \
            case CCP_INSTANCE_4: EPIC_REG8(PIC_REG_CCPR4H) = (uint8_t)(value); break; \
            case CCP_INSTANCE_5: EPIC_REG8(PIC_REG_CCPR5H) = (uint8_t)(value); break; \
            default: break;                                             \
        }                                                               \
    } while (0)
#define CCP_WRITE_CON(inst, value)                                      \
    do {                                                                \
        switch (inst)                                                   \
        {                                                               \
            case CCP_INSTANCE_1: EPIC_REG8(PIC_REG_CCP1CON) = (uint8_t)(value); break; \
            case CCP_INSTANCE_2: EPIC_REG8(PIC_REG_CCP2CON) = (uint8_t)(value); break; \
            case CCP_INSTANCE_3: EPIC_REG8(PIC_REG_CCP3CON) = (uint8_t)(value); break; \
            case CCP_INSTANCE_4: EPIC_REG8(PIC_REG_CCP4CON) = (uint8_t)(value); break; \
            case CCP_INSTANCE_5: EPIC_REG8(PIC_REG_CCP5CON) = (uint8_t)(value); break; \
            default: break;                                             \
        }                                                               \
    } while (0)
#define CCP_READ_CPRL(inst, out)                                        \
    do {                                                                \
        switch (inst)                                                   \
        {                                                               \
            case CCP_INSTANCE_1: (out) = EPIC_REG8(PIC_REG_CCPR1L); break; \
            case CCP_INSTANCE_2: (out) = EPIC_REG8(PIC_REG_CCPR2L); break; \
            case CCP_INSTANCE_3: (out) = EPIC_REG8(PIC_REG_CCPR3L); break; \
            case CCP_INSTANCE_4: (out) = EPIC_REG8(PIC_REG_CCPR4L); break; \
            case CCP_INSTANCE_5: (out) = EPIC_REG8(PIC_REG_CCPR5L); break; \
            default: break;                                             \
        }                                                               \
    } while (0)
#define CCP_READ_CPRH(inst, out)                                        \
    do {                                                                \
        switch (inst)                                                   \
        {                                                               \
            case CCP_INSTANCE_1: (out) = EPIC_REG8(PIC_REG_CCPR1H); break; \
            case CCP_INSTANCE_2: (out) = EPIC_REG8(PIC_REG_CCPR2H); break; \
            case CCP_INSTANCE_3: (out) = EPIC_REG8(PIC_REG_CCPR3H); break; \
            case CCP_INSTANCE_4: (out) = EPIC_REG8(PIC_REG_CCPR4H); break; \
            case CCP_INSTANCE_5: (out) = EPIC_REG8(PIC_REG_CCPR5H); break; \
            default: break;                                             \
        }                                                               \
    } while (0)
#define CCP_READ_CON(inst, out)                                         \
    do {                                                                \
        switch (inst)                                                   \
        {                                                               \
            case CCP_INSTANCE_1: (out) = EPIC_REG8(PIC_REG_CCP1CON); break; \
            case CCP_INSTANCE_2: (out) = EPIC_REG8(PIC_REG_CCP2CON); break; \
            case CCP_INSTANCE_3: (out) = EPIC_REG8(PIC_REG_CCP3CON); break; \
            case CCP_INSTANCE_4: (out) = EPIC_REG8(PIC_REG_CCP4CON); break; \
            case CCP_INSTANCE_5: (out) = EPIC_REG8(PIC_REG_CCP5CON); break; \
            default: break;                                             \
        }                                                               \
    } while (0)

/**
 * @brief  Return 1 if `inst` is a valid CCP instance, else 0. ECAN
 *         quads carry CCP1-2 only (INSTANCE_3..5 rejected).
 * @param inst the instance to validate.
 * @return 1 if valid, else 0.
 */
static uint8_t ccp_valid(CCP_InstanceTypeDef inst)
{
#if PIC18F6520_FAMILY_HAS_CAN
    return (inst == CCP_INSTANCE_1 || inst == CCP_INSTANCE_2) ? 1U : 0U;
#else
    return (inst >= CCP_INSTANCE_1 && inst <= CCP_INSTANCE_5) ? 1U : 0U;
#endif
}

/**
 * @brief  Map a CCP instance to its interrupt ID.
 * @param inst CCP instance (CCP_INSTANCE_1..5).
 * @return PIC18_IRQ_CCP1..CCP5 for instances 1..5, else PIC18_IRQ_CCP1.
 */
static PIC18_IRQn ccp_irq(CCP_InstanceTypeDef inst)
{
    switch (inst)
    {
        case CCP_INSTANCE_1: return PIC18_IRQ_CCP1;
        case CCP_INSTANCE_2: return PIC18_IRQ_CCP2;
        case CCP_INSTANCE_3: return PIC18_IRQ_CCP3;
        case CCP_INSTANCE_4: return PIC18_IRQ_CCP4;
        case CCP_INSTANCE_5: return PIC18_IRQ_CCP5;
        default:             return PIC18_IRQ_CCP1;
    }
}

/* Static handle storage, one per CCP instance. COPIES the caller's handle
 * (dangling-pointer rationale, see Timer1). The weak ISRs read from these. */
static CCP_HandleTypeDef        g_ccp_storage[6];
static const CCP_HandleTypeDef *g_ccp_handles[6] = { NULL, NULL, NULL,
                                                     NULL, NULL, NULL };

/**
 * @brief  Initialize a CCP module from a handle. Stores an owned copy of
 *         the handle, clears and optionally enables the instance IRQ,
 *         then programs the 16-bit CCPRx value and mode (PWM duty or
 *         capture/compare). No ECCP auto-shutdown hardware on this part.
 * @param h Handle describing the CCP instance and its configuration.
 * @return EPIC_OK on success, EPIC_INVALID if `h` is NULL or its Instance
 *         is not CCP_INSTANCE_1..5.
 */
EPIC_StatusTypeDef EPIC_CCP_Init(const CCP_HandleTypeDef *h)
{
    if (!h) return EPIC_INVALID;
    if (!ccp_valid(h->Instance)) return EPIC_INVALID;
    g_ccp_storage[h->Instance] = *h;
    g_ccp_handles[h->Instance] = &g_ccp_storage[h->Instance];

    /* Clear/rearm the IRQ before reconfiguring. */
    EPIC_IRQ_ClearFlag(ccp_irq(h->Instance));
    if (h->EventCallback)
    {
        EPIC_IRQ_Enable(ccp_irq(h->Instance));
    }
    else
    {
        EPIC_IRQ_DisableSrc(ccp_irq(h->Instance));
    }

    if (h->Mode == CCP_MODE_PWM)
    {
        /* DS39609B §16.4.3 step 2: set the PWM duty BEFORE enabling PWM.
         * 10-bit duty: CCPRxL = duty[9:2], CCPxCON<5:4> = duty[1:0]. */
        uint16_t duty = (uint16_t)(h->PWM.Duty & 0x03FFU);
        uint8_t  con  = (uint8_t)(h->Mode & PIC_CCPxCON_CCPxM_MASK);
        con |= (uint8_t)((duty & 0x03U) << 4);
        CCP_WRITE_CPRL(h->Instance, duty >> 2);
        CCP_WRITE_CPRH(h->Instance, 0U);
        CCP_WRITE_CON(h->Instance, con);
    }
    else
    {
        /* Capture / compare: write the 16-bit value then enable mode. */
        CCP_WRITE_CPRH(h->Instance, h->CompareValue >> 8);
        CCP_WRITE_CPRL(h->Instance, h->CompareValue & 0xFFU);
        CCP_WRITE_CON(h->Instance, (uint8_t)(h->Mode & PIC_CCPxCON_CCPxM_MASK));
    }

    return EPIC_OK;
}

/**
 * @brief  Reset a CCP module: clear the PIR flag, disable the IRQ,
 *         reset CCPxCON to POR and drop the stored handle.
 * @param inst CCP instance to de-initialize.
 * @return EPIC_OK on success, EPIC_INVALID for an unknown instance.
 */
EPIC_StatusTypeDef EPIC_CCP_DeInit(CCP_InstanceTypeDef inst)
{
    if (!ccp_valid(inst)) return EPIC_INVALID;
    EPIC_IRQ_DisableSrc(ccp_irq(inst));
    EPIC_IRQ_ClearFlag(ccp_irq(inst));
    CCP_WRITE_CON(inst, PIC_CCPxCON_POR_VALUE);
    g_ccp_handles[inst] = NULL;
    return EPIC_OK;
}

/**
 * @brief  Set the 16-bit CCPRx compare value, high byte first to avoid a
 *         spurious compare match (DS39609B §16.x). No-op for an unknown
 *         instance.
 * @param inst CCP instance.
 * @param value 16-bit compare value to load.
 */
void EPIC_CCP_SetCompare(CCP_InstanceTypeDef inst, uint16_t value)
{
    if (!ccp_valid(inst)) return;
    CCP_WRITE_CPRH(inst, value >> 8);
    CCP_WRITE_CPRL(inst, value & 0xFFU);
}

/**
 * @brief  Change only the CCPxCON mode field, leaving CCPRx and the IRQ
 *         enable state untouched.
 * @param inst CCP instance.
 * @param mode the new mode (a @ref CCP_ModeTypeDef value).
 */
void EPIC_CCP_SetMode(CCP_InstanceTypeDef inst, CCP_ModeTypeDef mode)
{
    if (!ccp_valid(inst)) return;
    uint8_t con = 0U;
    CCP_READ_CON(inst, con);
    con = (uint8_t)((con & (uint8_t)~PIC_CCPxCON_CCPxM_MASK) |
                    (mode & PIC_CCPxCON_CCPxM_MASK));
    CCP_WRITE_CON(inst, con);
}

/**
 * @brief  Atomically read the 16-bit CCPRx capture value. With no RD16
 *         latch on CCP registers, read high then low; CCPR capture is
 *         frozen on the capture event so the pair is coherent.
 * @param inst CCP instance.
 * @return the 16-bit CCPRx value.
 */
uint16_t EPIC_CCP_GetCapture(CCP_InstanceTypeDef inst)
{
    uint8_t hi = 0U, lo = 0U;
    CCP_READ_CPRH(inst, hi);
    CCP_READ_CPRL(inst, lo);
    return (uint16_t)(((uint16_t)hi << 8) | lo);
}

/**
 * @brief  Set PWM duty in 10-bit units (0..1023). Writes the LSBs into
 *         CCPxCON<5:4> then CCPRxL (bits 9:2), preserving the mode bits.
 * @param inst CCP instance.
 * @param duty the 10-bit duty value, 0..1023.
 */
void EPIC_CCP_SetPWMDuty(CCP_InstanceTypeDef inst, uint16_t duty)
{
    if (!ccp_valid(inst)) return;
    uint8_t con = 0U;
    CCP_READ_CON(inst, con);
    con = (uint8_t)((con & (uint8_t)~PIC_CCPxCON_DCxB_MASK) |
                    ((duty & 0x03U) << 4));
    CCP_WRITE_CON(inst, con);
    CCP_WRITE_CPRL(inst, (uint8_t)(duty >> 2));
}

/**
 * @brief  Find the stored handle for an instance, or NULL.
 * @param inst CCP instance.
 * @return the stored handle pointer, or NULL.
 */
static const CCP_HandleTypeDef *ccp_handle(CCP_InstanceTypeDef inst)
{
    if (!ccp_valid(inst)) return NULL;
    return g_ccp_handles[inst];
}

/**
 * @brief  Weak CCP1 interrupt handler: clears CCP1IF and invokes the
 *         event callback registered via Init.
 */
void CCP1_IRQHandler(void)
{
    if (!EPIC_IRQ_GetFlag(PIC18_IRQ_CCP1)) return;
    EPIC_IRQ_ClearFlag(PIC18_IRQ_CCP1);
    const CCP_HandleTypeDef *h = ccp_handle(CCP_INSTANCE_1);
    if (h && h->EventCallback) h->EventCallback();
}

/**
 * @brief  Weak CCP2 interrupt handler.
 */
void CCP2_IRQHandler(void)
{
    if (!EPIC_IRQ_GetFlag(PIC18_IRQ_CCP2)) return;
    EPIC_IRQ_ClearFlag(PIC18_IRQ_CCP2);
    const CCP_HandleTypeDef *h = ccp_handle(CCP_INSTANCE_2);
    if (h && h->EventCallback) h->EventCallback();
}

/**
 * @brief  Weak CCP3 interrupt handler.
 */
void CCP3_IRQHandler(void)
{
    if (!EPIC_IRQ_GetFlag(PIC18_IRQ_CCP3)) return;
    EPIC_IRQ_ClearFlag(PIC18_IRQ_CCP3);
    const CCP_HandleTypeDef *h = ccp_handle(CCP_INSTANCE_3);
    if (h && h->EventCallback) h->EventCallback();
}

/**
 * @brief  Weak CCP4 interrupt handler.
 */
void CCP4_IRQHandler(void)
{
    if (!EPIC_IRQ_GetFlag(PIC18_IRQ_CCP4)) return;
    EPIC_IRQ_ClearFlag(PIC18_IRQ_CCP4);
    const CCP_HandleTypeDef *h = ccp_handle(CCP_INSTANCE_4);
    if (h && h->EventCallback) h->EventCallback();
}

/**
 * @brief  Weak CCP5 interrupt handler.
 */
void CCP5_IRQHandler(void)
{
    if (!EPIC_IRQ_GetFlag(PIC18_IRQ_CCP5)) return;
    EPIC_IRQ_ClearFlag(PIC18_IRQ_CCP5);
    const CCP_HandleTypeDef *h = ccp_handle(CCP_INSTANCE_5);
    if (h && h->EventCallback) h->EventCallback();
}
