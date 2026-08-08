# Probe: does the v2 event-driven ISR fit its 521-cycle-per-bit budget?

**Verdict: NO for the brief's literal probe shape, YES for a straight-line
dispatch shape.** Measured with the brief's exact probe.c (array-of-pointers
`chans[2]` + a `for` loop over channels): coincidence case (two channels,
TX+RX all due) **796 cycles**, 53% over the 521-cycle budget; common case
(one slot due) **568 cycles**, 9% over budget. Neither fits. A follow-up
diagnostic probe, rewritten straight-line (two named channel variable sets,
no array of pointers, no loop, matching the shape v1's own probe
(`docs/superpowers/plans/probe-swuart-isr-budget.md`) already established as
necessary), measures **280 cycles** coincidence and **238 cycles** common,
comfortably under budget both ways (46% and 54% margin respectively).

**Consequence for Tasks 2-7: proceed, but only if Task 2's real
implementation uses straight-line, no-loop, no-array-of-pointers dispatch.**
This is not a new constraint invented here; it is v1's own already-measured
finding (loop + array indexing added ~2.4x overhead there too), which the
v2 plan's own probe.c inexplicably reintroduced. The good news is the
underlying workload (2 channels, TX+RX tracked per channel, coincidence
handled) is not inherently too expensive for the 521-cycle budget; the loop
shape was the entire gap. No design change (e.g. disallowing TX/RX
coincidence per channel) is needed based on these numbers.

Run on 2026-08-07 against XC8 v3.10 (native,
`/opt/microchip/xc8/v3.10/bin`), `-mcpu=16f877a -O2 -std=c99`, MPLAB SIM
under `mdb` (`/opt/microchip/mplabx/v6.35/mplab_platform/bin/mdb.sh`),
breakpoint-and-stopwatch technique per
`docs/superpowers/plans/probe-swuart-isr-budget.md`'s Step 5.

## What was run

### Step 1: probe code (one deviation from the brief)

`/tmp/swuart-v2-probe/probe.c`, copied from the v2 plan's brief verbatim
except for one line, the same deviation the v1 probe already needed and
documented: `LATB` does not exist on classic PIC16F877A (no LATx registers
on this family). Replaced both `LATB` references in `tx_bit()` with
`PORTB`. No other change.

### Step 2: compile and measure the coincidence (worst) case

```sh
mkdir -p /tmp/swuart-v2-probe && cd /tmp/swuart-v2-probe
export PATH=$PATH:/opt/microchip/xc8/v3.10/bin
xc8-cc -mcpu=16f877a \
  -mdfp=/opt/microchip/mplabx/v6.35/packs/Microchip/PIC16Fxxx_DFP/1.8.167/xc8 \
  -O2 -std=c99 -fasmfile -Wa,-a probe.c -o probe.hex -ginhx32
```

The brief's DFP path exists verbatim on this machine (confirmed via
`find /opt/microchip/mplabx/v6.35/packs -maxdepth 3 -iname "*16F*"` before
compiling); no substitution needed.

Output:

```
16F877A Memory Summary:
    Program space        used   1FFh (   511) of  2000h words   (  6.2%)
    Data space           used    39h (    57) of   170h bytes   ( 15.5%)
    EEPROM space         used     0h (     0) of   100h bytes   (  0.0%)
    Configuration bits   used     1h (     1) of     1h word    (100.0%)
    ID Location space    used     0h (     0) of     4h bytes   (  0.0%)
```

`probe.lst` puts the interrupt vector trampoline at `0x0004` (`__pintentry`)
and the ISR's `retfie` at `0x0124` (verified by reading the listing's
control flow: `_isr` body at `0x0038` falls through into the context-restore
block ending at `retfie`, confirmed there is exactly one `retfie` in the
file).

`chan_a` and `chan_b` are file-scope `static` with no initializer, so both
zero-init to all-fields-0 at reset, i.e. every `tx_ticks_left`/
`rx_ticks_left` already "due" (`<= elapsed`) on the very first TMR1
overflow. That is exactly the brief's coincidence case with no memory pokes
needed; letting TMR1 free-run from reset exercises it directly.

