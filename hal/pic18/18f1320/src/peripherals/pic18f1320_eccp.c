/*
 * ECCP1 (Enhanced CCP) driver, implementation (DS39605F §15.0). Single
 * Enhanced CCP1 module (no CCP2): capture/compare/PWM with P1M multi-output
 * PWM, PWM1CON dead-band/auto-restart and ECCPAS auto-shutdown.
 */

#include "peripherals/pic18f1320_eccp.h"
#include "core/pic18_irq.h"

/* The only CCP instance on this part is CCP1. Register access uses literal
 * PIC_REG_* tokens; CCP2 does not exist, so all access is direct through the
 * single CCP1CON/CCPR1 pair (no per-instance branching needed). */

/** Per-instance storage (one slot; CCP1). COPIES the caller's handle
 *  (dangling-pointer rationale, see Timer1). The weak ISR reads from this. */
static CCP_HandleTypeDef        g_ccp_storage[3];
static const CCP_HandleTypeDef *g_ccp_handles[3] = { NULL, NULL, NULL };

/**
 * @brief  Encode a PinState enum into the 2-bit PSS field value
 *         (0b00/01/10).
 * @param s Pin state to encode (DRIVE_0, DRIVE_1 or TRISTATE).
 * @return The 2-bit PSS encoding of `s`.
 */
static uint8_t pss_encode(CCP_PinStateTypeDef s)
{
    /* DRIVE_0 -> 00, DRIVE_1 -> 01, TRISTATE -> 10 (1x). */
    return (uint8_t)(s & 0x3U);
}

/**
 * @brief  Initialize the ECCP1 module from a handle. Stores an owned copy
 *         of the handle, clears and optionally enables the CCP1 IRQ, then
 *         programs the 16-bit CCPR1 value and mode (PWM duty or
 *         capture/compare), plus PWM1CON dead-band and ECCPAS
 *         auto-shutdown.
 * @param h Handle describing the ECCP1 configuration.
 * @return EPIC_OK on success, EPIC_INVALID if `h` is NULL or its Instance
 *         is not CCP_INSTANCE_1.
 */
EPIC_StatusTypeDef EPIC_CCP_Init(const CCP_HandleTypeDef *h)
{
    if (!h) return EPIC_INVALID;
    if (h->Instance != CCP_INSTANCE_1)
    {
        return EPIC_INVALID;
    }
    g_ccp_storage[CCP_INSTANCE_1] = *h;
    g_ccp_handles[CCP_INSTANCE_1] = &g_ccp_storage[CCP_INSTANCE_1];

    /* Clear/rearm the IRQ before reconfiguring. */
    EPIC_IRQ_ClearFlag(PIC18_IRQ_CCP1);
    if (h->EventCallback)
    {
        EPIC_IRQ_Enable(PIC18_IRQ_CCP1);
    }
    else
    {
        EPIC_IRQ_DisableSrc(PIC18_IRQ_CCP1);
    }

    if (h->Mode == CCP_MODE_PWM)
    {
        /* DS39605F §15.5.3 step 2: set the PWM duty BEFORE enabling PWM.
         * 10-bit duty: CCPR1L = duty[9:2], CCP1CON<5:4> = duty[1:0]. */
        uint16_t duty = (uint16_t)(h->PWM.Duty & 0x03FFU);
        uint8_t  con  = (uint8_t)(h->Mode & PIC_CCP1_M_MASK);   /* 11xx */
        con |= (uint8_t)((duty & 0x03U) << 4);                   /* duty[1:0] */
        con |= (uint8_t)((h->PWMOutputMode & 0x3U) << 6);        /* P1M[7:6] */
        EPIC_REG8(PIC_REG_CCPR1L) = (uint8_t)(duty >> 2);
        EPIC_REG8(PIC_REG_CCPR1H) = 0x00U;
        EPIC_REG8(PIC_REG_CCP1CON) = con;
    }
    else
    {
        /* Capture / compare: write the 16-bit value then enable mode. */
        EPIC_REG8(PIC_REG_CCPR1H) = (uint8_t)(h->CompareValue >> 8);
        EPIC_REG8(PIC_REG_CCPR1L) = (uint8_t)(h->CompareValue & 0xFFU);
        uint8_t con = (uint8_t)(h->Mode & PIC_CCP1_M_MASK);
        con |= (uint8_t)((h->PWMOutputMode & 0x3U) << 6);         /* P1M[7:6] */
        EPIC_REG8(PIC_REG_CCP1CON) = con;
    }

    /* Dead-band + auto-restart (PWM1CON) and auto-shutdown (ECCPAS). */
    uint8_t del = (uint8_t)(h->DeadBand.Delay & PIC_PWM1CON_PDC_MASK);
    if (h->DeadBand.AutoRestart) del |= PIC_PWM1CON_PRSEN;
    EPIC_REG8(PIC_REG_PWM1CON) = del;
    /* ECCPAS: source[6:4] | PSSAC[3:2] | PSSBD[1:0]; ECCPASE (bit 7) is
     * the status bit, left 0 here. */
    uint8_t asv = (uint8_t)(((h->AutoShutdown.Source & 0x7U) << 4) |
                            (pss_encode(h->AutoShutdown.PinsAC) << 2) |
                            pss_encode(h->AutoShutdown.PinsBD));
    EPIC_REG8(PIC_REG_ECCPAS) = asv;

    return EPIC_OK;
}

