/* Family-neutral Timer2 contract (EPIC_TIMER2_*), backed by TIMER246
 * instance 2. The include path selects which family's copy resolves.
 *
 * A header-only alias onto EPIC_TIMER246_Init would dangle: that Init
 * stores the handle pointer for the ISR while epic_tick_init builds
 * its handle as a stack local. So this shim's Init copies the caller
 * into a driver-owned handle the ISR reads, the pic14 contract's
 * g_t2_overflow_cb shape. */

#ifndef EPIC_TIMER2_H
#define EPIC_TIMER2_H

#include "peripherals/pic16f193x_timer246.h"

/* Same DS41364B section 17.0 encodings as TIMER246, so these alias it. */
typedef TIMER246_PrescalerTypeDef  TIMER2_PrescalerTypeDef;
typedef TIMER246_PostscalerTypeDef TIMER2_PostscalerTypeDef;

#define TIMER2_PRESCALER_1_1  TIMER246_PRESCALER_1_1
#define TIMER2_PRESCALER_1_4  TIMER246_PRESCALER_1_4
#define TIMER2_PRESCALER_1_16 TIMER246_PRESCALER_1_16

#define TIMER2_POSTSCALER_1_1  TIMER246_POSTSCALER_1_1
#define TIMER2_POSTSCALER_1_2  TIMER246_POSTSCALER_1_2
#define TIMER2_POSTSCALER_1_3  TIMER246_POSTSCALER_1_3
#define TIMER2_POSTSCALER_1_4  TIMER246_POSTSCALER_1_4
#define TIMER2_POSTSCALER_1_5  TIMER246_POSTSCALER_1_5
#define TIMER2_POSTSCALER_1_6  TIMER246_POSTSCALER_1_6
#define TIMER2_POSTSCALER_1_7  TIMER246_POSTSCALER_1_7
#define TIMER2_POSTSCALER_1_8  TIMER246_POSTSCALER_1_8
#define TIMER2_POSTSCALER_1_9  TIMER246_POSTSCALER_1_9
#define TIMER2_POSTSCALER_1_10 TIMER246_POSTSCALER_1_10
#define TIMER2_POSTSCALER_1_11 TIMER246_POSTSCALER_1_11
#define TIMER2_POSTSCALER_1_12 TIMER246_POSTSCALER_1_12
#define TIMER2_POSTSCALER_1_13 TIMER246_POSTSCALER_1_13
#define TIMER2_POSTSCALER_1_14 TIMER246_POSTSCALER_1_14
#define TIMER2_POSTSCALER_1_15 TIMER246_POSTSCALER_1_15
#define TIMER2_POSTSCALER_1_16 TIMER246_POSTSCALER_1_16

/** Driver handle (Cube-style), same field shape as the pic14 contract. */
typedef struct {
    TIMER2_PrescalerTypeDef  Prescaler;
    TIMER2_PostscalerTypeDef Postscaler;
    uint8_t                  Period;   /**< PR2 value, 0..255. */
    /** @brief Optional overflow callback (fires on TMR2IF, i.e. every
     *         prescaler x (PR2+1) x postscaler cycles). */
    void (*OverflowCallback)(void);
} TIMER2_HandleTypeDef;

#define TIMER2_HANDLE_DEFAULT {                                         \
    .Prescaler        = TIMER2_PRESCALER_1_1,                           \
    .Postscaler       = TIMER2_POSTSCALER_1_1,                          \
    .Period           = 0xFFU,                                          \
    .OverflowCallback = NULL,                                           \
}

/* Driver-owned instance-2 handle, defined in pic16f193x_timer246.c.
 * EPIC_TIMER2_Init copies the caller here and registers this address,
 * never the caller's, so a stack-built handle cannot dangle. */
extern TIMER246_HandleTypeDef g_timer2_owned_handle;

/**
 * @brief Copy a Timer2 handle into TIMER246 instance-2 form.
 * @param h source Timer2 handle.
 * @param t246 destination TIMER246 handle, pinned to instance 2.
 */