`mdb_cmds.txt`:

```
Device PIC16F877A
Hwtool sim
Program "probe.hex"
break *0x0004
break *0x0124
Run
Wait 5000
Stopwatch clear
Continue
Wait 5000
Stopwatch
quit
```

```sh
export PATH=$PATH:/opt/microchip/xc8/v3.10/bin:/opt/microchip/mplabx/v6.35/mplab_platform/bin
mdb.sh mdb_cmds.txt
```

Relevant output:

```
break *0x0004
Breakpoint 0 at 0x4.
break *0x0124
Breakpoint 1 at 0x124.
Run
Running
Single breakpoint: @0x4
Simulator halted
Stopwatch clear
Stopwatch cleared. Stopwatch cycle count = 0 (0 ns)
Continue
Running
Single breakpoint: @0x124
Simulator halted
Stopwatch
Stopwatch cycle count = 794 (794 µs)
```

794 cycles from the vector (`0x0004`, before its first instruction) to
`0x0124` (before `retfie`). Add `retfie`'s own 2 cycles: **796 cycles
total** for the coincidence case, against the 521-cycle budget.

`796 / 521 = 1.53x`, **53% over budget**, margin **-53%**.

### Step 3: measure the common case (one slot due)

Modified the probe per the brief: only `chan_a.tx_ticks_left` starts at 0,
everything else starts non-zero.

**A real pitfall found while doing this, worth recording**: the brief
suggests `521u` as the "non-zero, not due" example value. That value
silently reproduces the coincidence case instead of the common case. The
ISR's due check is `chan_tx_due(c) <= elapsed`, and `elapsed = g_last_delta`,
itself declared `static uint16_t g_last_delta = 521u;`. On the very first
tick, `elapsed == 521`. Setting the "not due" fields to exactly `521u`
makes `521 <= 521` true for all of them too, so all four slots fire, not
just one. First build with this literal value measured 797 cycles, nearly
identical to the coincidence case, which was the tell. Fixed by using
`1000u` (clear of `elapsed`) for the three fields that should not be due:

```c
static chan_t chan_a = { .tx_ticks_left = 0u, .rx_ticks_left = 1000u };
static chan_t chan_b = { .tx_ticks_left = 1000u, .rx_ticks_left = 1000u };
```

Rebuilt (`probe_common.c` -> `probe_common.hex`, same compile command,
`retfie` at `0x0111` per the new listing, memory summary `209h` = 521
words), re-measured with `mdb_cmds_common.txt` (same technique, breakpoints
at `0x0004` and `0x0111`):

```
Continue
Running
Single breakpoint: @0x111
Simulator halted
Stopwatch
Stopwatch cycle count = 566 (566 µs)
```

566 + 2 (`retfie`) = **568 cycles total** for the common case (one bit
action: `chan_a`'s TX bit fires; the other three fields are decremented,
not fired; then a final sweep computes the next minimum delta over all
four fields regardless of how many fired).

`568 / 521 = 1.09x`, **9% over budget**, margin **-9%**. The common case,
the shape most interrupts have in real operation, does not fit either with
this dispatch shape. The per-interrupt overhead of scanning all channels'
due-ness and recomputing the next timer reload is paid on every interrupt
regardless of how many slots are actually due, and that fixed overhead
alone (with this loop shape) already exceeds budget.

### Supplementary diagnostic: does a straight-line rewrite change the verdict?

The brief's probe.c code comment claims the two-named-channel shape "is
what XC8 -O2 compiles cheaply," citing v1's probe. But the actual code
builds `chan_t *chans[2] = { &chan_a, &chan_b };` and loops over it with
`for (uint8_t i = 0; i < 2u; i++)`, an array-of-pointers-plus-loop shape.
That is a materially different shape from v1's own winning probe
(`probe2.c` in `docs/superpowers/plans/probe-swuart-isr-budget.md`), which
used two named channel variable sets with no array and no loop at all, and
which v1 measured at 122 cycles (vs. 299 for an array-indexed-by-loop-
variable shape, a 2.4x difference from the loop/array overhead alone).

