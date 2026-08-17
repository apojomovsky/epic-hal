/* EUSART driver smoke test on the sim backend: SPBRG math vs
 * DS40001291H Table 12-5, Init programs TXSTA/SPBRG, Transmit writes
 * TXREG, and a simulated RX byte is returned with RCIF cleared. The
 * 88X EUSART adds the BRG16 bit (16-bit SPBRGH:SPBRG pair), auto-baud
 * and wake-up (BAUDCTL), exercised below. */

#include "pic16f88x.h"
#include "pic16f88x_sim.h"
#include "pic16f88x_sfr.h"
#include "peripherals/pic16f88x_usart.h"
#include "core/pic16_irq.h"
#include <stdio.h>

/* Helper: report a failed assert and exit. */
#define CHECK(cond, msg) do { \
    if (!(cond)) { printf("FAIL: %s\n", msg); return 1; } \
} while (0)

/**
 * @brief Smoke-test the EUSART driver: SPBRG math (8/16-bit), init
 *        register state, transmit and receive.
 */
int main(void)
{
    /* 1. SPBRG formula check, 8-bit BRG.
     * Async @ 16 MHz, BRGH=1, 9600 baud:
     *   X = (16_000_000 / (16 × 9_600)) - 1 = 103.17 → 103
     * DS40001291H Table 12-5 (Fosc=16 MHz, 9.6K, BRGH=1) lists 103. */
    uint16_t sp = USART_ComputeSPBRG(16000000UL, 9600UL,
                                     USART_MODE_ASYNCHRONOUS,
                                     USART_BRGH_HIGH);
    CHECK(sp == 103U, "SPBRG async 16MHz BRGH=1 9600 baud != 103");

    /* Async @ 16 MHz, BRGH=0, 9600 baud:
     *   X = (16_000_000 / (64 × 9_600)) - 1 = 25.04 → 25
     * DS40001291H Table 12-5 (Fosc=16 MHz, 9.6K, BRGH=0) lists 25. */
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

    /* 16-bit BRG @ 20 MHz, 9600 baud:
     *   X = (20_000_000 / (16 × 9_600)) - 1 = 129.2 → 129 */
    sp = USART_ComputeSPBRG16(20000000UL, 9600UL,
                              USART_MODE_ASYNCHRONOUS);
    CHECK(sp == 129U, "SPBRG 16-bit 20MHz 9600 baud != 129");

    /* 2. Init and verify register state. */
    pic16f88x_sim_reset();
    /* No IRQ callback, the test polls flags and registers directly. */
    pic16f88x_sim_set_irq_callback(NULL);

    USART_HandleTypeDef h = USART_HANDLE_DEFAULT;
    h.Mode       = USART_MODE_ASYNCHRONOUS;
    h.BaudHigh   = USART_BRGH_HIGH;
    h.SPBRG      = 103U;
    h.RxCpltCallback = NULL;   /* No callback → CREN not set. */
    EPIC_USART_Init(&h);

    /* TXSTA is at 0x98 (Bank 1); SPBRG is at 0x99 (Bank 1). */
    uint8_t prev = (EPIC_REG8(PIC_REG_STATUS) >> 5) & 0x03U;
    pic_select_bank(1);
    uint8_t txsta_b1 = EPIC_REG8(PIC_REG_TXSTA);
    uint8_t spbrg    = EPIC_REG8(PIC_REG_SPBRG);
    pic_select_bank(prev);

    CHECK(spbrg == 103U, "SPBRG not 103 after Init");
    /* TXSTA reset value: 0x02 (TRMT). With BaudHigh=HIGH (1) the
     * driver sets BRGH (bit 2), so the result is 0x06. */
    CHECK(txsta_b1 == 0x06U, "TXSTA not 0x06 after Init (BRGH expected)");

    /* 3. Transmit a byte. Verify TXREG holds it. */
    EPIC_USART_Transmit(0xA5U);
    CHECK(EPIC_REG8(PIC_REG_TXREG) == 0xA5U, "TXREG did not capture 0xA5");
    /* TXIF should be 0 right after the write. */
    CHECK((EPIC_REG8(PIC_REG_PIR1) & 0x10U) == 0U, "TXIF should be 0 after Transmit");

    /* 4. RX path: drive a byte, then Receive. */
    pic16f88x_sim_drive_usart_rx(0xC3U);
    CHECK((EPIC_REG8(PIC_REG_PIR1) & 0x20U) != 0U, "RCIF not set after drive_usart_rx");
    uint8_t got = EPIC_USART_Receive();
    CHECK(got == 0xC3U, "Receive did not return 0xC3");
    CHECK((EPIC_REG8(PIC_REG_PIR1) & 0x20U) == 0U, "RCIF not cleared after Receive");

    /* 5. Enhanced features: wake-up + auto-baud bits (BAUDCTL, Bank 3). */
    EPIC_USART_SetWakeUp(1U);
    {
        uint8_t prev3 = (EPIC_REG8(PIC_REG_STATUS) >> 5) & 0x03U;
        pic_select_bank(3);
        uint8_t baudctl = EPIC_REG8(PIC_REG_BAUDCTL);
        pic_select_bank(prev3);
        CHECK((baudctl & PIC_BAUDCTL_WUE) != 0U, "WUE not set after SetWakeUp(1)");
    }
    EPIC_USART_SetWakeUp(0U);
    EPIC_USART_SetAutoBaud(1U);
    {
        uint8_t prev3 = (EPIC_REG8(PIC_REG_STATUS) >> 5) & 0x03U;
        pic_select_bank(3);
        uint8_t baudctl = EPIC_REG8(PIC_REG_BAUDCTL);
        pic_select_bank(prev3);
        CHECK((baudctl & PIC_BAUDCTL_ABDEN) != 0U, "ABDEN not set after SetAutoBaud(1)");
    }

    printf("OK: EUSART driver, SPBRG math (8/16-bit), init, transmit, receive, wake/auto-baud all pass.\n");
    return 0;
}
