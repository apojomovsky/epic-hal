# Combination matrix: peripherals and modules exercised together

Status: **complete 2026-08-09**. All 12 combo gates land and PASS
(112/112 matrix builds, 32/32 sim gates, 6/6 bundles). Bugs found and
fixed: the PIE2 bank regression (C11) and the un-gated EEIF dispatch
branch (C7). Simulator limits mapped (inert 16F877A EEPROM, TMR0IF,
the wedge classes). Follow-ups recorded in docs/toolchain-coverage.md:
the swuart/CCP 8-bit-handle-pointer hardening (landed 2026-08-10: the
CCP driver stores driver-owned callbacks, so the C11 gate no longer
pins its handle) and the PIC16 math pin.

## Problem

The gate campaign (docs/toolchain-coverage.md) exercised every module
alone. The real failure classes found so far all lived in
*interactions*: the SSPSTAT RMW corrupted SSPCON while the USART was
active, the OPTION_REG RMW corrupted TMR0, the EEPROM bank-2/3 macros
interleaved back to back, the TXIF dispatch wedged the ISR, the GIE
race tore the tick counter. Single-module gates cannot see the next
layer: two peripherals or two modules sharing one CPU, one ISR, one
bank state. This plan builds the combination matrix and hunts those
bugs.

## The matrix

Each row is a self-reporting sim example (the established harness
contract) exercising the listed components *simultaneously* with
cross-checks, as a permanent mdb gate. New manifest modules
(`epic-combo-*`), each `depends_on` the real modules it combines, one
example per family, sim variant per the established pattern.

### HAL peripheral interleaves (the bank/ISR surface)

| Combo | Components | Cross-checks | Family |
|---|---|---|---|
| C1 uart-ssp | USART TX (ISR-driven) + SSP I2C master + EEPROM read | bytes byte-exact in capture while SSP busy; SSPCON/SSPSTAT uncorrupted; EEPROM read correct | 87XA |
| C2 multitimer | TMR0 + TMR1 + TMR2 + USART, all interrupt-driven | each timer's callback count advances; tick counter monotone; TX still byte-exact; no GIE wedging | 87XA |
| C3 adc-uart | ADC conversion loop + TIMER1 + USART TX | conversion result stable, reported over UART, timer keeps ticking | 87XA |
| C4 rb-uart | RB change ISR + USART TX + TIMER2 | RB callback fires on port change (mdb pin write), TX unaffected | 87XA |
| C5 eeprom-isr | EEPROM write under a live TIMER2 ISR | write completes (eeprom_writes runner arg), bytes land, ISR count intact | 87XA |

### Module combos

| Combo | Components | Cross-checks | Family |
|---|---|---|---|
| C6 tick-serial | epic-tick + epic-serial | tick-driven periodic TX, byte-exact payloads, ring never blocks, tick monotone | 87XA |
| C7 tick-settings | epic-tick + epic-settings | save/load round trip under the live 1 ms tick ISR, CRC valid, defaults on corruption | 87XA |
| C8 taskmgr-serial | epic-taskmgr + epic-serial | tasks doing UART writes at different rates, scheduler counts correct, ring coherent | PIC18 |
| C9 encoder-tick | epic-encoder + epic-tick | scripted quadrature, position readback under live tick, no torn reads | 87XA |
| C10 lcd-tick | epic-lcd + epic-tick | real HD44780 init/print with real tick delays, then report (stack constraint noted) | 87XA |
| C11 swuart-tick | epic-swuart + epic-tick | swuart TX byte paced by tick, drains, tick alive | 87XA |
| C12 modbus-full | epic-modbus + epic-serial + epic-tick | FC03 frame built and transmitted with a live tick ISR running, CRC correct | PIC18 |

## Expected bug classes

- Bank-state corruption when two banked peripherals interleave (the
  class-B mechanism under a live second peripheral).
- ISR dispatch interactions: multiple pending flags, the TXIE gate
  under load, GIE wedging (Finding 10(a) class).
- Ring-buffer races under preemption (the class-G Disable/Restore
  sites with a genuinely competing ISR).
- Layout: combo builds are large; PIC16 gates may hit the flash-page-0
  constraint (then move to PIC18 like epic-pid).

## Acceptance

- Every combo gate PASSES under mdb in CI; each found bug fixed at
  source and the gate kept as the regression.
- No regressions: existing 20 gates, full matrix, host suite green.

## Process

Per combo: write the sim example, emit, run under mdb, fix what it
exposes, commit. C1 first (the highest-value interleave); then C2/C6
in parallel with subagents; then the rest.