To find out whether v2's budget miss above is inherent to the workload or
just the reintroduced loop shape, wrote a diagnostic rewrite,
`/tmp/swuart-v2-probe/probe_straightline.c`: same fields, same worst-case
branch behavior, but two named channel variable sets (`a_*`/`b_*`), no
array of pointers, no loop, each channel's TX/RX handling and the final
min-delta computation written out directly.

Compiled and measured the same way (coincidence case: all fields zero-init;
common case: only `a_tx_ticks_left` starts at 0, rest at `1000u`):

| case | file | retfie addr | stopwatch cycles | + retfie(2) | vs 521 budget |
|---|---|---|---|---|---|
| coincidence | `probe_straightline.c` | `0x0121` | 278 | **280** | 0.54x, **+46% margin** |
| common | `probe_straightline_common.c` | `0x0124` | 236 | **238** | 0.46x, **+54% margin** |

Full mdb output for the coincidence case:

```
Continue
Running
Single breakpoint: @0x121
Simulator halted
Stopwatch
Stopwatch cycle count = 278 (278 µs)
```

Full mdb output for the common case:

```
Continue
Running
Single breakpoint: @0x124
Simulator halted
Stopwatch
Stopwatch cycle count = 236 (236 µs)
```

Both fit comfortably. The workload itself (2 channels, TX+RX ticks-left
tracking, coincidence handling, next-deadline recomputation) is not the
problem; the array-of-pointers-plus-loop dispatch is. This directly answers
the brief's Step 4 prompt about whether "the group-read dispatch
optimization from the v1 fix wave" matters here: it does, more than in v1,
because the fixed per-interrupt overhead (scan + reload-compute) is paid
even in the common case, and that overhead is exactly where the loop
shape's cost concentrates.

## Verdict (Step 4)

- **Common case, brief's literal probe.c shape: does NOT fit.** 568 cycles
  vs. 521 budget, 9% over, every single interrupt, not just the rare one.
- **Coincidence case, brief's literal probe.c shape: does NOT fit, and
  badly.** 796 cycles vs. 521 budget, 53% over. This is worse than v1's
  already-marginal 562-677 cycle measurement that motivated the v2 redesign
  in the first place, so the literal probe.c shape does not represent an
  improvement over v1 at all.
- **Both cases fit comfortably with a straight-line, no-loop,
  no-array-of-pointers dispatch shape**: 238 cycles common (54% margin), 280
  cycles coincidence (46% margin). This is the same shape v1 already
  established as required (`docs/superpowers/plans/probe-swuart-isr-budget.md`),
  now reconfirmed for v2's workload.
- The design spec's "a little jitter, corrected at the next event"
  reasoning is not in play here: with the straight-line shape, there is no
  overrun to correct in the first place, common or coincidence. With the
  loop shape, both cases run over on *every* interrupt (not just rare
  coincidences), which would not be jitter, it would be a sustained,
  unconditional cycle deficit; that reasoning does not rescue the loop
  shape and should not be invoked to justify it.
- **No design change is needed** (e.g. disallowing TX/RX coincidence within
  a channel by construction): the coincidence case fits with real margin
  once dispatch is straight-line, so there is no evidence forcing that
  restriction.

**Tasks 2-7 should proceed**, on the condition that Task 2's real
per-channel dispatch is written straight-line: two named channel variable
sets (or two channel structs accessed through compile-time-constant
pointers), no array indexed by a runtime loop variable, no per-channel
helper call left un-inlined, matching exactly the constraint v1 already
placed on itself. The margins here (46-54%) are healthier than v1's final
6% margin at N=4, but the real Task 2 state machine will add more states
than this probe's body (start-bit detection via pin-change interrupt,
framing/stop-bit checks, idle detection); if that growth is more than a
modest addition, re-run this same measurement against the real code before
trusting these margins, per the same caveat v1's probe already recorded.

Reload-value formula (for reference, matches the plan's own derivation):
`reload = 65536 - round(FOSC_HZ / 4 / baud)` for N=1 event-driven timing;
`20000000 / 4 / 9600 = 520.83`, rounds to 521, matching `g_last_delta`'s
initial value in both probes.
