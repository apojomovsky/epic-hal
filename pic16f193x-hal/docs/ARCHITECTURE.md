# PIC16F193X HAL, XC8 codegen notes

## Why this exists

This file records compiler/codegen behavior on the PIC16F193X
(Enhanced Mid-range) that is invisible to code review, the host
simulator, and a clean `xc8-cc` compile-and-link, in the same spirit as
`pic16f87xa-hal/docs/ARCHITECTURE.md` and `pic18fxx5x-hal/docs/ARCHITECTURE.md`.
It is filled in as the §4 verification gate (`docs/adding-a-device.md`)
surfaces real findings, not from assumption.

The two existing families each had codegen bugs of the same shape: code
that looks correct, builds clean, links clean, and silently writes the
wrong thing to a register at runtime. The classic PIC16 bug was an SFR
access misdirected while a bank switch (RP0/RP1) was in effect via a C
local/parameter; the PIC18 bug was a runtime SFR address compiled to
program-memory table access instead of data-memory access. Neither was
caught short of running the firmware under `mdb` and reading the
registers back. The 193X uses a different banking scheme (BSR) and a
different core, so neither result carries over automatically.

## Status

Real-target build passes for all six parts (`Microchip.PIC12-16F1xxx_DFP`
1.9.258 installed). Finding 1 below is the completed codegen probe for
the two known-risky patterns, done by disassembling the actual linked
firmware (`xc8-cc ... -o firmware.elf` then reading the generated
`.s`, since a bare `-S` on a non-`main` translation unit does not
trigger XC8's full optimizer here; linking the real `example_blink`
image and dumping its `.s` does). `mdb` (MPLAB SIM, headless) is now
installed and confirmed working; Timer1 has been through the
real-register readback half of the §4 gate (Finding 2; `PORTA=1` PASS,
`PIE1=0x01` confirmed), the other peripherals have not yet (see "Open,
for whoever picks this back up" below). Finding 1 covers the codegen
inspection half only.

## Finding 1: runtime SFR addresses route through FSR1:INDF1, not BSR; literal tokens auto-bank correctly

**Probed, both patterns clean, no misdirection found for the read-only
half. The RMW half is broken; see Finding 2.**

Every function that dispatches a runtime value to one of several SFRs
(`EPIC_IRQ_Enable/DisableSrc/ClearFlag/GetFlag` picking PIE1/PIE2/PIE3 or
PIR1/PIR2/PIR3 by `pir_index`; `EPIC_GPIO_Init` picking TRISx/LATx/ANSELx
by `port`) computes the target's 12-bit data-memory address in a local
(`_pa`, `ta`, `la`, `aa`) and then reads/writes through it. Disassembly
of the linked `example_blink` firmware (XC8 v3.10, `-O2`,
`-mcpu=16f1937`) shows every one of these compiles to:

```
movf    (addr_local),w
movwf   fsr1l
movf    (addr_local+1),w      ; upper byte, since the space is >256 bytes
movwf   fsr1h
movf    indf1,w               ; or: movwf indf1 for a write
```

i.e. **indirect addressing via FSR1/INDF1** (DS41364B §2.5). Reading
INDF1 in PIC16F193X returns the byte at the FSR1 16-bit data-memory
address, which on this core is *decoded as a linear-style address, NOT
BSR-relative* (DS41364B §2.5 "Indirect Addressing", with bit 7 of FSRnH
acting as an IRP-style high bit). Because FSR1 holds the full address
itself, no bank-select state is involved: so this is a true architectural
"BSR-independent" access.

**Confirmed clean for the read-only half (`EPIC_IRQ_GetFlag`,
`EPIC_IRQ_ClearFlag` for the `pir_index=1` / `pir_index=2` branches, the
PIR1-bank dispatch in `pic16f193x_irq.c`, and the port-register dispatch
in `pic16f193x_gpio.c`).** The PIC18 failure mode (compiled to the
program-memory table mechanism instead of a data access) does not apply
on this core.

Separately, a **literal, compile-time-constant SFR token in a
non-mirrored bank** (`option_clr_set()`'s access to `PIC_REG_OPTION`,
bank 1) compiles to the expected `movlb 1` / access / `movlb 0`
sequence, i.e. XC8 auto-banks literal SFR tokens on this core exactly as
`pic16f193x_sfr.h`'s header comment assumed. No manual
`pic_select_bank`-style macro is needed for a literal token, and none of
the foundation code uses one for that reason.

**Not yet exercised / now superseded**: the `pic16f193x_select_bank()`
macro (`pic16f193x_sfr.h`) is currently unused by any driver, since
every access above is either a literal auto-banked token or a runtime
address that lowers to FSR/INDF. For *read-modify-write* against a
runtime SFR address (the PIE1/2/3 case) this FSR/INDF shape turned out
to be the wrong approach. See Finding 2.

Method, for reproducing or extending this check: `xc8-cc -mdfp=<DFP>/xc8
-mcpu=16f1937 -O2 <all HAL .c + one app .c> -o out.elf` (a full link, not
a standalone `-c`/`-S` on one file) produces `out.s` alongside `out.elf`
as a side effect; grep it for the function under test's label
(`_EPIC_IRQ_Enable:` etc.) and read the instructions between it and the
next `global` line.

## Finding 2: PIC8_PIE_ENABLE_BIT FSR1:INDF1 route silently addresses the wrong byte for PIE1/2/3; replaced with `__at()`-pinned scratch + inline asm `movlb 1`/`iorwf PIE1,f`/`movlb 0`

**Status:** fixed; verified by both the disassembly comparison below and
the `mdb` register readback (§4 control-register check) of the Timer1
example now showing `PIE1=0x01` (TMR1IE bit 0 set) where the broken
build showed `PIE1=0x00`. The fix mirrors `pic16f87xa-hal`'s proven
`__at(0x70)`-pinned scratch + inline-asm bank-switch shape; the
Enhanced-Mid-range idiom is `movlb 1` instead of classic's
`bsf STATUS,5`.

### Why this had to be looked at twice

Finding 1 concluded the FSR1:INDF1 indirect pattern was safe because
"FSR1 holds the full 12-bit address itself, so no bank-select state can
be stale or wrong at the point of access." That was wrong for
*read-modify-write* against a runtime address: while reading via
`movf indf1,w` does return the byte at the FSR 16-bit address, the
matching *write* via `movwf indf1` writes the SAME FSR-addressed byte,
which is the *unmirrored* PIE1 byte at FSR=0x0091 (linear/GPR-style
addressing). Per DS41364B §2.5 "Indirect Addressing", the PIC16F193X
data-memory map addresses the FSR <-> byte mapping linearly for the
area between 0x2000-0x29AF (the linear GPR region) and via the banked
mirror for everything below 0x2000 *except where the banked mirror
overrides the linear map*. PIE1 lives at bank 1, offset 0x11, which
is reachable only via the banked mirror at FSR<0x2000 OR via direct
addressing with BSR=1. The `movwf indf1` route writes to byte 0x91 in
the linear/GPR-style region, which is a GPR byte, NOT PIE1. So
PIE1<TMR1IE>=0 was the runtime symptom even though the *read* half of
the broken RMW appears to succeed (it reads the same wrong byte, then
ORs in the mask, then writes the same wrong byte. The read returns
0x00 for an unimplemented GPR, so the OR with the mask sets the GPR
byte, but PIE1 in the bank 1 mirror stays at 0x00).

This is the §4-gate scenario the gate is designed to catch: code that
builds clean, links clean, and silently writes the wrong thing at
runtime.

### The fix

Mirror `pic16f87xa-hal/include/target/pic16f87xa_platform.h` line for
line, swapping `bsf STATUS,5` (RP0-bit bank select on classic PIC16)
for `movlb 1` (BSR-byte bank select on Enhanced Mid-range, single
instruction, DS41364B §3.4). Switch on `pir_index` to pick the right
PIE1/PIE2/PIE3 literal symbol (all three PIEs are in bank 1 per
DS41364B §3.4 Tables 2-4, verified in the DFP's `pic16f1937.inc`:
`PIE1 equ 0091h`, `PIE2 equ 0092h`, `PIE3 equ 0093h`, all consistent
with bank-1 offsets 0x11/0x12/0x13).

**Files changed**:

1. `pic16f193x-hal/include/target/pic16f193x_platform.h`: replace
   the plain-C `PIC8_PIE_ENABLE_BIT` / `PIC8_PIE_DISABLE_BIT` macros
   with the inline-asm shape. Add the
   `extern volatile uint8_t epic_irq_pie_scratch __at(0x70);`
   declaration. Update the file header to point at this finding for
   the failure mode.
2. `pic16f193x-hal/src/core/pic16f193x_isr_vector.c`: add the
   `volatile uint8_t epic_irq_pie_scratch __at(0x70);` definition
   that the platform header's `extern` needs, plus a header-comment
   update that removes the old "no bank-switch scratch bytes are
   pinned" assertion (it was the assumption that hid the bug).

The platform header's scratch byte is in PIC16F193X's
bank-independent common RAM at 0x70 (DS41364B Table 2-3,
`pic16f193x_isr_vector.c`'s `pic16_irq_pie_scratch` parallels the
classic `pic16_isr_vector.c`'s same-named byte at the same address).

### Disassembly evidence

**Before** (broken, plain-C RMW, `make xc8-build ...MCU=16F1937
HARNESS=sim` then `xxd build/16F1937-firmware-sim.s` near
`_EPIC_IRQ_Enable:`):

```
l2400:                          ; pir_index = 0 branch (PIE1)
    movlw   091h
    movwf   (_EPIC_IRQ_Enable$410)
    movlw   0
    movwf   ((_EPIC_IRQ_Enable$410))+1    ; _pa = 0x0091

l223:
    movf    (_EPIC_IRQ_Enable$410),w
    movwf   (??_EPIC_IRQ_Enable)
    clrf    (??_EPIC_IRQ_Enable+1)
    movf    (0+(??_EPIC_IRQ_Enable)),w
    movwf   fsr1l                   ; FSR1L = 0x91
    movf    (1+(??_EPIC_IRQ_Enable)),w
    movwf   fsr1h                   ; FSR1H = 0  <-  address = 0x0091

    movf    indf1,w                 ; read byte at FSR1 = 0x0091
    movwf   (EPIC_IRQ_Enable@_v)

l2404:
    movf    (EPIC_IRQ_Enable@enable_mask),w
    iorwf   (EPIC_IRQ_Enable@_v),f       ; v |= mask

l2406:
    movf    (EPIC_IRQ_Enable@_pa),w
    movwf   (??_EPIC_IRQ_Enable)
    clrf    (??_EPIC_IRQ_Enable+1)
    movf    (0+(??_EPIC_IRQ_Enable)),w
    movwf   fsr1l                   ; FSR1L = 0x91
    movf    (1+(??_EPIC_IRQ_Enable)),w
    movwf   fsr1h                   ; FSR1H = 0

    movf    (EPIC_IRQ_Enable@_v),w
    movwf   indf1                   ; write byte at FSR1 = 0x0091
```

This routes the read AND the write through FSR=0x0091, never setting
BSR. Per DS41364B Table 2-4, the byte mapped to FSR=0x0091 in the
linear/GPR region is not PIE1; PIE1 lives in bank 1 at offset 0x11
and is reachable via the banked mirror with BSR=1 (or via direct
addressing `movf 091h,w` / `movwf 091h` after `movlb 1`).

**After** (fixed, inline asm, same `pir_index = 0` path for PIE1):

```
l224:
# 98 "../../src/core/pic16f193x_irq.c"
    movf    _epic_irq_pie_scratch,w      ; W = mask
# 98 "../../src/core/pic16f193x_irq.c"
    movlb   1                              ; BSR = 1 (bank 1)
# 98 "../../src/core/pic16f193x_irq.c"
    iorwf   PIE1,f                         ; PIE1 |= mask
# 98 "../../src/core/pic16f193x_irq.c"
    movlb   0                              ; restore BSR
```

Three asm lines do what the C-level `_v = read8(); _v |= mask;
write8(_v)` tried to do. The read half is implicit in `iorwf PIE1,f`
(W is loaded with PIE1 OR'd against the scratch-mask value, then
written back). The `movlb 0` at the end restores bank 0 because the
*callers* of `EPIC_IRQ_Enable` (e.g. `EPIC_TIMER1_Init`) expect bank 0
to be active on return. The same defensive-restoring shape as
`pic16f87xa-hal`'s `bcf STATUS,5` exit.

The exact form for `pir_index=1` and `pir_index=2` (PIE2 and PIE3)
is the same shape with `iorwf PIE2,f` / `iorwf PIE3,f`. Both
register symbols also live in bank 1, so the `movlb 1` is correct
for all three.

### Runtime evidence: §4 control-register check

`mdb` register readback against `HARNESS=sim` builds:

**Before** (Timer1 example on top of broken macro):
```
TRISB  = 254 (0xFE)        ; GPIO path correct
T1CON  = 49  (0x31)         ; EPIC_TIMER1_Start ran (manual movlb)
PIE1   = 0                  ; <- BROKEN: TMR1IE bit 0 should be set
PIR1   = 0
INTCON = 194 (0xC2)         ; GIE=1, PEIE=1
LATA   = 0                  ; harness never drove RA0 high
LATB   = 1                  ; at least one Timer0 ISR ran in last wait
```

**After** (same example on top of the fixed macro):
```
TRISB  = 254 (0xFE)        ; unchanged (no GPIO code change)
T1CON  = 49  (0x31)         ; unchanged (no Timer1 code change)
PIE1   = 1                  ; <- FIXED: TMR1IE bit 0 now set
PIR1   = 0                  ; ISR cleared TMR1IF after handling
INTCON = 194 (0xC2)         ; unchanged
LATA   = 1                  ; <- FIXED: PIC8_HARNESS_RESULT: PASS fired
LATB   = 0                  ; even toggle count returns RB0 to 0
PORTA  = 1                  ; PASS marker visible from mdb
```

The PORTA bit 0 transition (0 -> 1) is what makes the
`make mdb-test ... MODE=gpio` recipe finally return success (`PASS
marker found (PORTA bit 0 set, byte=1)`). The brief's default
`WAIT_MS=2000` is too short on the Docker MPLAB SIM (~1/2000th
real-time, see Phase 0 notes); passing `WAIT_MS=60000` to
`make mdb-test` produces PASS in the same session.

### Why the same shape as pic16f87xa-hal

`pic16f87xa-hal/include/target/pic16f87xa_platform.h` carries the same
fix shape because the same codegen trap existed on the classic PIC16
core (Finding 1 of that family's `docs/ARCHITECTURE.md`). The
Enhanced Mid-range port uses the same `__at(0x70)` scratch + asm
pattern, just with `movlb 1` (BSR-byte select) instead of
`bsf STATUS,5` (RP0 bit select). pic18_irq.c uses an equivalent
`switch`-based per-bank dispatch via case-by-case inline writes; same
goal, different mechanism appropriate to the family's pure-C
compilation model.

## Finding 3: read-only status/flag bits reading back set even though the driver never wrote them, mistaken for a codegen bug across several peripherals

**Status:** not a bug, a repeated false alarm worth documenting so it
stops costing debugging time. Confirmed across four independent
peripherals during their §4 gates: EUSART's `BAUDCON<RCIDL>` (bit 6,
hardware sets it whenever the receiver is idle, so a driver that
writes `BAUDCON = 0x00` reads back `0x40`), both comparators'
`CxCON0<CxOUT>` (bit 6, the live comparator output, always driven by
hardware regardless of what the driver wrote to the rest of the
register), FVR's `FVRCON<FVRRDY>` (bit 6, hardware sets it once the
reference has stabilized), and CPS's `CPSCON0<CPSOUT>` (bit 1, the raw
oscillator output). In every case the register's writable control bits
landed exactly as written; only the datasheet-documented read-only
status bit differed from the written value.

The fix in each case was the same: check the datasheet's register
table for which bits are read-only status/flag bits before treating a
readback mismatch as a bug, and mask those bits out of the comparison
(or assert the POR-then-hardware-set value for them specifically, not
the value the driver wrote). This is now `docs/adding-a-device.md` §4
step 8's standing instruction, not something to re-derive per
peripheral.

## Open, for whoever picks this back up

The real *register-readback* half of the §4 gate has now run for at
least one peripheral routed through every PIE/PIR bank: PIE1 (Timer1,
Timer2, CCP1/2, ADC), PIE2 (comparators C1/C2), and PIE3 (Timer4/6,
CCP3/4/5), closing the verification gap this section used to flag for
PIE2 and PIE3. All 13 peripherals in `docs/pic16f193x-plan.md` §7's
roadmap have landed and cleared the §4 gate; there is no remaining
open peripheral work for this family as of this note. Future work here
is either a new device variant (Path A, `docs/adding-a-device.md` §3)
or wiring the family-agnostic `epic-*` modules (taskmgr, tick, serial,
...) against it for the first time, per the main `README.md`'s status
table.
