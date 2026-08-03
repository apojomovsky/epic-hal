/**
 * @file    pic18fxx5x_ccp.c
 * @brief   ECCP1 (Enhanced CCP) + CCP2 driver, implementation
 *          (DS39632E §16.0).
 */

#include "peripherals/pic18fxx5x_ccp.h"
#include "core/pic18_irq.h"

/**
 * @brief Per-instance register access. CCP1 is the enhanced module (has
 *        ECCP1DEL/ECCP1AS); CCP2 is plain.
 *
 * @details
 *   Previously a `ccp_addrs_t` lookup table holding CCPRxL/CCPRxH/CCPxCON
 *   as `uint16_t` fields, dereferenced via a runtime `const ccp_addrs_t *a
 *   = &addrs[inst]; PIC8_REG8(a->cprl) = ...`. Same bug class as
 *   `pic18_irq.c`'s (see `pic18fxx5x-hal/docs/ARCHITECTURE.md` Finding 3):
 *   confirmed via a real-target `mdb` probe that `CCPR1L`/`CCP1CON` stayed
 *   `0` after `HAL_CCP_Init` while `ECCP1DEL` (written through the
 *   compile-time-constant `PIC_REG_ECCP1DEL` directly, never through the
 *   `addrs[]` table) came out correct. XC8 compiles a runtime-addressed
 *   SFR access on PIC18 to its program-memory table read/write mechanism
 *   (`TBLPTR`/`tblrd`/`tblwt`) instead of a data-memory access, so the
 *   table-driven writes silently went nowhere.
 *
 *   Fixed the same way as `pic18_irq.c`: these macros branch on `inst`
 *   *before* touching any SFR, so each branch's `PIC8_REG8`/
 *   `pic8_sfr_read8`/`write8` call always sees a literal `PIC_REG_*`
 *   token, never a value that crossed a function-call or struct-member
 *   boundary as a `uint16_t`. */
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
#define CCP_READ_CON(inst, out)                                         \
    do {                                                                \
        if ((inst) == CCP_INSTANCE_1) (out) = PIC8_REG8(PIC_REG_CCP1CON); \
        else                          (out) = PIC8_REG8(PIC_REG_CCP2CON); \
    } while (0)

/* The interrupt ID isn't an SFR address (just a small enum passed into
 * pic18_irq.c's own, already-fixed dispatch), so a plain lookup is fine
 * here, nothing at risk. */
static PIC18_IRQn ccp_irq(CCP_InstanceTypeDef inst)
{
    return (inst == CCP_INSTANCE_1) ? PIC18_IRQ_CCP1 : PIC18_IRQ_CCP2;
}

/* Static handle storage, one per CCP instance. COPIES the caller's handle
 * (dangling-pointer rationale, see Timer1). The weak ISRs read from these. */
static CCP_HandleTypeDef        g_ccp_storage[3];
static const CCP_HandleTypeDef *g_ccp_handles[3] = { NULL, NULL, NULL };

/** Encode a PinState enum into the 2-bit PSS field value (0b00/01/10). */
static uint8_t pss_encode(CCP_PinStateTypeDef s)
{
    /* DRIVE_0 -> 00, DRIVE_1 -> 01, TRISTATE -> 10 (1x). */
    return (uint8_t)(s & 0x3U);
}

/* ───────────────────────── public API ───────────────────────────── */

