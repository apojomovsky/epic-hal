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

No findings yet: the real-target build and the `mdb` gate are pending
the `Microchip.PIC12-16F1xxx_DFP` (see the plan doc and the mcu README).
The foundation is host-sim verified only. The first action once the DFP
lands is the codegen probe below, before any peripheral is built on top
of the platform layer.

## Finding 0 (planned): the BSR / runtime-SFR-address codegen probe

Before trusting the platform layer on this core, probe these two
patterns under XC8 and inspect the generated `.s`/`.map`, per
`docs/adding-a-device.md` §4.8 and the appendix's known-risky patterns:

1. An SFR access made while a BSR bank switch is in effect, where the
   address or an intermediate value is a C local/parameter (the
   classic-PIC16 shape, `pic16f87xa-hal/docs/ARCHITECTURE.md` Findings
   1/2/9). On the 193X XC8 auto-banks literal SFR access, so the question
   is whether a manual `pic16f193x_select_bank` interleaved with a C
   local misdirects the same way.
2. An SFR address that is a runtime variable/struct-field/parameter at
   the point of access (the PIC18 shape, `pic18fxx5x-hal/docs/ARCHITECTURE.md`
   Findings 3/4), compiled to a data-memory access and not to the
   program-memory table-read mechanism.

If either misdirects, the platform header's plain-C PIE RMW form is
replaced with an inline-asm banking sequence (MOVLB-based, the
Enhanced Mid-range equivalent of the classic `bsf STATUS,5` path) and
the finding is written up here with the XC8 User's Guide section
cited. If neither misdirects, that is recorded here too, so the
plain-C form is trusted on evidence rather than assumption.

## Open, for whoever picks this back up

The probe runs the moment the `PIC12-16F1xxx_DFP` is installed. Until
then every SFR access in the foundation stays a compile-time-constant
`PIC_REG_*` token and runtime dispatch branches before touching any
SFR, the same proven pattern from `pic18_irq.c` / `pic18fxx5x_ccp.c`,
which is correct on both existing families regardless of the 193X
result.
