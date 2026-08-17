# pic16f88x-hal architecture

How the PIC16F88X HAL is structured and why. The shared contract
(naming, handle pattern, harness, interrupt model) is in
`epic-common/MANUAL.md`; this document covers what is per-family.

## The family in one paragraph

PIC16F88X is a classic mid-range family (35 instructions, RP0/RP1
banking, single interrupt vector at 0x0004, 8-level stack) with a
richer peripheral set than the 87XA: an enhanced Timer1 with gate, an
EUSART with BRG16/auto-baud/wake-up, an MSSP with I2C address mask, a
10-bit ADC with per-pin analog select, two independent comparators
with an SR latch, an ECCP with dead-time/steering/auto-shutdown, an
internal oscillator with fail-safe monitor, and ULPWU. Architecturally
it is the 87XA's sibling (same core, same banking model), so
`pic16f87xa-hal` is the template; every register address, bit and
reset value is 1:1 from DS40001291H.

## Build-time split

The host-vs-target split is done at build time via the include path,
never `#ifdef` in application code:

- `include/host/pic16f88x_platform.h`: SFR access indexes a 512-byte
  memory-backed register file (`src/sim/pic16f88x_sim.c`), so tests
  can poke registers directly. `EPIC_WEAK` is GCC's `__attribute__((weak))`.
- `include/target/pic16f88x_platform.h`: SFR access is a direct
  volatile deref; `EPIC_WEAK` is empty (XC8 has no weak symbols);
  `EPIC_PLACE(addr)` maps to `__at(addr)`.

The CMake host build puts `include/host` first; the manifest's
real-target build puts `include/target` first. The same source files
compile both ways.

## The banking layer

The 88X spreads SFRs across all four banks. The 87XA's proven
inline-asm RMW pattern (load into W through a common-RAM scratch byte,
bank in, one instruction, bank out) ports directly, with two deltas:

1. **Bank 2 macros** (`EPIC_BANK2_READ8/WRITE8`): the 88X keeps
   WDTCON, CM1CON0/CM2CON0/CM2CON1 and the EEPROM data/address
   registers in Bank 2. The 87XA only needed Banks 1/3.
2. **Bank 3 macros** (`EPIC_BANK3_READ8/WRITE8`): SRCON, BAUDCTL,
   ANSEL/ANSELH, EECON1/EECON2.

Both set/clear RP1:RP0 explicitly (the peripheral code interleaves
banks back to back, so the incoming bank can't be assumed) and exit to
Bank 0. The scratch bytes (`epic_irq_pie_scratch`, `epic_bank1_scratch`)
are `__at`-pinned to common RAM 0x70/0x71 in `src/target/
pic16_isr_vector.c`.

Why inline asm at all: XC8 v4.00 can misdirect a plain C local assumed
to live in Bank 0 while a bank is selected, so an ordinary
read-modify-write on a banked SFR silently corrupts. The HARNESS=sim
probe (`tests/sim_bank_probe.c`) exercises every banked access site
under MPLAB SIM with known values; the mdb gate fails on any
misdirection.

## Interrupt model

Single vector at 0x0004 (DS40001291H §14.11). `src/target/
pic16_isr_vector.c` clears RP1:RP0 (the preempted main line can be
inside a bank-macro window) and calls `epic_dispatch_all_irqs`, which
reads INTCON/PIR1/PIR2 once and branches on the bits. The 88X IRQ set
differs from the 87XA: no PSP, and PIR2 gains C1IF/C2IF (separate
comparator flags), ULPWUIF and OSFIF.

The dispatch gates three sources on their enable bits, not just the
flag:

- **TMR1IF**: Timer1 free-runs with its overflow interrupt disabled
  (epic-swuart needs the counter, never the overflow), so TMR1IF
  latches at every 65536-cycle wrap. Without the gate every later
  event pays the full handler cost; the stale flag is dropped when the
  source is disabled.
- **TXIF**: a read-only status bit that stays set whenever TXREG is
  empty. Without the gate every ISR calls the TX handler's callback
  through XC8's PC-relative function-pointer table, which requires the
  callback to share the table's flash page.
- **EEIF**: EEPROM completion is often polled with EEIE off, so the
  flag is left untouched when the source is disabled (the polling
  consumer owns it).

The enable/disable/flag table in `src/core/pic16_irq.c` maps each IRQ
to its INTCON/PIR/PIE bit; the PIE writes go through the banked
macros (PIE1/PIE2 are Bank 1).

## Handle storage

Every driver copies the caller's handle into driver-owned storage
(`g_*_storage`) and keeps a pointer to the copy. The 87XA's
pointer-holding version was a confirmed dangling-pointer bug
(epic-common/MANUAL.md §3.3): the caller's handle is typically
stack-local, out of scope by the time the ISR reads it. The storage is
unpinned: the linker's best-fit scatter packs it, and the smallest
part (882, 128 B RAM) has no Bank 2/3 GPR to pin into.

## Flash page pinning

`epic_dispatch_all_irqs` is pinned to 0x900 on parts with >= 4K flash:
XC8 emits no PCLATH setup for the handler calls (it assumes the
interrupt call-graph is linked into one flash page), so the dispatch
and handlers must share a page. The 882 (2K flash, one page) needs no
pin; the pin is conditional on `PIC16F88X_FAMILY_FLASH_KW >= 4`.

## The sim backend

`src/sim/pic16f88x_sim.c` provides the 512-byte register file and the
peripheral models: Timer0/1/2 stepping (prescaler, overflow flags),
the EUSART TXIF re-assert (TXIF is read-only and stays set while TXEN
is on), and the drive/read helpers (`pic16f88x_sim_drive_*`) that
inject RX bytes, ADC results, EEPROM writes, comparator transitions and
oscillator-fail events. The sim never bit-bangs external pins; the test
rig drives and observes them through the helpers.

## The mdb harness

`src/mdb/pic16_harness_mdb.c` is the HARNESS=sim firmware: it
configures the EUSART for polled output (9600 baud, BRGH=1) and writes
the `EPIC_HARNESS_RESULT: PASS/FAIL` marker. The banked-SFR probe
(`tests/sim_bank_probe.c`) runs under MPLAB SIM and reports each failed
check as a hex index, so a bank misdirection is diagnosable from the
captured UART output.

## Why the reference project is HAL-only

The 87XA reference project links every epic-* module and sits at 97.8%
of the 16F877A's RAM. The 88X HAL's correct handle-copy pattern plus
three extra drivers (osc, ulpwu, srlatch) tips the same all-modules
link over the 16F887's 368 B. The 88X reference project therefore
mirrors the 193X precedent: HAL + epic-common only, a Timer0-overflow
blink, at ~35% RAM. Consumers add modules through the bundle's
epic-hal.mk or the epic-hal CLI, which select only what they need.