HAL_StatusTypeDef HAL_CCP_Init(const CCP_HandleTypeDef *h)
{
    if (!h) return HAL_INVALID;
    if (h->Instance != CCP_INSTANCE_1 && h->Instance != CCP_INSTANCE_2) {
        return HAL_INVALID;
    }
    g_ccp_storage[h->Instance] = *h;
    g_ccp_handles[h->Instance] = &g_ccp_storage[h->Instance];

    /* Clear/rearm the IRQ before reconfiguring. */
    HAL_IRQ_ClearFlag(ccp_irq(h->Instance));
    if (h->EventCallback) {
        HAL_IRQ_Enable(ccp_irq(h->Instance));
    } else {
        HAL_IRQ_DisableSrc(ccp_irq(h->Instance));
    }

    if (h->Mode == CCP_MODE_PWM) {
        /* DS39632E §16.4.3 step 2: set the PWM duty BEFORE enabling PWM.
         * 10-bit duty: CCPRxL = duty[9:2], CCPxCON<5:4> = duty[1:0]. */
        uint16_t duty = (uint16_t)(h->PWM.Duty & 0x03FFU);
        uint8_t  con  = (uint8_t)(h->Mode & PIC_CCP1_M_MASK);   /* 11xx */
        con |= (uint8_t)((duty & 0x03U) << 4);                   /* duty[1:0] */
        if (h->Instance == CCP_INSTANCE_1) {
            con |= (uint8_t)((h->PWMOutputMode & 0x3U) << 6);   /* P1M[7:6] */
        }
        CCP_WRITE_CPRL(h->Instance, duty >> 2);
        CCP_WRITE_CPRH(h->Instance, 0U);
        CCP_WRITE_CON(h->Instance, con);
    } else {
        /* Capture / compare: write the 16-bit value then enable mode. */
        CCP_WRITE_CPRH(h->Instance, h->CompareValue >> 8);
        CCP_WRITE_CPRL(h->Instance, h->CompareValue & 0xFFU);
        uint8_t con = (uint8_t)(h->Mode & PIC_CCP1_M_MASK);
        if (h->Instance == CCP_INSTANCE_1) {
            con |= (uint8_t)((h->PWMOutputMode & 0x3U) << 6);
        }
        CCP_WRITE_CON(h->Instance, con);
    }

    /* ECCP1-only: dead-band + auto-restart (ECCP1DEL) and auto-shutdown
     * (ECCP1AS). CCP2 has neither; skip. */
    if (h->Instance == CCP_INSTANCE_1) {
        uint8_t del = (uint8_t)(h->DeadBand.Delay & PIC_ECCP1DEL_PDC_MASK);
        if (h->DeadBand.AutoRestart) del |= PIC_ECCP1DEL_PRSEN;
        PIC8_REG8(PIC_REG_ECCP1DEL) = del;
        /* ECCP1AS: source[6:4] | PSSAC[3:2] | PSSBD[1:0]; ECCPASE (bit 7)
         * is the status bit, left 0 here. */
        uint8_t asv = (uint8_t)(((h->AutoShutdown.Source & 0x7U) << 4) |
                                (pss_encode(h->AutoShutdown.PinsAC) << 2) |
                                pss_encode(h->AutoShutdown.PinsBD));
        PIC8_REG8(PIC_REG_ECCP1AS) = asv;
    }

    return HAL_OK;
}

HAL_StatusTypeDef HAL_CCP_DeInit(CCP_InstanceTypeDef inst)
{
    if (inst != CCP_INSTANCE_1 && inst != CCP_INSTANCE_2) return HAL_INVALID;
    HAL_IRQ_DisableSrc(ccp_irq(inst));
    HAL_IRQ_ClearFlag(ccp_irq(inst));
    CCP_WRITE_CON(inst, 0x00U);
    if (inst == CCP_INSTANCE_1) {
        PIC8_REG8(PIC_REG_ECCP1DEL) = PIC_ECCP1DEL_POR_VALUE;
        PIC8_REG8(PIC_REG_ECCP1AS) = PIC_ECCP1AS_POR_VALUE;
    }
    g_ccp_handles[inst] = NULL;
    return HAL_OK;
}

void HAL_CCP_SetCompare(CCP_InstanceTypeDef inst, uint16_t value)
{
    if (inst != CCP_INSTANCE_1 && inst != CCP_INSTANCE_2) return;
    /* High byte first to avoid a spurious compare match (DS39632E §16.x). */
    CCP_WRITE_CPRH(inst, value >> 8);
    CCP_WRITE_CPRL(inst, value & 0xFFU);
}

uint16_t HAL_CCP_GetCapture(CCP_InstanceTypeDef inst)
{
    if (inst != CCP_INSTANCE_1 && inst != CCP_INSTANCE_2) return 0U;
    uint8_t lo, hi1, hi2;
    do {
        CCP_READ_CPRH(inst, hi1);
        CCP_READ_CPRL(inst, lo);
        CCP_READ_CPRH(inst, hi2);
    } while (hi1 != hi2);
    return (uint16_t)(((uint16_t)hi2 << 8) | lo);
}

