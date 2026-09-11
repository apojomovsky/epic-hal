# PIC16F628A HAL manual

Register reference for the PIC16F628A family HAL (DS40044G). Every
address below is DFP-verified against `Microchip.PIC16Fxxx_DFP`
`edc/PIC16F628A.PIC` and pinned by `scripts/gen-sfr.py --family
PIC16F628A` (`--check` runs in CI). Shared conventions (naming, handle
pattern, harness, interrupt model) live in `epic-common/MANUAL.md`;
shared driver behavior lives in `pic14-midrange-core/` sources. This
manual covers only what is actually per-part: the register placement
deltas vs the PIC16F87XA.

## Address map (Bank 0)

INDF 0x00, TMR0 0x01, PCL 0x02, STATUS 0x03, FSR 0x04, PORTA 0x05,
PORTB 0x06, PCLATH 0x0A, INTCON 0x0B, PIR1 0x0C, TMR1L 0x0E, TMR1H
0x0F, T1CON 0x10, TMR2 0x11, T2CON 0x12, CCP1CON 0x17, CCPR1L 0x15,
CCPR1H 0x16, RCSTA 0x18, TXREG 0x19, RCREG 0x1A, CMCON 0x1F.

CMCON in Bank 0 is the headline delta: the 87XA keeps it in Bank 1
(0x9C). Bit layout is identical (CM2:CM0, CIS, C1INV, C2INV, C1OUT,
C2OUT). The shared comparator driver selects the bank per part.

PIR1 carries the flags the 87XA keeps in PIR2: CMIF is PIR1 bit 6 and
EEIF is PIR1 bit 7 (TXIF bit 4, RCIF bit 5, as on the 87XA). There is
no PIR2/PIE2. The shared IRQ core takes these rows from the family's
translation table; the dispatcher has no PIR2 block on this part.

## Address map (Bank 1)

OPTION 0x81, TRISA 0x85, TRISB 0x86, PIE1 0x8C, PCON 0x8E, PR2 0x92,
TXSTA 0x98, SPBRG 0x99, VRCON 0x9F, EEDATA 0x9A, EEADR 0x9B, EECON1
0x9C, EECON2 0x9D.

The EEPROM block in Bank 1 is the second headline delta (Bank 2/3 on
the 87XA). EEDATH/EEADRH do not exist (128 B need no high byte); the
shared driver never references them on any part.

VRCON replaces the 87XA's CVRCON at a new address (0x9F vs 0x9D) with
VR-named fields in the same positions (VR3:VR0, VRR, VROE, VREN) plus
no VRSS bit. VRR=1 selects the low range on VRCON parts; the 87XA's
CVRR bit has the opposite polarity (its own datasheet's register
table). The shared driver programs each form's own polarity; the mdb
bank-probe gate asserts the VRCON image (0xC8 for high range, tap 8,
output enabled) on silicon.

## Absent silicon

PORTC/D/E (+TRIS), PIR2/PIE2, SSP (SSPBUF/SSPCON/SSPSTAT/SSPADD),
ADC (ADCON0/1, ADRESH/L), CCP2, SPBRGH/BAUDCTL, OSCCON, WDTCON,
EEDATH/EEADRH. Referencing any of these fails at compile time: the
family SFR header omits them deliberately.

## XC8 codegen: banked SFRs need literal-token access

The shared EEPROM driver once accessed EEDATA/EEADR/EECON1/EECON2
through `EPIC_REG8` with a runtime address plus `pic_select_bank`
(the fallback branch of `pic14_eeprom.c`, taken on any part without
`EPIC_BANK3_WRITE8`). On XC8 v4.00 that shape silently misdirects for
this part: the EEDATA/EEADR writes landed in Bank 0 GPRs while the
EECON1/EECON2 writes landed, so the bank probe's WR-initiation check
passed and only a data-register readback exposed it (EEADR read 0x01
instead of 0x20 after a driver `WriteByte(0x20, ...)`). The failure was
invisible to code review, the host simulator (gcc lowers the same code
correctly), and a clean compile-and-link; only the mdb readback caught
it, the exact class `docs/adding-a-device.md` §4 exists for.

The fix routes Bank-1-only parts through the same literal-token
`EPIC_BANK1_*` macros the rest of the family uses (assembler-resolved
`movwf EEDATA`, no runtime address), selected by
`PIC14MIDRANGE_HAS_EEPROM_BANK1`. Bank-0 variable-address access is
unaffected (no banking to get wrong: the CCP driver's
`EPIC_REG8(a->cprl)` table lookups work), but any future banked
register accessed through a variable address must be re-proven on
silicon per §4, not assumed from the host run.

## MPLAB SIM limits (EEPROM)

The SIM never completes a CPU-initiated EEPROM write: WR stays set and
EEIF never fires, so write-complete-readback is unobservable in SIM
(same as every other family; the host sim backend models completion
instead). Reads are fine: the RD strobe completes within a few
instructions, and a driver `ReadByte` on an erased cell returns 0xFF
(gated ad-hoc, 2026-09-11). A read strobed while a write is still
pending returns the EEDATA latch, so read-path gates must run with no
prior write in the same boot.

## Gate record

`tests/sim_bank_probe.c` (CI sim gate, 16F628A, 15 s wall) asserts on
silicon, each with a check index: CMCON image 0x12 (0x00), VRCON
image 0xC8 (0x02), EEADR image 0x10 (0x03), EEDATA image 0xC3 (0x04),
WR initiation (0x05), write-incomplete (0x09), PIE1 CMIE (0x0B) and
EEIE (0x0C) rows, plus GPIO/TRISA images (0x06-0x08). PR2/PIE1/USART
sites are covered by the same shapes; Timer1 advance, CCP1 PWM
images (CCP1CON 0x2C, CCPR 12 at duty 50), and the erased-cell
`ReadByte` were proven ad-hoc on silicon (2026-09-11) and are
documented here rather than in the size-constrained probe (2K flash).
The probe reports once and halts: XC8 restarts `main()` on return and
the SIM analog-pin model latches after the comparator/VREF block, so
later iterations would re-fail the GPIO reads.

## GPIO widths

PORTA implements RA0..RA7 with RA5 input-only, so the implemented-pin
mask is 0xDF (not a contiguous width). PORTB is a full 8 bits. The
shared GPIO driver takes the mask from `PIC14MIDRANGE_PORTA_MASK`.

## Interrupts

Single vector at 0x0004, no priority (as on the 87XA). Ten sources:
RB, INT, TMR0, TMR1, TMR2, CCP1, USART TX/RX, EEPROM, CMP. The 2K
flash is a single page, so the dispatcher carries no `__at(0x900)`
page pin (as on the 2K 16F882).

## Flash and RAM budget

2 KW flash, 224 B RAM (three GPR banks + 16 B common), 128 B data
EEPROM. The blink firmware uses 1089 words / 70 B; the bank probe
uses 1945 words / 135 B. Peripheral-heavy applications must watch
both budgets; see `docs/adding-a-device.md` §3.2.
