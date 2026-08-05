/**
 * @file    example_usart.c
 * @brief   EUSART driver smoke test on the PIC18 host sim.
 *
 * @details
 *   Verifies BRG math (all of Table 20-1, DS39632E §20.0, including the
 *   16-bit BRG16=1 extension PIC16 lacks), init register programming,
 *   TX/RX, auto-baud, and 9-bit address-detect. The RX IRQ handler
 *   consumes RCREG, so polling checks disable the sim IRQ callback; host
 *   sim only, calls `pic18_sim_*` host-only hooks.
 */

#include "epic_hal.h"
#include "core/epic_harness.h"
#include "pic18fxx5x_sim.h"
#include <stdio.h>

#define CHECK(cond, msg) do { \
    if (!(cond)) { epic_harness_log("FAIL: %s\n", msg); return epic_harness_report(0); } \
} while (0)

/* A non-NULL RX callback forces CREN on in RCSTA; it is never invoked here
 * because the sim IRQ callback is disabled, so the body is irrelevant. */
static void rx_dummy_cb(uint8_t data) { (void)data; }

int main(void)
{
    /* 1. BRG math (DS39632E Table 20-1).
     *
     * Async @ 16 MHz, BRG16=0, BRGH=1, 9600 baud: divisor 16 -> X=103.
     *   (16e6 / (16*9600)) - 1 = 104.16 -> 104 - 1 = 103.
     * Async @ 16 MHz, BRG16=0, BRGH=0, 9600 baud: divisor 64 -> X=25.
     * Sync  @ 16 MHz, 1 MHz baud: divisor 4 -> X=3.
     * Async @ 16 MHz, BRG16=1, BRGH=1, 115200 baud: divisor 4 -> X=33. */
    uint16_t sp = USART_ComputeSPBRG(16000000UL, 9600UL,
                                     USART_MODE_ASYNCHRONOUS,
                                     USART_BRGH_HIGH, USART_BAUDGEN_8BIT);
    CHECK(sp == 103U, "SPBRG async 16MHz BRG16=0 BRGH=1 9600 != 103");

    sp = USART_ComputeSPBRG(16000000UL, 9600UL,
                            USART_MODE_ASYNCHRONOUS,
                            USART_BRGH_LOW, USART_BAUDGEN_8BIT);
    CHECK(sp == 25U, "SPBRG async 16MHz BRG16=0 BRGH=0 9600 != 25");

    sp = USART_ComputeSPBRG(16000000UL, 1000000UL,
                            USART_MODE_SYNCHRONOUS,
                            USART_BRGH_LOW, USART_BAUDGEN_8BIT);
    CHECK(sp == 3U, "SPBRG sync 16MHz 1MHz != 3");

    /* 16-bit BRG: divisor 4 at 115200 -> 33 (would be 33 with 8-bit too,
     * but this proves the BRG16=1 path). */
    sp = USART_ComputeSPBRG(16000000UL, 115200UL,
                            USART_MODE_ASYNCHRONOUS,
                            USART_BRGH_HIGH, USART_BAUDGEN_16BIT);
    CHECK(sp == 33U, "SPBRG async 16MHz BRG16=1 BRGH=1 115200 != 33");

    /* 16-bit BRG range extension. Same divisor (16) and same X=832 on both
     * sides — only the BRG width changes: 8-bit BRG with BRGH=1 (divisor 16)
     * at 1200 baud gives X = (16e6/(16*1200))-1 = 832, which exceeds 255 ->
     * 0xFFFF; 16-bit BRG with BRGH=0 (also divisor 16) accepts 832.
     *   X = (16e6 / (16*1200)) - 1 = 833.3 - 1 = 832. */
    sp = USART_ComputeSPBRG(16000000UL, 1200UL,
                            USART_MODE_ASYNCHRONOUS,
                            USART_BRGH_HIGH, USART_BAUDGEN_8BIT);
    CHECK(sp == 0xFFFFU, "SPBRG 8-bit should overflow at 832");
    sp = USART_ComputeSPBRG(16000000UL, 1200UL,
                            USART_MODE_ASYNCHRONOUS,
                            USART_BRGH_LOW, USART_BAUDGEN_16BIT);
    CHECK(sp == 832U, "SPBRG 16-bit should reach 832 at 1200 baud");

    /* 2. Init async 8-bit BRG, no callbacks (poll mode). */
    epic_harness_init(16U);
    /* Disable the sim IRQ callback so the RX handler does not consume the
     * byte before the polling checks read it. */
    pic18_sim_set_irq_callback(NULL);

    USART_HandleTypeDef h = USART_HANDLE_DEFAULT;
    h.Mode     = USART_MODE_ASYNCHRONOUS;
    h.BaudHigh = USART_BRGH_HIGH;
    h.BaudGen  = USART_BAUDGEN_8BIT;
    h.SPBRG    = 103U;
    h.RxCpltCallback = NULL;   /* No callback -> CREN not set. */
    EPIC_USART_Init(&h);

    /* TXSTA: reset 0x02 (TRMT) | BRGH(bit2) = 0x06. No TXEN (no callback). */
    CHECK(epic_sfr_read8(PIC_REG_TXSTA) == 0x06U,
          "TXSTA not 0x06 after async 8-bit init (BRGH expected)");
    /* RCSTA: SPEN(bit7) only, no CREN/RX9/ADDEN -> 0x80. */
    CHECK(epic_sfr_read8(PIC_REG_RCSTA) == 0x80U,
          "RCSTA not 0x80 after async 8-bit init (SPEN only)");
    /* BAUDCON: BRG16=0, no ABDEN -> 0x00. */
    CHECK(epic_sfr_read8(PIC_REG_BAUDCON) == 0x00U,
          "BAUDCON not 0x00 for 8-bit BRG");
    CHECK(epic_sfr_read8(PIC_REG_SPBRG) == 103U, "SPBRG not 103 after init");
    CHECK(epic_sfr_read8(PIC_REG_SPBRGH) == 0x00U, "SPBRGH not 0 after 8-bit init");

    /* 3. Transmit a byte. Verify TXREG holds it and TXIF cleared. */
    EPIC_USART_Transmit(0xA5U);
    CHECK(epic_sfr_read8(PIC_REG_TXREG) == 0xA5U, "TXREG did not capture 0xA5");
    CHECK((epic_sfr_read8(PIC_REG_PIR1) & PIC_PIR1_TXIF) == 0U,
          "TXIF should be 0 after Transmit");

    /* 4. RX path: drive a byte, then Receive. */
    pic18_sim_drive_usart_rx(0xC3U);
    CHECK((epic_sfr_read8(PIC_REG_PIR1) & PIC_PIR1_RCIF) != 0U,
          "RCIF not set after drive_usart_rx");
    uint8_t got = EPIC_USART_Receive();
    CHECK(got == 0xC3U, "Receive did not return 0xC3");
    CHECK((epic_sfr_read8(PIC_REG_PIR1) & PIC_PIR1_RCIF) == 0U,
          "RCIF not cleared after Receive");

    /* 5. 16-bit BRG mode (BRG16=1, BRGH=1, SPBRG=33). BAUDCON<BRG16>=bit3=0x08. */
    h.BaudGen = USART_BAUDGEN_16BIT;
    h.SPBRG   = 33U;
    h.SPBRGH  = 0U;
    EPIC_USART_Init(&h);
    CHECK(epic_sfr_read8(PIC_REG_BAUDCON) == PIC_BAUDCON_BRG16,
          "BAUDCON<BRG16> not set for 16-bit BRG");
    CHECK(epic_sfr_read8(PIC_REG_SPBRG) == 33U, "SPBRG not 33 after 16-bit init");
    CHECK(epic_sfr_read8(PIC_REG_SPBRGH) == 0x00U, "SPBRGH not 0 after 16-bit init");

    /* 6. Auto-baud: StartAutoBaud sets ABDEN; it stays busy; overflow clearable. */
    EPIC_USART_StartAutoBaud();
    CHECK((epic_sfr_read8(PIC_REG_BAUDCON) & PIC_BAUDCON_ABDEN) != 0U,
          "ABDEN not set after StartAutoBaud");
    CHECK(EPIC_USART_IsAutoBaudBusy() == 1U, "IsAutoBaudBusy not 1");
    /* ABDOVF is read/clear; clear it and confirm. */
    EPIC_USART_ClearAutoBaudOverflow();
    CHECK(EPIC_USART_HasAutoBaudOverflow() == 0U, "ABDOVF not cleared");

    /* 7. 9-bit address-detect mode: a callback forces CREN; ADDEN + RX9 set.
     *    RCSTA = SPEN|RX9|CREN|ADDEN = 0x80|0x40|0x10|0x08 = 0xD8. */
    USART_HandleTypeDef ha = USART_HANDLE_DEFAULT;
    ha.Mode          = USART_MODE_ASYNCHRONOUS;
    ha.DataWidth     = USART_DATA_9BITS;
    ha.AddressDetect = 1U;
    ha.RxCpltCallback = rx_dummy_cb;   /* non-NULL -> CREN set; not invoked. */
    EPIC_USART_Init(&ha);
    CHECK(epic_sfr_read8(PIC_REG_RCSTA) == 0xD8U,
          "RCSTA not 0xD8 for 9-bit address-detect (SPEN|RX9|CREN|ADDEN)");

    epic_harness_log("OK: EUSART driver, BRG math (8/16-bit), init, TX, RX, "
                     "auto-baud, address-detect all pass.\n");
    return epic_harness_report(1);
}
