# Layout budgets and pinned resources (PIC16F87XA / PIC16F193X)

Status: 2026-08-11. One place for the recorded XC8 layout constraints
that have cost this repo more churn than any single bug class. Each
constraint below was discovered by a probe or a gate failure, not
assumed; the evidence pointer is in parentheses. The top rule: before
trusting an uncertain placement or address behavior, build a probe and
inspect the .s / .sym / .map (AGENTS.md).

## Flash budgets

- **Page-0 ISR body (87XA)**: the `__interrupt` body must stay in
  flash page 0 because the vector's goto is PCLATH-less. Past ~4.2 K
  words total the linker scatters the ISR body and hand-asm internal
  gotos across pages and aborts with fixup-overflow 1356
  (docs/toolchain-coverage.md "Simulator and toolchain findings").
  Pure-C code elsewhere can live on any page (the compiler emits
  PCLATH management for calls); the constraint is the ISR body and
  any hand-asm routine with internal gotos.
- **Hand-asm internal gotos**: a `goto` inside hand-asm is
  page-local only. This is why the epic-math PIC16 asm leaves are
  planned to be pinned below 0x800 (quality task 5a) so the full
  golden-vector selftest can run on PIC16 without the 1356 aborts.

## Stack budget

- The PIC16 hardware stack is 8 levels. A live tick ISR (2 levels:
  vector + handler) leaves 6 for the main line; the lcd driver's
  ops-function-pointer send path with its dispatch chain overflowed
  it (the C10 gate moved to PIC18 for this). Keep ISR depth <= 2
  and budget main-line depth against 8, not 8 minus nothing.

## GPR placement (banked families)

- **IRP-baked ISR pointer derefs (87XA)**: XC8 v4.00 emits a
  CONSTANT IRP select into ISR-context 8-bit-pointer derefs. The
  two observed ISRs bake opposite windows: TIMER0_IRQHandler uses
  `bsf STATUS,7` (banks 2/3), USART_TX_IRQHandler uses
  `bcf STATUS,7` (banks 0/1) (disassembly probes, 2026-08-11). Any
  storage deref'd from an 87XA ISR through a pointer must live in
  the window that handler bakes, so it is `EPIC_PLACE`-pinned:
  `g_t0_storage` at 0x130 (bank 2, 6 bytes), `s_usart_handle` at
  0xA0 (bank 1, 7 bytes). Direct symbol accesses (e.g. the CCP ISR's
  `g_ccp_callbacks[i]`) get auto-banksel and are bank-correct
  anywhere; only pointer derefs bake.
- **FSR1 indirect (193X)**: pointer derefs compile to FSR1 indirect
  (`movwf fsr1l; clrf fsr1h; moviw`), which reaches any bank; the
  193X statics are verified safe unpinned (Finding 1 in
  pic16f193x-hal/docs/ARCHITECTURE.md; timer0 ISR disassembly probe,
  2026-08-11).
- **PIC16 bank-2/3 pointer reachability**: a runtime-computed SFR
  address on classic PIC16 compiles to FSR1:INDF1, which needs IRP
  set for banks 2/3; the 87XA CCP incident (baked IRP=1) is the
  cautionary record.
- **Math scratch (epic-math)**: the shared 12-byte
  `pic16_mscratch` is pinned to common RAM 0x72-0x7D, clear of the
  HAL RMW scratches (0x70/0x71) and the compiler's top-of-common-RAM
  working area (0x7E/0x7F). The 16-byte original at 0x70 overlapped
  all three (XC8 1262/1482/2084, found 2026-08-11); the mul_u16
  layout was compressed to 12 bytes (bk reuses b's bytes) to fit.
  Per-routine scratch is a single struct per routine so the linker
  cannot split it across banks (epic-math/docs/ARCHITECTURE.md).
- **Linker best-fit**: unpinned `static` GPR is scattered by
  best-fit, not declaration order. Anything bank-sensitive gets
  `EPIC_PLACE`; the statics audit (scripts/statics-audit.py) flags
  unpinned IRQ-shared multi-byte statics mechanically.

## SFR bank rules

- Classic PIC16 (87XA): RP0/RP1 bank bits; `STATUS,7` (IRP) selects
  a bank PAIR. Bank-1 SFR RMW (OPTION_REG, PIE1/2, TRIS...) must use
  the bank-absolute `EPIC_BANK1_READ8` path or the read silently
  misdirects to the bank-0 alias (sim_bank_probe, 2026-08-09).
- The 87XA PIE2 memory-map truth: PIE2 = 0x8D is Bank 1. A "fix"
  that moved it to Bank 2 silently rerouted PIE2-source
  Enable/DisableSrc into EEADR (2026-08-09 incident; the sfr-map
  audit now guards the whole map).
- 193X: real BSR (32 banks x 128 B); runtime-dispatched SFR
  addresses compile to safe FSR1:INDF1 (Finding 1, verified).

## Pinned-address inventory (87XA)

| Address | Size | Object | Why |
|---|---|---|---|
| 0x72-0x7D | 12 | pic16_mscratch (epic-math) | common RAM, clear of HAL/compiler |
| 0x70 | 1 | epic_irq_pie_scratch (HAL) | common RAM, asm operand reach |
| 0x71 | 1 | epic_bank1_scratch (HAL) | common RAM, asm operand reach |
| 0xA0 | 7 | s_usart_handle (harness) | USART ISR bakes IRP=0 |
| 0x100-0x490 | ~0x390 | epic-math PIC16 asm leaves (10 fns) | internal gotos, page 0 only |
| 0x190 | 6 | g_t0_storage (timer0) | Timer0 ISR bakes IRP=1; bank 3 keeps bank 2 contiguous |

New pins must land outside these ranges (the linker skips pinned
addresses, so a collision only happens against another pin or an
SFR; the table exists so a future pin fails loudly in review).

The toolchain-coverage.md "Simulator and toolchain findings"
section is the companion record of what the sim cannot do.
