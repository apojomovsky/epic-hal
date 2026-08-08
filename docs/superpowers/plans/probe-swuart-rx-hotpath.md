# Probe: measure the real RX hot-path overhead and verify the fix

**Verdict: the fix works, with one honest caveat.** `rx_capture_event_fast`'s
RX_IDLE branch costs 494 cycles edge-to-retfie on real PIC16F877A
(MPLAB SIM, XC8 v3.10, `-O2`, DFP 1.8.167), and with
`RX_CAPTURE_OVERHEAD_CYCLES = 325` the first (d0) sample deadline
lands at **1.503 bit periods after the start-bit edge**, mid-d0, no
65536-cycle wraparound. The old placeholder `0u` put that same deadline
at **2.12 bit periods, inside d1's window**, a guaranteed mis-sample;
the constant is load-bearing, not cosmetic. The caveat: the steady-state
per-bit re-arm has only **34 cycles of margin** against the 521-cycle
budget, far tighter than the 233-cycle margin the plan's TX-side
288-cycle estimate implied. It fits deterministically, but this is a
real number to know about, not "fine".

## What was run

Probe source `/tmp/swuart-rx-hotpath-probe/probe.c` (outside the repo),
PIC16F877A, linking the real, unmodified `epic-swuart/src/epic_swuart.c`
plus `pic16f87xa_gpio.c` / `pic16f87xa_timer1.c` / `pic16f87xa_ccp.c` /
`pic16_irq.c` / `pic16_irq_dispatch.c` / `pic16_isr_vector.c`, and
link-satisfying empty stubs for the peripherals not under test
(`TIMER0`/`TIMER2`/`SSP`/`USART_RX`/`USART_TX`/`ADC`/`EEPROM`/`COMP`/
`PSP` IRQ handlers), the same stub pattern v3's probe used. Compiled
with:

```sh
xc8-cc -mcpu=16f877a -I$REPO/pic16f87xa-hal/include/target -I$REPO/pic16f87xa-hal/include \
  -I$REPO/epic-common/include -I$REPO/epic-swuart/include \
  -mdfp=/opt/microchip/mplabx/v6.35/packs/Microchip/PIC16Fxxx_DFP/1.8.167/xc8 \
  -O2 -std=c99 -fasmfile probe.c stubs.c \
  $REPO/epic-swuart/src/epic_swuart.c \
  $REPO/pic16f87xa-hal/src/peripherals/pic16f87xa_gpio.c \
  $REPO/pic16f87xa-hal/src/peripherals/pic16f87xa_timer1.c \
  $REPO/pic16f87xa-hal/src/peripherals/pic16f87xa_ccp.c \
  $REPO/pic16f87xa-hal/src/core/pic16_irq.c \
  $REPO/pic16f87xa-hal/src/core/pic16_irq_dispatch.c \
  $REPO/pic16f87xa-hal/src/core/pic16_isr_vector.c \
  -o probe.hex -ginhx32
```

`probe.c` is exactly the plan's Task 2 Step 1 source: `EPIC_SWUART_Init`
on GPIOC/RC2 RX at 20 MHz / 9600 baud, then an empty loop.

## Code layout (from `readelf -s` + `mdb` `x /i`)

- Vector entry: `0x0004` (XC8 trampoline: save W/STATUS/FSR/PCLATH,
  `GOTO` dispatch).
- `_PIC16_IRQ_Handler` (dispatch entry): `CALL` to
  `epic_dispatch_all_irqs` at `0xF2B`, then context restore ending in
  `RETFIE` at **`0x0B30`**.
- `_CCP1_IRQHandler` at `0x0B31`: clears CCP1IF, dispatches the
  registered callback through XC8's computed-goto thunk (`CALL 0x29`).
- `_on_rx_event_a` at `0x0ADB`: loads `g_chan_a`, `CALL 0x35` into the
  inlined `rx_capture_event_fast` body at `0x35`.
- The RX_IDLE branch (`goto 0x187` when `rx_state == 0`) calls
  `EPIC_TIMER1_ReadCounter` at `0x0B59` via `CALL 0x359` at `0x1DC`,
  then writes CCPR1L/H and CCP1CON (`COMPARE_SOFT_IF`) at
  `0x222`/`0x236`/`0x238`.
- The steady-state branch's last re-arm write (CCPR1H) is at `0x185`.
- Post-Init halt point: `main`'s self-loop `GOTO 0x44E` at `0x0C4E`
  (corrected build: `0x0C4B`).

## Measured results (reproduced identically across 3 runs)

