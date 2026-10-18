/**
 * PIC16F188xx comparator pair driver (DS40001802D §18). Per-instance
 * CMxNSEL/CMxPSEL, CMxCON1 and CMxCON0 are written with
 * branch-before-touch access, so no SFR address is ever a runtime
 * value (adding-a-device.md §4 high-risk pattern).
 */

#include <stddef.h>
#include "peripherals/pic16f188xx_comp.h"
#include "core/pic16f188xx_irq.h"

/* ISR callback slots, written from main context only. Scalars, not
 * array elements: each handler references a fixed symbol. */
static void (*s_c1_cb)(void) = NULL;
static void (*s_c2_cb)(void) = NULL;

/**
 * @brief Test whether a positive channel code names a connected source.
 * @param ch channel code to check.
 * @return 1 for a pin, DAC, FVR or AVSS code, 0 for unconnected codes.
 */
static uint8_t pos_ok(uint8_t ch)
{
    return (uint8_t)((ch <= (uint8_t)COMP_POS_PIN1) || ((ch >= (uint8_t)COMP_POS_DAC) &&
        (ch <= (uint8_t)COMP_POS_AVSS)));
}

/**
 * @brief Test whether a negative channel code names a connected source.
 * @param ch channel code to check.
 * @return 1 for a pin, FVR or AVSS code, 0 for unconnected codes.
 */
static uint8_t neg_ok(uint8_t ch)
{
    return (uint8_t)((ch <= (uint8_t)COMP_NEG_PIN3) || ((ch >= (uint8_t)COMP_NEG_FVR) &&
        (ch <= (uint8_t)COMP_NEG_AVSS)));
}

/**
 * @brief Build the CMxCON1 edge bits shared by both instances.
 * @param edge edge selection from the handle.
 * @return CMxCON1 value with INTN/INTP set.
 */
static uint8_t edge_bits(COMP_InterruptEdgeTypeDef edge)
{
    uint8_t con1 = 0x00U;

    if (((uint8_t)edge & (uint8_t)COMP_INT_EDGE_RISING) != 0U)
    {
        con1 |= PIC_CM1CON1_INTP;
    }
    if (((uint8_t)edge & (uint8_t)COMP_INT_EDGE_FALLING) != 0U)
    {
        con1 |= PIC_CM1CON1_INTN;
    }
    return con1;
}

/**
 * @brief Configure a comparator from the handle and enable it.
 * @param h handle with instance, channel and edge selection
 * @return EPIC_OK on success, EPIC_INVALID for a NULL handle, an invalid
 *         instance or a channel outside the header enums
 */
EPIC_StatusTypeDef EPIC_COMP_Init(const COMP_HandleTypeDef *h)
{
    uint8_t con0;
    uint8_t con1;

    if ((h == NULL) || (pos_ok((uint8_t)h->PosChannel) == 0U) ||
        (neg_ok((uint8_t)h->NegChannel) == 0U))
    {
        return EPIC_INVALID;
    }
    con1 = edge_bits(h->InterruptEdge);
    con0 = PIC_CM1CON0_ON;
    if (h->HysteresisOn != 0U)
    {
        con0 |= PIC_CM1CON0_HYS;
    }
    if (h->InvertOutput != 0U)
    {
        con0 |= PIC_CM1CON0_POL;
    }

    if (h->Instance == COMP_INSTANCE_1)
    {
        EPIC_REG8(PIC_REG_CM1NSEL) = (uint8_t)((uint8_t)h->NegChannel & PIC_CM1NSEL_NCH_MASK);
        EPIC_REG8(PIC_REG_CM1PSEL) = (uint8_t)((uint8_t)h->PosChannel & PIC_CM1PSEL_PCH_MASK);
        EPIC_REG8(PIC_REG_CM1CON1) = con1;
        EPIC_REG8(PIC_REG_CM1CON0) = con0;
        s_c1_cb = h->EventCallback;
    }
    else if (h->Instance == COMP_INSTANCE_2)
    {
        EPIC_REG8(PIC_REG_CM2NSEL) = (uint8_t)((uint8_t)h->NegChannel & PIC_CM2NSEL_NCH_MASK);
        EPIC_REG8(PIC_REG_CM2PSEL) = (uint8_t)((uint8_t)h->PosChannel & PIC_CM2PSEL_PCH_MASK);
        EPIC_REG8(PIC_REG_CM2CON1) = con1;
        EPIC_REG8(PIC_REG_CM2CON0) = con0;
        s_c2_cb = h->EventCallback;
    }
    else
    {
        return EPIC_INVALID;
    }
    return EPIC_OK;
}

/**
 * @brief Disable a comparator by clearing its registers.
 * @param inst comparator instance (1-2)
 * @return EPIC_OK on success, EPIC_INVALID for an unknown instance
 */
EPIC_StatusTypeDef EPIC_COMP_DeInit(COMP_InstanceTypeDef inst)
{
    if (inst == COMP_INSTANCE_1)
    {
        EPIC_REG8(PIC_REG_CM1CON0) = 0x00U;
        EPIC_REG8(PIC_REG_CM1CON1) = 0x00U;
        EPIC_REG8(PIC_REG_CM1NSEL) = 0x00U;
        EPIC_REG8(PIC_REG_CM1PSEL) = 0x00U;
        s_c1_cb = NULL;
    }
    else if (inst == COMP_INSTANCE_2)
    {
        EPIC_REG8(PIC_REG_CM2CON0) = 0x00U;
        EPIC_REG8(PIC_REG_CM2CON1) = 0x00U;
        EPIC_REG8(PIC_REG_CM2NSEL) = 0x00U;
        EPIC_REG8(PIC_REG_CM2PSEL) = 0x00U;
        s_c2_cb = NULL;
    }
    else
    {
        return EPIC_INVALID;
    }
    return EPIC_OK;
}

/**
 * @brief Read the comparator's current output level from CMOUT.
 * @param inst comparator instance (1-2)
 * @return 1 if the output is high, 0 if low (0 for an unknown instance)
 */
uint8_t EPIC_COMP_ReadOutput(COMP_InstanceTypeDef inst)
{
    uint8_t cmout = EPIC_REG8(PIC_REG_CMOUT);

    if (inst == COMP_INSTANCE_1)
    {
        return (cmout & PIC_CMOUT_MC1OUT) ? 1U : 0U;
    }
    if (inst == COMP_INSTANCE_2)
    {
        return (cmout & PIC_CMOUT_MC2OUT) ? 1U : 0U;
    }
    return 0U;
}

/**
 * @brief Comparator 1 interrupt handler: clears C1IF, then runs the
 *        instance callback if one is set (weak, override in user code).
 * @details Keeps no cstack autos (ISR bank rule, family MANUAL.md).
 */
void CMP1_IRQHandler(void)
{
    EPIC_IRQ_ClearFlag(PIC16F188XX_IRQ_CMP1);
    if (s_c1_cb != NULL)
    {
        s_c1_cb();
    }
}

/**
 * @brief Comparator 2 interrupt handler: clears C2IF, then runs the
 *        instance callback if one is set (weak, override in user code).
 * @details Keeps no cstack autos (ISR bank rule, family MANUAL.md).
 */
void CMP2_IRQHandler(void)
{
    EPIC_IRQ_ClearFlag(PIC16F188XX_IRQ_CMP2);
    if (s_c2_cb != NULL)
    {
        s_c2_cb();
    }
}