static inline void timer2_copy_to_246(const TIMER2_HandleTypeDef *h,
                                      TIMER246_HandleTypeDef *t246)
{
    t246->Instance         = TIMER246_INSTANCE_2;
    t246->Prescaler        = (TIMER246_PrescalerTypeDef)h->Prescaler;
    t246->Postscaler       = (TIMER246_PostscalerTypeDef)h->Postscaler;
    t246->Period           = h->Period;
    t246->OverflowCallback = h->OverflowCallback;
}

/**
 * @brief Initialize Timer2 from the handle. Static inline so the
 *        callback store lands in the caller's TU as a named literal,
 *        which the epic-cc cross-context analysis resolves.
 * @param h handle with Prescaler, Postscaler, Period, OverflowCallback.
 * @return EPIC_OK on success, EPIC_INVALID if `h` is NULL.
 */
static inline EPIC_StatusTypeDef EPIC_TIMER2_Init(const TIMER2_HandleTypeDef *h)
{
    if (!h) return EPIC_INVALID;
    timer2_copy_to_246(h, &g_timer2_owned_handle);
    return EPIC_TIMER246_Init(&g_timer2_owned_handle);
}

/**
 * @brief De-initialize Timer2: disable the interrupt and restore T2CON
 *        and PR2 to reset values.
 * @return EPIC_OK on success.
 */
static inline EPIC_StatusTypeDef EPIC_TIMER2_DeInit(void)
{
    return EPIC_TIMER246_DeInit(TIMER246_INSTANCE_2);
}

/**
 * @brief Start Timer2 counting. Static inline for the same reason as
 *        Init. Programs PR2 and T2CON from the caller's handle; the
 *        local never escapes, EPIC_TIMER246_Start stores no pointer.
 * @param h handle whose Period is loaded into PR2.
 * @return EPIC_OK on success, EPIC_INVALID if `h` is NULL.
 */
static inline EPIC_StatusTypeDef EPIC_TIMER2_Start(const TIMER2_HandleTypeDef *h)
{
    if (!h) return EPIC_INVALID;
    TIMER246_HandleTypeDef t246;
    timer2_copy_to_246(h, &t246);
    return EPIC_TIMER246_Start(&t246);
}

/**
 * @brief Stop Timer2 counting. Clears TMR2ON.
 * @return EPIC_OK on success.
 */
static inline EPIC_StatusTypeDef EPIC_TIMER2_Stop(void)
{
    return EPIC_TIMER246_Stop(TIMER246_INSTANCE_2);
}

/**
 * @brief Read the current counter value.
 * @return the current 8-bit TMR2 value.
 */
static inline uint8_t EPIC_TIMER2_ReadCounter(void)
{
    return EPIC_TIMER246_ReadCounter(TIMER246_INSTANCE_2);
}

/**
 * @brief Write the counter value.
 * @param value the 8-bit value to load into TMR2.
 */
static inline void EPIC_TIMER2_WriteCounter(uint8_t value)
{
    EPIC_TIMER246_WriteCounter(TIMER246_INSTANCE_2, value);
}

/**
 * @brief Read the period register value.
 * @return the current PR2 value.
 */
static inline uint8_t EPIC_TIMER2_ReadPeriod(void)
{
    return EPIC_TIMER246_ReadPeriod(TIMER246_INSTANCE_2);
}

/**
 * @brief Write the period register value.
 * @param period the 8-bit PR2 value, 0..255.
 */
static inline void EPIC_TIMER2_WritePeriod(uint8_t period)
{
    EPIC_TIMER246_WritePeriod(TIMER246_INSTANCE_2, period);
}

/**
 * @brief Convert a prescaler enum to its integer ratio (1, 4, 16).
 * @param p the prescaler enum value.
 * @return the integer prescaler ratio (1, 4 or 16).
 */
static inline uint16_t EPIC_TIMER2_PrescalerToRatio(TIMER2_PrescalerTypeDef p)
{
    return EPIC_TIMER246_PrescalerToRatio((TIMER246_PrescalerTypeDef)p);
}

/**
 * @brief Convert a postscaler enum to its integer ratio (1..16).
 * @param p the postscaler enum value.
 * @return the integer postscaler ratio (1..16).
 */
static inline uint16_t EPIC_TIMER2_PostscalerToRatio(TIMER2_PostscalerTypeDef p)
{
    return EPIC_TIMER246_PostscalerToRatio((TIMER246_PostscalerTypeDef)p);
}

#endif /* EPIC_TIMER2_H */
