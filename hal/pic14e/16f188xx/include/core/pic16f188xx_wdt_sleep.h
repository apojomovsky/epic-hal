/**
 * WDT, Sleep, Idle and BOR/POR helpers (DS40001802D §5.13, §8.0, §9.0).
 * CONFIG2 WDTE picks the WDT mode (11 on, 10 off in Sleep, 01 SWDTEN, 00
 * off); Enable/Disable act only in 01. The period is settable only with
 * CONFIG3 WDTCPS=11111 (the XC8 default). Refresh and Sleep are
 * link-time selected: `clrwdt`/`sleep` on target, counters on host.
 */
#ifndef PIC16F188XX_WDT_SLEEP_H
#define PIC16F188XX_WDT_SLEEP_H

#include <stdint.h>
#include "pic16f188xx.h"

/**
 * @brief WDT period (WDTCON0<WDTPS4:WDTPS0>, DS40001802D Register 9-1).
 *        Value N is 2^(N+5) cycles of the 31 kHz clock, so 1 ms nominal at
 *        0 and 256 s at 18. Times are approximate.
 */
typedef enum {
    WDT_PERIOD_1MS    = 0x00U,
    WDT_PERIOD_2MS    = 0x01U,
    WDT_PERIOD_4MS    = 0x02U,
    WDT_PERIOD_8MS    = 0x03U,
    WDT_PERIOD_16MS   = 0x04U,
    WDT_PERIOD_32MS   = 0x05U,
    WDT_PERIOD_64MS   = 0x06U,
    WDT_PERIOD_128MS  = 0x07U,
    WDT_PERIOD_256MS  = 0x08U,
    WDT_PERIOD_512MS  = 0x09U,
    WDT_PERIOD_1S     = 0x0AU,
    WDT_PERIOD_2S     = 0x0BU,   /**< Reset value when software-controlled. */
    WDT_PERIOD_4S     = 0x0CU,
    WDT_PERIOD_8S     = 0x0DU,
    WDT_PERIOD_16S    = 0x0EU,
    WDT_PERIOD_32S    = 0x0FU,
    WDT_PERIOD_64S    = 0x10U,
    WDT_PERIOD_128S   = 0x11U,
    WDT_PERIOD_256S   = 0x12U,
} WDT_PeriodTypeDef;

/**
 * @brief Program the WDT period and turn it on (WDTCON0<SWDTEN>).
 * @details Any WDTCON0 write clears the WDT, so the new period starts from
 * zero. SWDTEN has an effect only when CONFIG2 WDTE=01; in the other modes
 * it is ignored. When CONFIG3 WDTCPS fixes the period, the WDTPS bits are
 * read-only and the period stays as configured.
 * @param period Period to program.
 * @return EPIC_OK when WDTPS reads back as programmed, EPIC_INVALID for an
 *         out-of-range period (nothing is written), EPIC_ERROR when the
 *         period is fixed by CONFIG3 and differs from the request (SWDTEN
 *         is still set).
 */
EPIC_StatusTypeDef EPIC_WDT_Enable(WDT_PeriodTypeDef period);

/**
 * @brief Turn the software-controlled WDT off (clear WDTCON0<SWDTEN>).
 * @details The period bits are kept. No effect unless CONFIG2 WDTE=01.
 * @return EPIC_OK on success.
 */
EPIC_StatusTypeDef EPIC_WDT_Disable(void);

/**
 * @brief Refresh the WDT. Target: read WDTCON0 (arms the window in windowed
 *        mode, DS40001802D §9.5.1) then `clrwdt`. Host: counted in the simulator.
 */
void EPIC_WDT_Refresh(void);

/**
 * @brief Enter Sleep. Target: the `sleep` instruction followed by a `nop`
 *        (§8.2.1: the instruction after SLEEP runs before any ISR). Host:
 *        counted in the simulator, execution does not stop. With
 *        CPUDOZE<IDLEN> set the hardware enters Idle instead.
 */
void EPIC_Sleep_Enter(void);

/**
 * @brief Enter Idle: set CPUDOZE<IDLEN>, run EPIC_Sleep_Enter, then put
 *        IDLEN back as it was.
 * @details In Idle the CPU and flash stop but peripherals on FOSC keep
 * running (§8.2.4), so Timer0 on FOSC/4 or the EUSART can wake the part.
 * Any enabled interrupt ends Idle, GIE or not.
 */
void EPIC_Idle_Enter(void);

/**
 * @brief Report a Brown-out Reset since the flag was last cleared (PCON0<BOR>
 *        is active low: hardware clears it on the event).
 * @return 1 when the last reset was a Brown-out Reset, 0 otherwise.
 */
uint8_t EPIC_BOR_GetStatus(void);

/**
 * @brief Re-arm the BOR flag by setting PCON0<BOR> (required in software
 *        after a Power-on or Brown-out Reset, §5.13).
 */
void EPIC_BOR_ClearFlag(void);

/**
 * @brief Report a Power-on Reset since the flag was last cleared (PCON0<POR>
 *        is active low).
 * @return 1 when a Power-on Reset occurred, 0 otherwise.
 */
uint8_t EPIC_POR_GetStatus(void);

/**
 * @brief Re-arm the POR flag by setting PCON0<POR> (required in software
 *        after a Power-on Reset, §5.13).
 */
void EPIC_POR_ClearFlag(void);

#endif /* PIC16F188XX_WDT_SLEEP_H */
