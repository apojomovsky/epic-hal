# `pic8-usb`: USB CDC-ACM device stack for the PIC18F2455/2550/4455/4550 family — implementation plan

Status: **Phase 1/2 done** (M-Stack vendored, wrapper + host-stub + unit
tests, see "Phase 2 findings" below and `pic8-usb/docs/ARCHITECTURE.md` for
the as-built version), **Phase 3 (real-silicon bring-up,
`mcu/pic18fxx5x-usb-mplabx/`) not started**, matching `pic8-usb/README.md`'s
own status. Originally written for a fresh implementing agent with no other
context on this conversation. Chip scope and device class were decided in
the design discussion this plan closes out: PIC18F2455/2550/4455/4550 only
(the exact family `pic18fxx5x-hal` already targets), CDC-ACM only (virtual
serial port, no HID/vendor-class work in this plan).

## What this is, and why it breaks the pattern every other `pic8-*` module set

Every other module in this repo (`pic8-fsm`, `pic8-debounce`, `pic8-adcfilter`,
`pic8-math`) is either genuinely vendor-agnostic C or, at worst, a
per-family backend split with a portable-C reference. `pic8-usb` is neither:
it is a thin firmware-facing wrapper around a **vendored third-party USB
device stack**, because a USB 2.0-compliant enumeration/control-transfer
state machine is not something to hand-roll for a plan like this, and
because the PIC18F4550's SIE (Serial Interface Engine) peripheral — the
thing that actually does USB — has no equivalent on any other family this
repo supports. `pic16f87xa-hal` parts have no USB peripheral at all. So,
unlike `fsm.c` or `debounce.c`, there is no meaningful "host simulation
proves the shipped code" story here (see "Host build story" below), and
there will never be a `pic16f87xa` backend — this module is PIC18Fxx5x-only
by hardware necessity, not by narrow initial scope.

The payoff for accepting a third-party dependency: a hand-rolled USB stack
is a multi-month effort with a large defect surface (chapter 9 enumeration
alone has a lot of host-compatibility edge cases). Wrapping a proven,
already-PIC18F4550-targeted stack gets a working `/dev/ttyACM0` in a
fraction of that time, with the wrapper layer being the only genuinely new
code.

## Third-party dependency: M-Stack