Breakpoint-and-stopwatch technique, same as `probe-swuart-v3-ccp-cost.md`:
halt at the post-Init loop, `write pin RC2 high`, `write pin RC2 low`
(real falling edge into CCP1's capture pin), `Stopwatch clear`,
`continue` to each breakpoint, read `Stopwatch`. `wait` after every
`run`/`continue` (scripted mdb is async; reading registers without it
samples mid-Init, a mistake caught in-run by TMR1 reading 0).

### RX_IDLE branch (the fix's core claim), OVERHEAD=0 build first

| Segment | Addresses | Cycles | TMR1 at end |
|---|---|---|---|
| Edge to vector entry | pin toggle -> 0x0004 | 3 | 1306 |
| Vector to Timer1 read | 0x0004 -> 0x1DC (CALL) | 322 | 1628 |
| Timer1 read to retfie | 0x1DC -> 0x0B30 | 161 | 1789 |
| **Total, edge to retfie** | | **486** | |

CCPR1 latched the edge at **1303**; `now` read at 1628, so
**edge-to-read = 325 cycles** (3 + 322). Cross-checked against TMR1
deltas, not just the stopwatch: identical in all three runs.

### Corrected build (RX_CAPTURE_OVERHEAD_CYCLES = 325 compiled in)

| Segment | Addresses | Cycles | TMR1 at end |
|---|---|---|---|
| Edge to vector entry | pin toggle -> 0x0004 | 3 | 1306 |
| Vector to Timer1 read | 0x0004 -> 0x1DC | 320 | 1626 |
| Timer1 read to retfie | 0x1DC -> 0x0B2D | 171 | 1797 |
| **Total, edge to retfie** | | **494** | |

The deadline armed: `now + 781 - 325` = 1626+4 (CALL+prologue latency
inside ReadCounter) + 781 - 325 = **2086**, confirmed by reading CCPR1
back as 2086 at the d0 vector entry. Sample point = 2086 - 1303 = 783
cycles after the edge = **1.503 bit periods** (bit period = 521).
Intended: 1.5, mid-d0 (d0 spans [1.0, 2.0)). **Dead on.**

Contrast with the placeholder build: deadline = 1628 + 781 - 0 = 2409,
= 2.12 bit periods after the edge, inside d1's first 12%. The plan's
warning that a guessed/zero constant mis-samples is empirically
confirmed.

### Steady-state per-bit event (d0 compare), corrected build

| Segment | Addresses | Cycles | TMR1 at end |
|---|---|---|---|
| d0 vector entry | 0x0004 | | 2090 |
| d0 vector entry to re-arm write | 0x0004 -> 0x185 | 483 | 2573 |
| re-arm write to retfie | 0x185 -> 0x0B30 | ~51 | 2624 |
| **Total, vector to retfie** | | **534** | |

The d1 deadline = 2086 + 521 = 2607; TMR1 reaches it 517 cycles after
the d0 vector entry. The re-arm write lands at +483. **Margin = 34
cycles** (6.5% of the bit period). Deterministic (fixed instruction
count, no data-dependent loops in this path), so it fits, but this is
the real number: the RX per-bit event costs 534 cycles total, nearly
double the 288-cycle TX-side compare event v3's probe measured, and the
plan's "288/521 is representative of this same class" assumption does
not transfer to RX. Root cause visible in the disassembly: the
steady-state branch pays a full `EPIC_GPIO_ReadPin` call (table-driven
port lookup) plus the FSR-indirect deadline store through the handle
struct, work the TX compare event does not do.

## Derived constant

`RX_CAPTURE_OVERHEAD_CYCLES = 325` = 3 (edge-to-vector interrupt
response) + 322 (vector-to-Timer1-read software latency), measured on
the real compiled handler. Derivation recorded, not asserted: each
component was a separate breakpoint-pair stopwatch reading, and the
TMR1 deltas between those same breakpoints agree exactly. The corrected
build validates the value end-to-end: d0 lands at 1.503 bit periods
with it, and the constant does not swing with rebuilds (the corrected
build's edge-to-read is 323, a 2-cycle codegen shift; 325 still lands
the sample at 1.50 bit periods because the deadline arithmetic consumes
the constant at compile time and the residual is sub-bit-period).

## Verdict

- **Race removed.** The d0 deadline is a relative reload off a fresh
  Timer1 read taken after the handler's own latency has elapsed; it
  can never be in the past. No 65536-cycle wraparound observed: the d0
  compare fired 293 cycles after the RX_IDLE retfie (TMR1 1797 to
  2090), not ~65000 cycles later.
- **Sample point correct.** d0 lands at 1.503 bit periods after the
  edge, mid-d0, with the measured constant.
- **Margin honest.** RX_IDLE itself fits 494/521 (94.9%). The
  steady-state per-bit path fits with only 34 cycles of margin, not
  the comfortable margin the TX-side estimate suggested. Worth a
  follow-up look (e.g. a direct-SFR GPIO read in the steady-state
  branch, the same trick this fix already applies to CCP1), but it is
  deterministic and it fits; the port to PIC18Fxx5x / PIC16F193X
  channel B should re-measure rather than inherit this number.
- **Known limitation restated:** this probe measures PIC16F87XA channel
  A only. PIC18Fxx5x and PIC16F193X channel B still carry the old,
  racy confirm-then-arm mechanism until a follow-up ports this pattern.
