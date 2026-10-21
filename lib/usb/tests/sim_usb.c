/*
 * Bounded, self-reporting HARNESS=sim bring-up gate for lib/usb on
 * PIC18F4550. MPLAB SIM models no USB SIE: there is no bus, no host and
 * no token engine (the simulator's PIC18 peripheral set has no USB
 * model), so this gate cannot enumerate and must not pretend to. It
 * proves the half of bring-up that enumeration depends on:
 * epic_usb_init() enables the SIE, selects full speed and the internal
 * pull-up, enables control transfers on EP0 and data transfers on
 * EP1/EP2, leaves UADDR at 0, and hands the SIE a buffer descriptor
 * table whose buffers all live in USB RAM. Enumeration, the
 * descriptors on the wire, CDC class requests and the echo round trip
 * stay unproven until a board runs the same firmware.
 *
 * Register and memory facts: DS39632E Section 17.2 (UCON, UCFG, UEPn,
 * UADDR), Section 17.3 (bank 4, 400h-4FFh, holds the buffer descriptor
 * table; banks 5-7, 500h-7FFh, hold endpoint data), Section 17.4.3 (a
 * buffer descriptor address outside 400h-7FFh loses or overwrites
 * data). Register names come from the pinned DFP's pic18f4550.h.
 */

#include <xc.h>

#include "epic_usb.h"
#include "core/epic_harness.h"

/* Buffer descriptor table base and the USB RAM data-bank window. */
#define BDT_ADDR   0x400u
#define BDT_COUNT  6u      /* EP0 out/in + EP1 out/in + EP2 out/in */
#define USB_RAM_LO 0x500u  /* DS39632E Section 17.3: bank 4 is the BDT */
#define USB_RAM_HI 0x800u

/* BDnSTAT bits this gate reads (DS39632E Register 17-6, SIE mode). */
#define BD_UOWN    0x80u
#define BD_BC_MASK 0x03u   /* BC9:BC8, the count's high bits */

static uint8_t g_failures;

/**
 * @brief  Record one gate assertion, naming it in the captured output.
 * @param ok non-zero when the assertion held.
 * @param what the assertion's name, logged only on failure.
 */
static void check(int ok, const char *what)
{
    if (!ok)
    {
        g_failures++;
        epic_harness_log("usb sim: FAIL ");
        epic_harness_log(what);
        epic_harness_log("\n");
    }
}

/**
 * @brief  Run the SIE and buffer-descriptor checks, then report.
 * @return 0 on pass, 1 on any failed assertion.
 */
int main(void)
{
    epic_harness_init(0u);

    epic_usb_init();
    /* Pump the polled path the example's main loop runs. No host exists
     * here, so this only proves service() enters and returns. */
    for (uint8_t i = 0; i < 8u; i++)
    {
        epic_usb_service();
    }

    /* Section 17.2.1/17.2.2: the module is on, not suspended, and packet
     * transfers are not disabled. */
    check(UCONbits.USBEN, "UCON.USBEN clear, SIE not enabled");
    check(!UCONbits.SUSPND, "UCON.SUSPND set, SIE suspended");
    check(!UCONbits.PKTDIS, "UCON.PKTDIS set, transfers disabled");

    /* Section 17.2.2: full speed, internal pull-up (the host only sees
     * D+ pulled up when this bit is set), internal transceiver. */
    check(UCFGbits.FSEN, "UCFG.FSEN clear, not full speed");
    check(UCFGbits.UPUEN, "UCFG.UPUEN clear, pull-up disabled");
    check(!UCFGbits.UTRDIS, "UCFG.UTRDIS set, external transceiver");

    /* Section 17.2.5: no host has addressed the device yet. */
    check(UADDR == 0u, "UADDR not 0 before enumeration");

    /* Section 17.2.4: EP0 is control with handshaking; EP1/EP2 carry
     * data with control transfers disabled on them. */
    check(UEP0bits.EPINEN && UEP0bits.EPOUTEN && UEP0bits.EPHSHK &&
          !UEP0bits.EPSTALL && !UEP0bits.EPCONDIS,
          "UEP0 control-endpoint bits wrong");
    check(UEP1bits.EPINEN && UEP1bits.EPOUTEN && UEP1bits.EPHSHK &&
          UEP1bits.EPCONDIS && !UEP1bits.EPSTALL,
          "UEP1 endpoint bits wrong");
    check(UEP2bits.EPINEN && UEP2bits.EPOUTEN && UEP2bits.EPHSHK &&
          UEP2bits.EPCONDIS && !UEP2bits.EPSTALL,
          "UEP2 endpoint bits wrong");

    /* Section 17.3/17.4.3: every buffer the descriptors point at must be
     * endpoint data RAM (bank 5 and up), inside banks 4-7. A buffer the
     * linker left in the general GPR banks is the defect this gate is
     * here to catch. The pointer is deliberately not const-qualified: on
     * PIC18, XC8 lowers a const pointer to a program-space reference, so
     * a const volatile read of 0x400 returns the wrong bytes entirely
     * (verified under MPLAB SIM, 2026-10-20). */
    volatile uint8_t *bd = (volatile uint8_t *)BDT_ADDR;
    for (uint8_t i = 0; i < BDT_COUNT; i++)
    {
        uint16_t addr = (uint16_t)bd[i * 4u + 2u] |
                        ((uint16_t)bd[i * 4u + 3u] << 8);
        uint16_t len = (uint16_t)bd[i * 4u + 1u] |
                       ((uint16_t)(bd[i * 4u] & BD_BC_MASK) << 8);
        check(addr >= USB_RAM_LO && (uint32_t)addr + len <= USB_RAM_HI,
              "BD buffer outside USB RAM 500h-7FFh");
    }

    /* Section 17.4.1.1: the SIE owns the EP0 OUT and EP2 OUT buffers
     * waiting for a transaction, and the device owns the IN buffers.
     * The byte counts are the configured endpoint lengths. The BDT
     * order is the datasheet's: ep0_out, ep0_in, ep1_out, ep1_in,
     * ep2_out, ep2_in (DS39632E Section 17.4, Table 17-4). */
    check((bd[0] & BD_UOWN) != 0u && bd[1] == 8u,
          "EP0 OUT descriptor not armed for 8 bytes");
    check((bd[4] & BD_UOWN) == 0u && bd[5] == 8u,
          "EP0 IN descriptor wrong for 8 bytes");
    check((bd[16] & BD_UOWN) != 0u && bd[17] == 64u,
          "EP2 OUT descriptor not armed for 64 bytes");
    check((bd[20] & BD_UOWN) == 0u && bd[21] == 64u,
          "EP2 IN descriptor wrong for 64 bytes");

    /* No host asserted DTR, so the module must not claim a connection. */
    check(!epic_usb_connected(), "connected() true without a host");
    check(epic_usb_available() == 0u, "bytes available without a host");

    if (g_failures == 0u)
    {
        epic_harness_log("usb sim: SIE enabled, full speed, pull-up on, ");
        epic_harness_log("BDT at 400h, 6 descriptors inside USB RAM\n");
    }
    else
    {
        epic_harness_log("usb sim: one or more checks FAILED\n");
    }
    epic_harness_log("usb sim: enumeration NOT exercised (no SIE model)\n");

    int rc = epic_harness_report(g_failures == 0u);
    /* Hold the verdict: XC8 re-enters main when it returns, which would
     * rerun usb_init() before mdb reads the capture. */
    for (;;)
    {
    }
    return rc;
}