/**
 * @brief  Reset the ECCP1 module: clear the CCP1 flag, disable the IRQ,
 *         reset CCP1CON/PWM1CON/ECCPAS to 0x00 and drop the stored handle.
 * @param inst CCP module (CCP_INSTANCE_1 only).
 * @return EPIC_OK on success, EPIC_INVALID for another instance.
 */
EPIC_StatusTypeDef EPIC_CCP_DeInit(CCP_InstanceTypeDef inst)
{
    if (inst != CCP_INSTANCE_1) return EPIC_INVALID;
    EPIC_IRQ_DisableSrc(PIC18_IRQ_CCP1);
    EPIC_IRQ_ClearFlag(PIC18_IRQ_CCP1);
    EPIC_REG8(PIC_REG_CCP1CON)  = PIC_CCP1CON_POR_VALUE;
    EPIC_REG8(PIC_REG_PWM1CON)  = PIC_PWM1CON_POR_VALUE;
    EPIC_REG8(PIC_REG_ECCPAS)   = PIC_ECCPAS_POR_VALUE;
    g_ccp_handles[CCP_INSTANCE_1] = NULL;
    return EPIC_OK;
}

/**
 * @brief  Set the 16-bit CCPR1 compare value, high byte first to avoid a
 *         spurious compare match (DS39605F §15.4). No-op for an unknown
 *         instance.
 * @param inst CCP module (CCP_INSTANCE_1 only).
 * @param value 16-bit compare value to load.
 */
void EPIC_CCP_SetCompare(CCP_InstanceTypeDef inst, uint16_t value)
{
    if (inst != CCP_INSTANCE_1) return;
    /* High byte first to avoid a spurious compare match (DS39605F §15.4). */
    EPIC_REG8(PIC_REG_CCPR1H) = (uint8_t)(value >> 8);
    EPIC_REG8(PIC_REG_CCPR1L) = (uint8_t)(value & 0xFFU);
}

/**
 * @brief  Change only CCP1CON's mode field, leaving CCPR1 and the IRQ
 *         enable state untouched. Cheap, so it suits repeated in-frame
 *         mode switches.
 * @param inst CCP module (CCP_INSTANCE_1 only).
 * @param mode New CCP mode to write into the mode field.
 */
void EPIC_CCP_SetMode(CCP_InstanceTypeDef inst, CCP_ModeTypeDef mode)
{
    if (inst != CCP_INSTANCE_1) return;
    EPIC_REG8(PIC_REG_CCP1CON) = (uint8_t)(mode & 0x0FU);
}

/**
 * @brief  Atomically read the 16-bit CCPR1 capture value using the
 *         high-low-high idiom.
 * @param inst CCP module (CCP_INSTANCE_1 only).
 * @return The 16-bit captured value, or 0 for an unknown instance.
 */
uint16_t EPIC_CCP_GetCapture(CCP_InstanceTypeDef inst)
{
    if (inst != CCP_INSTANCE_1) return 0U;
    uint8_t lo, hi1, hi2;
    do
    {
        hi1 = EPIC_REG8(PIC_REG_CCPR1H);
        lo  = EPIC_REG8(PIC_REG_CCPR1L);
        hi2 = EPIC_REG8(PIC_REG_CCPR1H);
    } while (hi1 != hi2);
    return (uint16_t)(((uint16_t)hi2 << 8) | lo);
}

/**
 * @brief  Set the PWM duty in 10-bit units (0..1023). Writes the LSBs into
 *         CCP1CON<5:4> then CCPR1L (bits 9:2), preserving the mode and P1M
 *         bits (DS39605F §15.5.4).
 * @param inst CCP module (CCP_INSTANCE_1 only).
 * @param duty 10-bit duty cycle value.
 */