[M-Stack](https://github.com/signal11/m-stack) (Alan Ott) is chosen over
Microchip's own USB Framework/Harmony because it is a plain public git repo
(clone and go, no click-through EULA gate) under a permissive-enough
license. **Correction from this plan's first draft, found during Phase 1
(vendored and actually read, commit `979665546f3f39fb4b28a4acedfb1f9a5c55782e`,
2017-09-28):** the license is **dual LGPLv3 / Apache-2.0**, not
Apache-2.0/GPLv2 as first assumed — confirmed from `README.txt`'s license
section. Apache-2.0 is still the right arm to build against (compatible
with this repo's MIT license, no copyleft complications).

**Bigger correction: there is no ready-made PIC18F2455/2550/4455/4550 chip
profile in current mainline M-Stack.** `usb/src/usb_hal.h` gates PIC18
support to `_18F46J50` only, with an explicit `#error "CPU not supported
yet"` for every other PIC18 part. This needs a real (if narrow) port, not
just a `#define` swap. The situation is better than "start from nothing,"
though — three things found in the vendored source make the port low-risk:

1. **The project's own origin was a PIC18F4550 implementation.** Per the
   first commit's message (`39d2441`, April 2013): *"In 2008, I made an
   implementation of basic USB stack for PIC18F4550 as a learning
   exercise... This is the code that worked on the PIC18F4550 in 2008."*
   M-Stack was generalized from 4550-specific code, then later ported to
   XC8 and extended to PIC18F46J50 (commit `b78835f`) — at which point the
   4550-specific memory addresses were dropped and the `#error` guard was
   added (`f87b9e8`), but the *generic* PIC18 SFR-level macros
   (`SFR_EP_MGMT`, `SFR_USB_INTERRUPT_FLAGS`, `UCFGbits.*`, `PIR2bits.USBIF`,
   etc., all under the `#ifdef _PIC18` block in `usb_hal.h`) still apply to
   the 4550 unchanged — those register names are the same across
   Microchip's PIC18 USB parts, which `README.txt` itself notes ("Microchip
   has ... made ... the register-level interfaces to their USB peripherals
   as similar as possible across MCUs").
2. **The only chip-specific unknown is `BD_ADDR`** (where the Buffer
   Descriptor Table and endpoint buffers sit in the linear memory map) —
   and cross-referencing the PIC18F2455/2550/4455/4550 datasheet
   ([DS39632E](https://ww1.microchip.com/downloads/en/DeviceDoc/39632e.pdf),
   already the register source for `pic18fxx5x-hal`), §17.3 "USB RAM": Bank 4
   (`400h`–`4FFh`) holds the BDT, banks 4–7 (`400h`–`7FFh`) are USB RAM —
   **the identical layout already coded for the J50's `BD_ADDR 0x400`.**
   The port is plausibly a one-line addition:
   `#elif defined(_18F4550) || defined(_18F4455) || defined(_18F2550) || defined(_18F2455)`
   reusing `BD_ADDR 0x400`, though this is a plan-stage hypothesis from
   datasheet cross-reference, not yet compiled or tested — confirm XC8's
   actual built-in device macro name (expected `_18F4550`, matching the
   `_18F46J50` naming pattern) and validate against real silicon in Phase 3,
   don't trust it from datasheet-reading alone.
3. **XC8 is already a first-class supported compiler** (`README.txt`:
   "Supported Software: Microchip XC8 compiler... Note that the C18
   compiler is not currently supported"), confirmed throughout
   `usb/src/*.c` and `usb/include/*.h` via `#elif __XC8` branches
   alongside the legacy `__C18` ones. No compiler-compatibility risk —
   this lines up with the toolchain already in use
   (`/opt/microchip/xc8/v3.10/bin`).

This changes Phase 1/2 scope from this plan's first draft: budget real time
for `usb_hal.h` porting work (writing the new `#elif` branch, then proving
it on real hardware in Phase 3), not just "read the existing profile and
wrap it."

**Bonus find, relevant to the earlier host-tooling discussion:** M-Stack's
own `host_test/` directory ships small **libusb**-based host-side test
programs (`control_transfer_in.c`, `control_transfer_out.c`, `feature.c`,
...) — concrete confirmation that libusb's role in this ecosystem is
exactly host-side control-transfer testing, as discussed earlier, and a
directly reusable starting point for this module's own bring-up testing
before the CDC layer is stable (raw control transfers are easier to debug
than a full CDC-ACM enumeration).

**Vendoring — done.** M-Stack is vendored into `pic8-usb/third_party/m-stack/`
(commit `979665546f3f39fb4b28a4acedfb1f9a5c55782e`, pinned via
`third_party/m-stack/VENDORED_COMMIT`, plain copy — no submodule, matching
this repo's existing PDF-vendoring convention). Licensed under the
Apache-2.0 arm; M-Stack's own `LICENSE-*.txt` files are left untouched
inside that directory, unmodified/not relicensed. Still needed before this
is committed: a root `README.md` license-section note mirroring the
existing PDF note ("this directory vendors M-Stack under Apache-2.0; remove
it from your fork if you don't want to redistribute third-party code").
Do not copy M-Stack files into `pic8-usb/src/` — `pic8_usb.c` calls into
`third_party/m-stack/` headers, it does not absorb them.

**Confirmed API surface** (read directly from `apps/cdc_acm/main.c`,
`usb/include/usb_cdc.h`, `usb/src/usb.c` — supersedes this plan's original
guessed signatures):

- `usb_init()`, `usb_service()` — matched the original guess exactly.
- **No built-in ring buffer or `read()`/`write()`.** The real API is raw
  endpoint-buffer access: `usb_get_in_buffer(ep)`, `usb_in_endpoint_busy(ep)`,
  `usb_send_in_buffer(ep, len)`, `usb_out_endpoint_has_data(ep)`,
  `usb_get_out_buffer(ep, &ptr)`, `usb_arm_out_endpoint(ep)`,
  `usb_in_endpoint_halted(ep)`. This *confirms* rather than changes this
  plan's design: `pic8_usb.c` genuinely has to own the ring buffering on
  top of these primitives, exactly as sketched in "Public API" below — the
  example app builds its own ad hoc buffering in `main.c` for lack of
  anything better.
- **`pic8_usb_connected()` (DTR) must be backed by
  `CDC_SET_CONTROL_LINE_STATE_CALLBACK`**, not by `usb_is_configured()` as
  a first instinct might suggest — `usb_is_configured()` only means
  enumeration finished, which happens whether or not any application on the
  host has actually opened the port. `usb_cdc.h`'s
  `CDC_SET_CONTROL_LINE_STATE_CALLBACK(interface, dtr, rts)` is the real
  "a terminal opened the port" signal (fires on `SET_CONTROL_LINE_STATE`,
  `wValue & 0x1` = DTR state). `pic8_usb.c` should track this in a static
  bool via that callback, not derive "connected" from configuration state.

## VID/PID: use [pid.codes](https://pid.codes)

USB-IF's own VID costs roughly $6000 — not appropriate for a hobby module.
pid.codes donates PID ranges under a shared VID (`0x1209`) free of charge to
open-source hardware/firmware projects; this repo already qualifies (MIT
license, public). Requesting a PID is a pull request against
[pid.codes/pids](https://github.com/pidcodes/pidcodes.github.com) naming the
project and product — do that as part of Phase 4 below, once the descriptor
set is stable enough to name (revising it after allocation is fine; the
registration is against the project, not a frozen descriptor).

Until that PID lands: M-Stack's own example descriptors ship a placeholder
VID/PID pair that its own docs mark development-only, not for
redistribution. Fine for bring-up and enumeration testing on a bench;
**swap to the pid.codes PID before this ships to anyone else, and say so
loudly in a `TODO`/README note so it isn't forgotten.**

## Public API — mirrors `pic8-serial` deliberately

Anyone who already knows `pic8_serial_*` should be able to pick this up
with near-zero new mental model — same shape, same TX-demand-driven/
RX-always-on ring-buffer discipline, just filled from the CDC endpoint
instead of a UART ISR:

```c
/* pic8-usb/include/pic8_usb.h */
#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>

pic8_status_t pic8_usb_init(void);

/* Must be called frequently (every main-loop iteration, or from a
 * pic8-taskmgr task with a short period — see "Servicing cadence" below).
 * Pumps M-Stack's device-task state machine; drives enumeration and
 * endpoint I/O. Nothing else in this API does anything useful before
 * pic8_usb_connected() is true. */
void pic8_usb_service(void);

/* Same contract as pic8_serial_write/read/available/flush: */
size_t pic8_usb_write(const uint8_t *data, size_t len);
size_t pic8_usb_read(uint8_t *buf, size_t max);
size_t pic8_usb_available(void);
void   pic8_usb_flush(void);

/* No UART equivalent: true once the host has the CDC port open (DTR
 * asserted), not merely once enumeration finished. A host can enumerate
 * the device and never open a terminal; callers that gate output on "is
 * anyone listening" need this, not just pic8_usb_init() having returned. */
bool pic8_usb_connected(void);
```

**Deliberately not building now:** no generic USB-class abstraction, no HID
or vendor-class entry points, no runtime class selection. CDC is the only
class this plan implements; if a HID or vendor-class need shows up later it
gets its own plan and its own module-internal decision about how much (if
any) of this API it can share, not a speculative abstraction layer bolted
on here on the chance it's needed. Same reasoning `pic8-debounce`'s plan
used to defer interrupt-driven debounce: build what's needed now, not what
might be needed.

## Servicing cadence

M-Stack's device-task pump needs to run often enough to service control
transfers and keep endpoints drained — on the order of low-single-digit
milliseconds, not tied to any fixed rate. Two ways to drive
`pic8_usb_service()`, both valid, caller's choice:

- **Bare main loop**: call it every iteration alongside whatever else runs
  there. Simplest, matches how `USBDeviceTasks()` is conventionally driven
  in vendor example code.
- **`pic8-taskmgr` task**: `task_spawn()` it with a short period (a couple
  of ticks). Fits naturally if the firmware is already structured around
  the cooperative scheduler, the same way `pic8-fsm`'s
  `example_taskmgr_integration.c` drives an `fsm_t` from a task callback.

Don't build a third option (e.g. wiring it to a USB-specific interrupt) in
this plan — M-Stack's own examples poll, and there's no established need
here to diverge from that.

## Host build story — where it genuinely stops

`pic8-serial`'s host build works because a UART loopback is a faithful
simulation of a UART. **There is no equivalent faithful simulation of a USB
SIE enumerating against a real host's driver stack.** Be honest about the
boundary instead of implying otherwise:

- The ring-buffer and connection-state bookkeeping inside `pic8_usb.c`
  (the part this module actually authors, as opposed to M-Stack's
  enumeration engine) gets a host-stub backend and real unit tests —
  same pattern as `pic8-tick`'s simulated Timer2: exercises the logic this
  module owns, not the SIE itself.
- Whether it actually enumerates, survives a real host's driver
  re-attach/suspend/resume cycle, and shows up as `/dev/ttyACM0` is
  **real-silicon-only, always**, and no amount of host testing substitutes
  for plugging a board into a Linux/Windows/macOS box and checking
  `dmesg`/Device Manager. Say this explicitly in `docs/ARCHITECTURE.md`
  when it's written, so nobody mistakes a green host test suite for proof
  of USB compliance.

## Repo layout

```
pic8-usb/
  CMakeLists.txt                     # host static lib (stub backend) + unit tests
  include/
    pic8_usb.h
  src/
    pic8_usb.c                       # ring buffers + pic8_usb_* wrapper, real usb_hal target only
    pic8_usb_host_stub.c             # host build: fakes connected()/service() enough to unit-test buffers
  third_party/
    m-stack/                         # vendored, Apache-2.0 arm of its dual license, untouched
  examples/
    example_usb_echo.c               # pic8_usb_read() -> pic8_usb_write() loopback, the smoke test
  tests/
    test_pic8_usb.c                  # ring buffer fill/drain, connected() state transitions, against the stub
  mcu/
    pic18fxx5x-usb-mplabx/Makefile   # XC8 Makefile, real silicon only — no PIC16 mcu/ dir, see above
  docs/
    ARCHITECTURE.md                  # the SIE-can't-be-host-simulated boundary goes here, prominently
    API.md
  README.md
```

No `pic16f87xa-usb-mplabx/` — do not create one. That family has no USB
peripheral; there is nothing to cross-compile-check there.

## Milestones

- **Phase 1 — vendor M-Stack, confirm its actual API surface. DONE.** Clone
  into `third_party/m-stack/`, read the CDC class driver and the
  2455/2550/4455/4550 chip profile, note any place this plan's assumed API
  shape (above) needs correcting against what M-Stack really exposes.
- **Phase 2 — wrapper + host-stub + unit tests. DONE**, with a bonus:
  since this environment happens to have MPLAB XC8 v3.10 actually
  installed (`/opt/microchip/xc8/v3.10/bin/xc8-cc`), the real-target build
  was not just written but **actually compiled and linked** against real
  PIC18F4550 device headers, producing a genuine `.hex` — stronger
  validation than this phase's original scope asked for. See "Phase 2
  findings" below for what that turned up. `pic8_usb.h`/`.c`, the
  host-stub backend, `tests/test_pic8_usb.c` (25 checks, all passing via
  `ctest`), `examples/example_usb_echo.c`.
- **Phase 3 — real-silicon bring-up.** `mcu/pic18fxx5x-usb-mplabx/Makefile`,
  descriptor set (VID/PID placeholder from M-Stack's examples, manufacturer/
  product strings), `example_usb_echo.c`. Flash a real PIC18F4550 board,
  confirm USB D+/D− wiring and crystal/PLL config bits are correct, verify
  enumeration in `dmesg` and a working echo over the resulting TTY.
- **Phase 4 — PID + docs.** File the pid.codes PR for a real PID, swap it
  in once allocated. `docs/ARCHITECTURE.md` (this plan's reasoning,
  written up properly, with the host-sim boundary stated prominently),
  `docs/API.md`, `pic8-usb/README.md`, a root `README.md` component-table
  row.

## Phase 2 findings

**The `usb_hal.h` port (Phase 1's central hypothesis) compiled and linked
clean on the first real attempt.** The one-line `BD_ADDR` widening
predicted from the datasheet cross-reference was correct as-is — no
further chip-specific fix was needed beyond it.

**Three genuine M-Stack-vs-XC8-v3.10 compatibility bugs turned up while
getting `usb.c`/`usb_cdc.c` to actually compile — all pre-existing in
upstream, all affecting every PIC18 target (the already-"supported" J50
included, not just this port), none specific to the 4550.** Fixed in the
vendored copy with inline comments explaining each (not sent upstream —
this is a vendored copy, not a maintained fork):

1. `BD_ATTR_TAG`/`XC8_BUFFER_ADDR_TAG` used `@##BD_ADDR` — the legacy
   PIC18 pic-cc placement syntax, token-pasted. XC8 v3.10's Clang-based
   PIC18 frontend rejects `##` forming `@0x400` as an invalid
   preprocessing token, **and** rejects the unpasted `@ 0x400` syntax
   outright (confirmed with a minimal repro, independent of M-Stack).
   Fixed to `__at(BD_ADDR)` — confirmed as this compiler's actual syntax
   by grepping its own headers (e.g. `proc/pic16f59.h`'s
   `... __at(0x000);`) and a standalone repro that compiled clean.
2. `usb_cdc.c` called a lowercase `min()` at four call sites
   (`CDC_GET_ENCAPSULATED_RESPONSE_CALLBACK` and three
   `SET_LINE_CODING`/`GET_LINE_CODING`-adjacent spots) despite defining an
   uppercase `MIN()` macro right above them and never defining or including
   a lowercase `min`. Old lenient compilers apparently let an implicit
   `int min()` declaration through silently; XC8 v3.10 (strict C99/Clang)
   correctly rejects it as an undeclared-function error. Fixed: `min(` →
   `MIN(` at all four sites — a one-word typo fix, not a redesign.
3. `struct cdc_serial_state_notification`'s bitfields (`bRxCarrier`,
   `bTxCarrier`, ...) were typed `uint16_t : 1`. XC8 v3.10 rejects sized
   stdint types as a bit-field base type outright ("bad bitfield type") —
   only `int`/`unsigned int` is accepted, which is what strict ISO C
   actually guarantees as a bit-field base type anyway (sized types being
   usable is implementation-defined, and apparently unsupported here).
   Fixed: `uint16_t` → `unsigned int` for each bit-field member; the union
   still totals 16 bits against its `uint16_t serial_state` sibling since
   `int` is 16 bits on PIC18/XC8.

**Real memory footprint, PIC18F4550, this wrapper + M-Stack's core +
CDC class driver + a trivial echo `main()`:** 8500 bytes flash (25.9% of
32 KiB), 454 bytes RAM (22.2% of 2 KiB) — reported by `xc8-cc` itself, not
estimated. Comfortable headroom against the RAM-budget risk flagged below;
worth re-checking once real application logic is added on top, but the
wrapper + stack overhead alone is not the tight part.

**Design decisions made while implementing, deviating from this plan's
earlier draft:**

- `pic8_usb_init()` returns `void`, not `pic8_status_t` as first drafted.
  `usb_init()` itself has no failure path to report, so a status return
  would have been fabricated always-success boilerplate — matches
  `pic8_fsm_init()`'s precedent of not reaching for `pic8-common`'s status
  enum when nothing can actually fail.
- M-Stack's required `app_*`-named callbacks are implemented as
  `pic8_usb_*_cb` in `pic8_usb.c` (mapped via macros in this module's own
  `src/usb_config.h`), not literally named `app_*` as M-Stack's own demo
  does — avoids colliding with a firmware's own `app_*` callbacks if
  something else in the same build also happens to use M-Stack's naming
  convention directly.
- The host-stub test double (`pic8_usb_host_stub.c`) is a genuinely
  separate implementation from `pic8_usb.c`, as this plan's "Host build
  story" already called for — not a shortcut discovered during
  implementation, confirmed as the right call once `pic8_usb.c` turned out
  to depend on M-Stack's raw endpoint API in ways with no reasonable host
  fake (see that section).

## Open risks to resolve during implementation, not now

- **RAM budget.** The PIC18F4550 has 2 KB total RAM; M-Stack's BDT plus
  endpoint buffers plus this module's ring buffers all compete for it.
  Phase 2's real `xc8-cc` build (wrapper + M-Stack core/CDC + a trivial
  `main()`) measured 454 bytes (22.2%) — comfortable, but re-check once
  real application logic sits on top in Phase 3, on real hardware, not by
  estimation.
- **Clock configuration.** The SIE needs a 48 MHz USB clock via the PLL,
  which constrains which `CONFIG` fuse settings and crystal are usable —
  cross-check against whatever crystal the target board actually has
  before assuming a config bit setting from a M-Stack example applies
  unchanged.
- **Descriptor strings and interface naming** are placeholder content until
  Phase 3; don't bikeshed them in this plan.

Not committing any of this until the user reviews Phase 1's findings —
implementation proceeds when they say go, git history stays theirs to
curate.
