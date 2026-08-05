# `pic8-vga`: PIC16F877A as a VGA-connector video generator for a 6502 — design notes

Status: **exploratory design — feasibility and the riskiest mechanism
validated; no module scaffolded, no implementation started.** This is a
record of a design conversation, not an approved plan like
`epic-debounce-plan.md`/`multi-family-plan.md`, read it as "here's what we
know before writing code," and pick up from the pending list below.

## The idea

Use a PIC16F877A as a dumb video co-processor for a 6502: the 6502 writes
cell/pixel data into the PIC over its hardware Parallel Slave Port (PSP),
and the PIC bit-bangs HSYNC/VSYNC + analog RGB out to a standard VGA
connector. Low resolution only (a chunky block/character grid, not a real
640x480 framebuffer) — the PIC's 5 MIPS and 368 bytes of RAM rule out a real
framebuffer at any standard VGA resolution.

## Solved / validated

Validated against the
[DS39582B](https://ww1.microchip.com/downloads/en/DeviceDoc/39582b.pdf)
datasheet (not vendored in this repo — Microchip vendor documentation,
linked instead), this repo's existing `pic16f87xa_psp.h`/`.c` driver and
`epic-math`'s documented XC8 inline-asm conventions
(`epic-math/docs/ARCHITECTURE.md`), a live web search, and hands-on probes
compiled with the pinned XC8 v3.10 toolchain (`xc8-cc -mcpu=16f877a`,
scratch-only, nothing below is in the repo tree):

- **Instruction timing.** All mid-range instructions are 1 cycle (= 4 Fosc,
  200ns at 20MHz) except program-counter-changing instructions (GOTO, CALL,
  RETURN, RETFIE, RETLW) and *taken* skips (BTFSC/BTFSS/DECFSZ/INCFSZ),
  which are 2 cycles. `MOVF`/`MOVWF`/`INCF` are each 1 cycle. DS39582B
  Table 15-2.
- **VESA 640x480@60Hz timing**, used as the sync-rate target even though
  active resolution will be much chunkier than 640x480: 25.175MHz dot
  clock, horizontal total 800px (640 active + 16 front porch + 96 sync + 48
  back porch) = 31.469kHz HSYNC, vertical total 525 lines (480 active + 10
  front porch + 2 sync + 33 back porch) = 59.94Hz VSYNC. Modern LCD-with-VGA
  monitors can reject sync rates off by as little as ~1kHz ("format not
  recognized"), so the design targets the standard rate exactly rather than
  assuming slack.
- **Per-line instruction budget** at 20MHz/5MIPS: active video 25.42µs =
  127 cycles, horizontal blank 6.36µs = 32 cycles, vertical blank (45
  lines) 1.43ms = ~7150 cycles.
- **Scanout inner loop**, confirmed compiled byte-for-byte with zero added
  overhead: an unrolled `movf INDF,w` / `movwf PORTD` / `incf FSR,f` per
  column costs exactly 3 cycles/column. 32 columns = 96 cycles, leaving 31
  cycles (6.2µs) of headroom inside the 127-cycle active window for jitter
  tolerance. This means **RAM, not active-line timing, is the binding
  constraint** on grid size (368 bytes total, ~240 available for a
  framebuffer after other state) — the opposite of an earlier, more
  conservative estimate in this same conversation.
- **PSP (Parallel Slave Port) handshake**, confirmed against the datasheet
  and this repo's driver: an external bus master (the 6502) writes a byte
  and PORTE's CS/WR/RD lines latch it into a hardware input buffer; `IBF`
  sets only once CS and WR are both seen high again, `IBOV` latches on a
  second write before the first is drained, and the firmware clears `IBF`
  simply by reading `PORTD`. The write side never stalls waiting on the PIC
  core — confirms the "6502 writes, PIC drains later during vblank" model.
- **`FSR`/`INDF` indirect addressing and bank selection** — this was the
  one open risk (the repo's own `epic-math` had needed its own empirical
  XC8 probe for a *different* bank-selection case, direct addressing via
  `banksel`, which doesn't apply to our indirect case). Resolved by
  compiling real probes:
  - `STATUS,7` is `IRP`, confirmed both from the datasheet and from XC8's
    own emitted runtime-init code (`bcf status,7 ;select IRP bank0`).
  - No `bankisel` pseudo-op exists or is needed; plain `bcf STATUS,7` /
    `bsf STATUS,7` is the correct, working idiom for indirect bank select.
  - The XC8 v3.10 linker **scatters** file-scope variables across banks by
    best fit, not declaration order (confirmed: a buffer declared after 160
    bytes of filler still landed in bank0) — relying on default placement
    and inspecting the `.map` after the fact would be fragile across
    recompiles.
  - `__at(addr)` reliably pins a variable to an exact address (confirmed:
    `static volatile uint8_t rowbuf[32] __at(0x120);` linked to
    `_rowbuf (abs) 0120` exactly). This is the mechanism to use: pin
    framebuffer/row-buffers at addresses we choose, so the IRP value for
    each is a hardcoded compile-time constant, never something to detect
    at runtime or re-derive from a `.map`.
  - Confirms `epic-math`'s underscore rule the hard way: `asm("movf
    fsr_lo,w")` (no underscore) compiles clean at `-S` but fails to *link*
    (`undefined symbol "fsr_lo"`) — PIC16 user globals need `_name` in the
    asm string; SFRs (`FSR`, `INDF`, `PORTD`, `STATUS`) are unprefixed.

## Pending — not yet done

Nothing below has been implemented, host-tested, or run on real silicon.

- **Grid size decision.** Sketched 20x12 and 32x7ish examples during design
  but nothing is fixed. Needs an actual choice of columns x rows x
  bytes/cell against the ~240-byte budget, informed by what's actually
  useful for a 6502 host to draw.
- **Module scaffold.** No `pic8-vga/` directory exists yet. Following this
  repo's convention (`epic-lcd`, `epic-sdcard`): README, ARCHITECTURE.md,
  API.md, host-sim + hardware backend split, `mcu/pic16f87xa-vga-mplabx/`
  Makefile.
- **Framebuffer / PSP-drain C API + host tests.** `epic_vga_init`,
  `epic_vga_set_cell`, `epic_vga_service_psp` (or equivalent), with an
  injectable-ops seam like `epic-bus`'s mock MEM device, and host tests
  covering cell writes, PSP byte parsing/overflow, and buffer bounds —
  none of this exists yet.
- **The actual scanout routine and frame super-loop.** Only the 32-column
  inner-loop primitive and the bank-pinning mechanism are validated in
  throwaway probes (not in the repo). Still needed: the full per-line
  routine (HSYNC pulse + back porch + `scanout_row()` + front porch), the
  480+45-line frame loop, VSYNC generation, and the "only service PSP
  during vblank, interrupts/PSP masked during active video" discipline
  discussed but not written.
- **Hardware bring-up validation.** This entire design is reasoned from
  datasheet timing and compiled-instruction inspection — nothing has run on
  a real PIC16F877A or been checked against an actual monitor/scope. Host
  simulation can validate the C-level logic (cell writes, PSP parsing) but
  *cannot* validate real-world scanout timing; that needs real silicon.
- **Color/DAC wiring.** Discussed (R/2R or a few resistor-divided GPIO
  lines into the VGA analog inputs) but not designed in any detail.

## Reference

- Datasheet: [DS39582B](https://ww1.microchip.com/downloads/en/DeviceDoc/39582b.pdf) (not vendored in this repo).
- PSP driver: `pic16f87xa-hal/include/peripherals/pic16f87xa_psp.h` (+ `.c`).
- XC8 inline-asm conventions: `epic-math/docs/ARCHITECTURE.md`, "Inline-asm
  binding, the XC8 round-trip probe".
- VGA timing: VESA DMT 1.13; http://www.tinyvga.com/vga-timing.
