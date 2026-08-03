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
image and dumping its `.s` does). `mdb` (MPLAB SIM, headless) is still
not installed, so no peripheral has been through the real-register
readback half of the §4 gate yet; this finding covers the codegen
inspection half only.

## Finding 1: runtime SFR addresses route through FSR1:INDF1, not BSR; literal tokens auto-bank correctly

**Probed, both patterns clean, no misdirection found.**

Every function that dispatches a runtime value to one of several SFRs
(`HAL_IRQ_Enable/DisableSrc/ClearFlag/GetFlag` picking PIE1/PIE2/PIE3 or
PIR1/PIR2/PIR3 by `pir_index`; `HAL_GPIO_Init` picking TRISx/LATx/ANSELx
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

i.e. **indirect addressing via FSR1/INDF1** (DS41364B §2.5), which is
architecturally BSR-independent by design: FSR1 holds the full 12-bit
address itself, so no bank-select state can be stale or wrong at the
point of access. This is a different (and safer) resolution than either
prior family's bug shape: it is not the classic-PIC16 failure (bank bits
+ direct addressing, where a stale/misordered bank switch corrupts the
access) and not the PIC18 failure (compiled to the program-memory table
mechanism instead of a data access). Confirmed clean for both the
PIE-bank dispatch in `pic16f193x_irq.c` and the port-register dispatch
in `pic16f193x_gpio.c`.

Separately, a **literal, compile-time-constant SFR token in a
non-mirrored bank** (`option_clr_set()`'s access to `PIC_REG_OPTION`,
bank 1) compiles to the expected `movlb 1` / access / `movlb 0`
sequence, i.e. XC8 auto-banks literal SFR tokens on this core exactly as
`pic16f193x_sfr.h`'s header comment assumed. No manual
`pic_select_bank`-style macro is needed for a literal token, and none of
the foundation code uses one for that reason.

**Not yet exercised**: the `pic16f193x_select_bank()` macro
(`pic16f193x_sfr.h`) is currently unused by any driver, since every
access above is either a literal auto-banked token or a runtime address
that lower to FSR/INDF (which never needs BSR set explicitly). Given
this finding, that is the *recommended* shape going forward: prefer
literal tokens (auto-banked) or FSR/INDF-routed runtime dispatch over a
manual `movlb` + raw address write, since the latter is the pattern that
broke classic PIC16. If a future peripheral needs
`pic16f193x_select_bank` directly (e.g. bulk indirect access into linear
data memory), re-run this same disassembly check on that specific call
site before trusting it, don't assume this finding covers it.

Method, for reproducing or extending this check: `xc8-cc -mdfp=<DFP>/xc8
-mcpu=16f1937 -O2 <all HAL .c + one app .c> -o out.elf` (a full link, not
a standalone `-c`/`-S` on one file) produces `out.s` alongside `out.elf`
as a side effect; grep it for the function under test's label
(`_HAL_IRQ_Enable:` etc.) and read the instructions between it and the
next `global` line.

## Open, for whoever picks this back up

The real *register-readback* half of the §4 gate (an actual `mdb`
session confirming, e.g., that enabling `PIC16F193X_IRQ_TMR2` sets bit 1
of PIE1 at address 0x91 and nothing else) has not run yet; `mdb` is not
installed. Finding 1 above only clears the static-codegen half of the
verification. Per `docs/adding-a-device.md`, that is necessary but not
sufficient, run the `mdb` gate before marking any peripheral done, even
though the codegen inspection came back clean.
