/* USART driver smoke test on the sim backend: SPBRG math vs DS39582B
 * Table 10-1, Init programs TXSTA/SPBRG, Transmit writes TXREG, and a
 * simulated RX byte is returned with RCIF cleared. */

#include "pic16f87xa.h"
#include "pic16f87xa_sim.h"
#include "pic16f87xa_sfr.h"
#include "peripherals/pic16f87xa_usart.h"
#include "core/pic16_irq.h"
#include <stdio.h>

/* Helper: report a failed assert and exit. */
#define CHECK(cond, msg) do { \
    if (!(cond)) { printf("FAIL: %s\n", msg); return 1; } \
} while (0)

int main(void)
{
    /* 1. SPBRG formula check.
     * Async @ 16 MHz, BRGH=1, 9600 baud:
     *   X = (16_000_000 / (16 × 9_600)) - 1 = 103.17 → 103
     * DS39582B Table 10-4 (Fosc=16 MHz, 9.6K, BRGH=1) lists 103. */
    uint16_t sp = USART_ComputeSPBRG(16000000UL, 9600UL,
                                     USART_MODE_ASYNCHRONOUS,
                                     USART_BRGH_HIGH);
    CHECK(sp == 103U, "SPBRG async 16MHz BRGH=1 9600 baud != 103");

    /* Async @ 16 MHz, BRGH=0, 9600 baud:
     *   X = (16_000_000 / (64 × 9_600)) - 1 = 25.04 → 25
     * DS39582B Table 10-3 (Fosc=16 MHz, 9.6K, BRGH=0) lists 25. */
    sp = USART_ComputeSPBRG(16000000UL, 9600UL,
                            USART_MODE_ASYNCHRONOUS,
                            USART_BRGH_LOW);
    CHECK(sp == 25U, "SPBRG async 16MHz BRGH=0 9600 baud != 25");

    /* Sync @ 16 MHz, 1 MHz baud:
     *   X = (16_000_000 / (4 × 1_000_000)) - 1 = 3. */
    sp = USART_ComputeSPBRG(16000000UL, 1000000UL,
                            USART_MODE_SYNCHRONOUS,
                            USART_BRGH_LOW);
    CHECK(sp == 3U, "SPBRG sync 16MHz 1MHz baud != 3");

    /* 2. Init and verify register state. */
    pic16f87xa_sim_reset();
    /* No IRQ callback, the test polls flags and registers directly. */
    pic16f87xa_sim_set_irq_callback(NULL);

    USART_HandleTypeDef h = USART_HANDLE_DEFAULT;
    h.Mode       = USART_MODE_ASYNCHRONOUS;
    h.BaudHigh   = USART_BRGH_HIGH;
    h.SPBRG      = 103U;
    h.RxCpltCallback = NULL;   /* No callback → CREN not set. */
    EPIC_USART_Init(&h);

    /* TXSTA is at 0x98 (Bank 1); SPBRG is at 0x99 (Bank 1). */
    uint8_t prev = (EPIC_REG8(PIC_REG_STATUS) >> 5) & 0x03U;
    pic_select_bank(1);
    uint8_t txsta_b1 = EPIC_REG8(0x98U);
    uint8_t spbrg    = EPIC_REG8(0x99U);
    pic_select_bank(prev);

    CHECK(spbrg == 103U, "SPBRG not 103 after Init");
    /* TXSTA reset value: 0x02 (TRMT). With BaudHigh=HIGH (1) the
     * driver sets BRGH (bit 2), so the result is 0x06. */
    CHECK(txsta_b1 == 0x06U, "TXSTA not 0x06 after Init (BRGH expected)");

    /* 3. Transmit a byte. Verify TXREG holds it. */
    EPIC_USART_Transmit(0xA5U);
    CHECK(EPIC_REG8(PIC_REG_TXREG) == 0xA5U, "TXREG did not capture 0xA5");
    /* TXIF should be 0 right after the write. */
    CHECK((EPIC_REG8(0x0CU) & 0x10U) == 0U, "TXIF should be 0 after Transmit");

    /* 4. RX path: drive a byte, then Receive. */
    pic16f87xa_sim_drive_usart_rx(0xC3U);
    CHECK((EPIC_REG8(0x0CU) & 0x20U) != 0U, "RCIF not set after drive_usart_rx");
    uint8_t got = EPIC_USART_Receive();
    CHECK(got == 0xC3U, "Receive did not return 0xC3");
    CHECK((EPIC_REG8(0x0CU) & 0x20U) == 0U, "RCIF not cleared after Receive");

    printf("OK: USART driver, SPBRG math, init, transmit, receive all pass.\n");
    return 0;
}