void HAL_CCP_SetPWMDuty(CCP_InstanceTypeDef inst, uint16_t duty)
{
    if (inst != CCP_INSTANCE_1 && inst != CCP_INSTANCE_2) return;
    duty &= 0x03FFU;
    /* Latch the duty LSBs first (CCPxCON<5:4>), then CCPRxL (bits 9:2),
     * preserving the mode + P1M bits (DS39632E §16.4.4). */
    uint8_t con;
    CCP_READ_CON(inst, con);
    con = (uint8_t)(con & ~PIC_CCP1_DC1B_MASK);
    con |= (uint8_t)((duty & 0x03U) << 4);
    CCP_WRITE_CON(inst, con);
    CCP_WRITE_CPRL(inst, duty >> 2);
}

/* ───────────────────────── ECCP1-only controls ──────────────────── */

void HAL_CCP_ConfigDeadBand(CCP_InstanceTypeDef inst,
                            uint8_t delay, bool auto_restart)
{
    if (inst != CCP_INSTANCE_1) return;     /* ECCP1 only */
    uint8_t del = (uint8_t)(delay & PIC_ECCP1DEL_PDC_MASK);
    if (auto_restart) del |= PIC_ECCP1DEL_PRSEN;
    PIC8_REG8(PIC_REG_ECCP1DEL) = del;
}

void HAL_CCP_ConfigAutoShutdown(CCP_InstanceTypeDef inst,
                                CCP_AutoShutdownSourceTypeDef source,
                                CCP_PinStateTypeDef pins_ac,
                                CCP_PinStateTypeDef pins_bd)
{
    if (inst != CCP_INSTANCE_1) return;     /* ECCP1 only */
    /* Preserve ECCPASE (status); reprogram source + pin states. */
    uint8_t asv = (uint8_t)(pic8_sfr_read8(PIC_REG_ECCP1AS) & PIC_ECCP1AS_ECCPASE);
    asv |= (uint8_t)(((source & 0x7U) << 4) |
                     (pss_encode(pins_ac) << 2) |
                     pss_encode(pins_bd));
    pic8_sfr_write8(PIC_REG_ECCP1AS, asv);
}

uint8_t HAL_CCP_IsShutdown(CCP_InstanceTypeDef inst)
{
    if (inst != CCP_INSTANCE_1) return 0U;
    return (PIC8_REG8(PIC_REG_ECCP1AS) & PIC_ECCP1AS_ECCPASE) ? 1U : 0U;
}

void HAL_CCP_Restart(CCP_InstanceTypeDef inst)
{
    if (inst != CCP_INSTANCE_1) return;
    /* Clear ECCPASE, preserving the source/pin-state configuration. */
    uint8_t asv = (uint8_t)(pic8_sfr_read8(PIC_REG_ECCP1AS) & ~PIC_ECCP1AS_ECCPASE);
    pic8_sfr_write8(PIC_REG_ECCP1AS, asv);
}

/* ───────────────────────── ISRs ─────────────────────────────────── */

void CCP1_IRQHandler(void)
{
    if (!HAL_IRQ_GetFlag(PIC18_IRQ_CCP1)) return;
    HAL_IRQ_ClearFlag(PIC18_IRQ_CCP1);
    if (g_ccp_handles[CCP_INSTANCE_1] &&
        g_ccp_handles[CCP_INSTANCE_1]->EventCallback) {
        g_ccp_handles[CCP_INSTANCE_1]->EventCallback();
    }
}

void CCP2_IRQHandler(void)
{
    if (!HAL_IRQ_GetFlag(PIC18_IRQ_CCP2)) return;
    HAL_IRQ_ClearFlag(PIC18_IRQ_CCP2);
    if (g_ccp_handles[CCP_INSTANCE_2] &&
        g_ccp_handles[CCP_INSTANCE_2]->EventCallback) {
        g_ccp_handles[CCP_INSTANCE_2]->EventCallback();
    }
}