void EPIC_CCP_SetPWMDuty(CCP_InstanceTypeDef inst, uint16_t duty)
{
    if (inst != CCP_INSTANCE_1) return;
    duty &= 0x03FFU;
    /* Latch the duty LSBs first (CCP1CON<5:4>), then CCPR1L (bits 9:2),
     * preserving the mode + P1M bits (DS39605F §15.5.4). */
    uint8_t con = (uint8_t)(EPIC_REG8(PIC_REG_CCP1CON) & (uint8_t)~PIC_CCP1_DC1B_MASK);
    con |= (uint8_t)((duty & 0x03U) << 4);
    EPIC_REG8(PIC_REG_CCP1CON) = con;
    EPIC_REG8(PIC_REG_CCPR1L) = (uint8_t)(duty >> 2);
}

/**
 * @brief  Configure the ECCP1 dead-band delay and auto-restart (PWM1CON).
 *         Relevant in half-bridge / full-bridge PWM modes.
 * @param inst CCP module (CCP_INSTANCE_1 only).
 * @param delay Dead-band delay count (PDC<6:0> field).
 * @param auto_restart Whether to auto-restart the PWM after shutdown
 *                     (PRSEN bit).
 */
void EPIC_CCP_ConfigDeadBand(CCP_InstanceTypeDef inst,
                            uint8_t delay, bool auto_restart)
{
    if (inst != CCP_INSTANCE_1) return;
    uint8_t del = (uint8_t)(delay & PIC_PWM1CON_PDC_MASK);
    if (auto_restart) del |= PIC_PWM1CON_PRSEN;
    EPIC_REG8(PIC_REG_PWM1CON) = del;
}

/**
 * @brief  Configure the ECCP1 auto-shutdown source and pin states
 *         (ECCPAS). Pass `CCP_AUTOSHUTDOWN_DISABLED` to turn it off.
 * @param inst CCP module (CCP_INSTANCE_1 only).
 * @param source the auto-shutdown source (a @ref CCP_AutoShutdownSourceTypeDef value).
 * @param pins_ac the P1A/P1C shutdown pin state.
 * @param pins_bd the P1B/P1D shutdown pin state.
 */
void EPIC_CCP_ConfigAutoShutdown(CCP_InstanceTypeDef inst,
                                CCP_AutoShutdownSourceTypeDef source,
                                CCP_PinStateTypeDef pins_ac,
                                CCP_PinStateTypeDef pins_bd)
{
    if (inst != CCP_INSTANCE_1) return;
    /* Preserve ECCPASE (status); reprogram source + pin states. */
    uint8_t asv = (uint8_t)(EPIC_REG8(PIC_REG_ECCPAS) & PIC_ECCPAS_ECCPASE);
    asv |= (uint8_t)(((source & 0x7U) << 4) |
                     (pss_encode(pins_ac) << 2) |
                     pss_encode(pins_bd));
    EPIC_REG8(PIC_REG_ECCPAS) = asv;
}

/**
 * @brief  Returns 1 if an ECCP1 auto-shutdown event is active
 *         (ECCPAS<ECCPASE>).
 * @param inst CCP module (CCP_INSTANCE_1 only).
 * @return 1 while an auto-shutdown event is active, else 0.
 */
uint8_t EPIC_CCP_IsShutdown(CCP_InstanceTypeDef inst)
{
    if (inst != CCP_INSTANCE_1) return 0U;
    return (EPIC_REG8(PIC_REG_ECCPAS) & PIC_ECCPAS_ECCPASE) ? 1U : 0U;
}

/**
 * @brief  Clear the ECCP1 auto-shutdown status (ECCPASE) to restart the PWM.
 *         Only effective when PRSEN = 0 (manual restart); with PRSEN = 1 the
 *         hardware auto-clears when the shutdown source deasserts.
 * @param inst CCP module (CCP_INSTANCE_1 only).
 */
void EPIC_CCP_Restart(CCP_InstanceTypeDef inst)
{
    if (inst != CCP_INSTANCE_1) return;
    /* Clear ECCPASE, preserving the source/pin-state configuration. */
    uint8_t asv = (uint8_t)(EPIC_REG8(PIC_REG_ECCPAS) & (uint8_t)~PIC_ECCPAS_ECCPASE);
    EPIC_REG8(PIC_REG_ECCPAS) = asv;
}

/**
 * @brief  Weak CCP1 (ECCP1) interrupt handler: clears CCP1IF and invokes
 *         the event callback registered via Init.
 */
void CCP1_IRQHandler(void)
{
    EPIC_IRQ_ClearFlag(PIC18_IRQ_CCP1);
    if (g_ccp_handles[CCP_INSTANCE_1] &&
        g_ccp_handles[CCP_INSTANCE_1]->EventCallback)
    {
        g_ccp_handles[CCP_INSTANCE_1]->EventCallback();
    }
}
