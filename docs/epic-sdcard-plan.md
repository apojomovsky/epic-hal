# `epic-sdcard`: SD/MMC-over-SPI block storage for PIC18F2455/2550/4455/4550 — implementation plan

Status: **Phase 1/2 done** (vendoring + binding layer + mock-SPI host tests,
see "Phase 1/2 findings" below and `epic-sdcard/docs/ARCHITECTURE.md` for the
as-built version), **Phase 3 (real-silicon bring-up,
`mcu/pic18fxx5x-sdcard-mplabx/`) not started**, matching
`epic-sdcard/README.md`'s own status. Originally written for a fresh
implementing agent with no other context on this conversation. Follows the
same vendor-a-third-party-stack shape as `epic-usb` (`docs/epic-usb-plan.md`),
read that plan first if unfamiliar with this repo's M-Stack vendoring
conventions; this plan assumes it.

## What this is

A block-storage driver for SD/MMC cards over SPI, wrapping M-Stack's
`storage/mmc.c` (SD/MMC-over-SPI, 512-byte block read/write,
SDSC/SDHC/SDXC detection, multi-block writes) and `storage/crc.c` (the
CRC7/CRC16 checksums the SD SPI protocol requires), the same vendored
upstream this repo already pulled in for `epic-usb`
(`github.com/signal11/m-stack`, commit
`979665546f3f39fb4b28a4acedfb1f9a5c55782e`). Unlike the USB stack, this
half of M-Stack has **no chip-specific code at all** — it talks to the SD
card entirely through a small set of caller-supplied SPI/CS/timer
callbacks, so there is no `usb_hal.h`-style peripheral port to write.

## Chip scope: PIC18F2455/2550/4455/4550 only — but for a different reason than epic-usb

Read this before assuming the SPI (SSP) HAL driver being genuinely
portable means the module is too.

`pic18fxx5x-hal` and `pic16f87xa-hal` **both** have a working SSP driver
with the same neutral API (`EPIC_SSP_Init`, `EPIC_SSP_WriteByte`,
`EPIC_SSP_ReadByte`, ...): confirmed by reading both headers;
`pic18fxx5x_ssp.h`'s own doc comment says it "mirrors pic16f87xa_ssp.h ...
so consumer code is portable." Unlike `epic-usb`, there is no hardware
reason this module couldn't link against either family.

**The real blocker is RAM, not the peripheral.** `mmc.h` fixes
`MMC_BLOCK_SIZE` at 512 bytes — a hard constant, "the only size supported
by this implementation," not a tunable knob, because that's what the SD
protocol's block size is. Every `pic16f87xa-hal` family member's *total*
RAM is smaller than one block: `PIC16F87XA_FAMILY_RAM_BYTES` is 192 for
PIC16F873A/874A and 368 for PIC16F876A/877A (confirmed in
`pic16f87xa-hal/include/pic16f87xa.h`) — even the family's largest part
falls 144 bytes short of holding a single SD block buffer, before
accounting for the `mmc_card` struct, CRC state, the call stack, or
anything the caller's own firmware needs. PIC18F4550 has 2048 bytes and
comfortably clears it (confirmed for real: `epic-usb`'s wrapper alone used
454 bytes per the actual `xc8-cc` build in that plan's Phase 2 findings,
leaving room for a 512-byte buffer alongside it, though combining both in
one firmware image is worth re-checking for real once it's built rather
than assumed — see "Open risks" below).

So: build the SPI/GPIO/timer binding layer in a way that's honestly
portable C (no reason to hand-tie it to PIC18 SFRs directly, when the HAL
already isn't family-specific here) — but scope the module's *target* list
to PIC18F2455/2550/4455/4550, same as `epic-usb`, because that's the only
family where the fixed 512-byte block size actually fits. If this repo
ever gains a PIC18 or PIC16 family member with less RAM, or M-Stack's
block size assumption changes upstream, revisit; not true today.

## Third-party dependency: same M-Stack commit, a separate small vendored copy

Extract just `storage/include/{crc.h,mmc.h}` and `storage/src/{crc.c,mmc.c}`
(roughly 1,200 lines total, vs. the full USB stack) from the same pinned
upstream commit already vendored for `epic-usb`
(`979665546f3f39fb4b28a4acedfb1f9a5c55782e`) into
`epic-sdcard/third_party/m-stack-storage/`. **Do not** make `epic-sdcard`
depend on `epic-usb/third_party/m-stack/` directly — this module has zero
USB dependency, and reaching into a sibling module's `third_party/` would
be a real coupling for no reason; a second small vendored copy, pinned the
same way, is the right amount of duplication (mirrors why `epic-common` is
the shared-code answer for *this repo's own* code, not for third-party
vendoring). Same Apache-2.0 arm of the LGPLv3/Apache-2.0 dual license, same
`VENDORED_COMMIT`-file convention as `epic-usb/third_party/m-stack/`.

Confirm both files still compile standalone (no `usb.h`/`usb_config.h`
dependency leaked in from being physically colocated with the USB stack
upstream) as the first implementation step — they read as self-contained
from `mmc.h`'s own doc comment ("A driver for the SPI hardware itself is
not provided here... client software... provide[s]... `mmc_config.h`"),
but confirm before building on top.

## Confirmed API surface (read directly from the vendored headers)

- `crc.h`: `add_crc7(csum, input)`, `add_crc16(csum, input)`,
  `add_crc16_array(csum, data, len)` — pure math, no callbacks, no state
  beyond the running checksum the caller threads through.
- `mmc.h`: `mmc_init(card_array, count)`, `mmc_init_card(card)`,
  `mmc_get_num_blocks(card)`, `mmc_ready(card)`, `mmc_is_initialized(card)`,
  `mmc_set_uninitialized(card)`, `mmc_read_block(card, block_addr, data)`,
  `mmc_write_block(card, block_addr, data)`, and a multi-block write
  triplet (`mmc_multiblock_write_start/data/end`, plus `_cancel`). `data`
  buffers are always exactly `MMC_BLOCK_SIZE` (512) bytes.
- Required callback macros, bound in a caller-supplied `mmc_config.h`
  (M-Stack's compile-time binding convention, same as `usb_config.h` for
  the USB stack): `MMC_SPI_TRANSFER(instance, out_buf, in_buf, len)`
  (blocking full-duplex SPI transfer), `MMC_SPI_SET_CS(instance, value)`
  (0=asserted), `MMC_SPI_SET_SPEED(instance, speed_hz)` (set SPI clock
  ≤ the requested rate). `mmc.c` calls `MMC_SPI_SET_SPEED` with a slow rate
  during card bring-up (SD spec requires ≤ 400 kHz until the card leaves
  idle state) and with `max_speed_hz` afterward — this module's binding
  needs to actually change the SSP clock divisor at runtime, not just
  accept and ignore the call.
- Optional `MMC_USE_TIMER` + `MMC_TIMER_START/EXPIRED/STOP(instance, ...)`:
  if undefined, `MMC_TIMER_EXPIRED` becomes a hardcoded `false` and
  `mmc.c` falls back to bounded retry counts (e.g.
  `NUM_ACMD41_RETRIES 32768`) with no real wall-clock timeout. **This
  module should define `MMC_USE_TIMER`** and bind the three macros to
  `epic-tick` (same dependency `epic-debounce` already takes for real
  timeout semantics, not a synthetic one) — `mmc.c` already has real
  spec-derived timeout constants sitting unused otherwise
  (`MMC_READ_TIMEOUT 150` /* ms, SD spec 4.6.2.1 */,
  `MMC_WRITE_TIMEOUT 500` /* ms, SD spec 4.6.2.2 */); binding them for real
  is strictly better than the retry-count fallback and costs nothing extra
  given `epic-tick` is already proven family-agnostic.

## Public API design

```c
/* epic-sdcard/include/epic_sdcard.h */
#include <stdint.h>
#include <stdbool.h>

typedef struct {
    GPIO_TypeDef  cs_port;   /* board-specific: which pin the card's CS is wired to */
    uint16_t      cs_pin;
} epic_sdcard_pins_t;

/* Owns one mmc_card instance internally; only one card is in scope for
 * this plan (mmc.h's array-of-cards shape stays available to a future
 * caller who wants more, this module just doesn't multiplex it yet --
 * don't build multi-card support speculatively). */
bool     epic_sdcard_init(const epic_sdcard_pins_t *pins, uint32_t fosc_hz);
bool     epic_sdcard_ready(void);
uint32_t epic_sdcard_num_blocks(void);
bool     epic_sdcard_read_block(uint32_t block_addr, uint8_t *data /* 512 B */);
bool     epic_sdcard_write_block(uint32_t block_addr, const uint8_t *data /* 512 B */);
```

Thin on purpose: `mmc.h`'s functions are already the right shape (unlike
M-Stack's raw USB endpoint API, which genuinely needed `epic-usb.c` to
invent ring buffering on top). This module's real work is the *binding*
layer — `mmc_config.h` pointing `MMC_SPI_TRANSFER`/`MMC_SPI_SET_CS` at
`EPIC_SSP_WriteByte`/`EPIC_GPIO_WritePin`, `MMC_SPI_SET_SPEED` at a small
Hz-to-`SSP_ModeTypeDef` divisor-picking helper (nearest-not-exceeding
among Fosc/4, /16, /64), and the timer macros at `epic-tick` — plus
`epic_sdcard_init/ready/num_blocks/read_block/write_block` as thin
call-throughs to `mmc_init`/`mmc_ready`/`mmc_get_num_blocks`/
`mmc_read_block`/`mmc_write_block` against the one owned `mmc_card`.
Multi-block write is real, spec-legitimate functionality but adds a
three-call state machine to the public API for a use case (streaming
writes across many blocks) this plan doesn't have a concrete caller for
yet — leave it reachable through the vendored `mmc.h` directly for now,
don't wrap it until something needs it.

**CS pin is caller-supplied, not assumed.** SCK/SDI/SDO are fixed to the
SSP peripheral's pins (RC3/RC4/RC5 per `pic18fxx5x_ssp.h`), but CS is
ordinary GPIO and different boards wire it to different pins — same
"don't assume the caller's hardware" reasoning `epic-debounce`'s read
callback and `epic-adcfilter`'s read callback already establish for this
repo, just via a GPIO pin pair instead of a function pointer since GPIO
control is already HAL-neutral.

## Host build story — genuinely stronger than epic-usb's

`epic-usb`'s host build could only test the ring-buffer logic this repo's
own code owns, because there is no faithful simulation of a real USB SIE
enumerating against a real host driver stack. **SD-over-SPI does not have
that problem** — it's a well-documented, deterministic byte-level
command/response protocol (send a command byte sequence, read a response
token, optionally clock out/in a data block + CRC16). A host build can
implement a **mock SPI slave** that plays the SD card's side of that
protocol (respond to `CMD0`/`CMD8`/`ACMD41`/`CMD17`/`CMD24` etc. the way a
real card would) and bind `MMC_SPI_TRANSFER` to it instead of real
hardware. That means `mmc_init_card`/`mmc_read_block`/`mmc_write_block`'s
actual logic — not just a hand-written stand-in for it, unlike
`epic_usb_host_stub.c` — can be host-tested directly, the same
"host tests prove the shipped code" story `epic-fsm`/`epic-debounce`
already have. `crc.c`'s CRC7/CRC16 are pure math and trivially host-tested
against known SD CRC test vectors with zero mocking at all.

This is real design/implementation work (the mock SPI slave needs to
actually implement enough of the SD SPI command set to be worth trusting),
budget real time for it in Phase 2 — but it is a strictly better
validation story than what was available for the USB module, worth calling
out so the difference isn't missed.

## Repo layout

```
epic-sdcard/
  CMakeLists.txt                      # host static lib (against the mock SPI slave) + unit tests
  include/
    epic_sdcard.h
  src/
    epic_sdcard.c                     # HAL/tick binding + mmc_config.h, real target
    epic_sdcard_mock_spi.c            # host build: mock SD-over-SPI slave (test-only, not shipped)
  third_party/
    m-stack-storage/                  # vendored crc.{c,h}, mmc.{c,h}, own VENDORED_COMMIT
  examples/
    example_sdcard_dump.c             # read block 0, print first N bytes -- the smoke test
  tests/
    test_epic_sdcard.c                # init sequence, read/write round-trip, CRC vectors, timeout path
  mcu/
    pic18fxx5x-sdcard-mplabx/Makefile # XC8 Makefile, real silicon only (see chip-scope reasoning above)
  docs/
    ARCHITECTURE.md
    API.md
  README.md
```

## Milestones

- **Phase 1 — vendor `storage/`, confirm it's genuinely self-contained.**
  Extract into `third_party/m-stack-storage/`, confirm no leaked USB
  dependency, re-read `mmc.c`'s actual retry/timeout constants and
  multi-block state machine closely enough to be sure this plan's API
  sketch is accurate (same "confirm before coding" discipline
  `epic-usb-plan.md` used).
- **Phase 2 — binding layer + mock-SPI host tests.** `epic_sdcard.h/.c`,
  the `EPIC_SSP`/`EPIC_GPIO`/`epic-tick` bindings, the clock-divisor helper,
  the mock SPI slave, `tests/test_epic_sdcard.c`. Given this environment
  has XC8 actually installed (per `epic-usb`'s Phase 2), also attempt a
  real `xc8-cc` compile+link against PIC18F4550 the same way, including
  checking real RAM footprint against the "Open risks" concern below —
  don't just estimate it.
- **Phase 3 — real hardware.** `mcu/pic18fxx5x-sdcard-mplabx/Makefile`,
  wire an actual SD card breakout to a real board's SPI pins + a chosen CS
  pin, confirm `mmc_init_card` actually succeeds against a real card and a
  read/write/read round-trip matches. Real-card-only; no amount of host
  mocking substitutes for this, same caveat `epic-usb-plan.md` has for USB
  enumeration.
- **Phase 4 — docs.** `docs/ARCHITECTURE.md`, `docs/API.md`,
  `epic-sdcard/README.md`, root `README.md` component-table row.

## Phase 1/2 findings

**Vendoring (Phase 1): done, self-contained as expected.** Extracted into
`epic-sdcard/third_party/m-stack-storage/`, same pinned upstream commit as
`epic-usb`. Confirmed no leaked USB dependency (grepped for
`usb.h`/`usb_config`/etc., none found). Two small, cosmetic host-build
compatibility gaps turned up while actually compiling it (not functional
bugs, just this vendored code never having been asked to build outside
XC8/XC16/XC32 or with `<xc.h>` unavailable): `mmc.c`'s own compiler
allowlist didn't include a host-build option, and `mmc.h` used `size_t`
without including `<stddef.h>` (relying on `<xc.h>`, pulled in from
`mmc.c` not this header, to provide it transitively -- which a host build
skips entirely). Both fixed and extracted as
`epic-sdcard/third_party/patches/0001-host-build-compat.patch` (verified
reapplicable the same way `epic-usb`'s patch was: dry-run + real apply
against a pristine copy, byte-identical result -- **note this one needs
`-p0`, not `-p1`**, since its paths already include the `epic-sdcard/`
prefix). `crc.{c,h}` needed no changes at all.

**Mock SPI slave (Phase 2): built, and it caught a real bug in itself
worth recording.** Implements CMD0/8/55/41/58/9/16/17/24/13 against a
small 4-block in-memory backing store (the card *reports* the planned
1024-block/512 KiB size via a crafted CSD register, matching a real SDHC
card's response shape, but only the first 4 blocks are actually backed --
see the mock's own header comment). 27 host tests pass: init sequence,
read of pre-seeded data, write-then-read round trip, two blocks staying
independent, reads rejected past the reported size and before init, and
CRC7/CRC16 self-consistency checks.

Getting there required correcting a wrong assumption from this plan's
first draft: **the CRC16 self-check `__read_data_block` relies on
requires the trailing CRC bytes in MSB-first (`[high, low]`) order, not
the LSB-first order `mmc_write_block` happens to send them in.** The first
mock draft mirrored `mmc_write_block`'s send order, reasoning "the sender
and the self-check must agree" -- which is wrong, because `mmc_write_block`'s
own CRC is *never independently verified by anything in mmc.c* (writes are
validated by the card's data-response token instead, not a self-check);
its byte order is simply an unrelated, unverified convention. Verified
which order actually self-checks to zero with a standalone probe against
the real vendored `crc.c` before fixing it (not guessed) -- getting this
wrong initially cascaded into 21 of 27 test failures, since `CMD9`
(`SEND_CSD`, needed by every single `mmc_init_card()` call) uses the same
data-block-reply path.

**Real XC8 compile validation (Phase 2, going beyond this phase's
original "host tests only" scope, same as `epic-usb` did):**

- `crc.c` + `mmc.c` alone (no HAL, stub SPI callbacks) compile *and link*
  clean against real PIC18F4550 headers: **3924 bytes flash (12.0%), 919
  bytes RAM (44.9% of 2048)** -- a real, measured number, substantially
  higher than this plan's original "just the 512-byte block buffer"
  framing assumed. The gap is `mmc.c`'s own internal call-graph-allocated
  locals across its many static helper functions (`skip_bytes_timeout`,
  `__read_data_block`, the `uint8_t buf[16]` in `mmc_init_card`, etc.) --
  XC8's stack-less "auto" variable model means every function's locals
  get real, permanent RAM allocation shaped by the call graph, not a
  transient stack frame.
- `epic_sdcard.c` also compiles clean standalone (after fixing a
  self-inflicted bug: a doc comment containing the literal text
  `EPIC_SSP_*/EPIC_GPIO_WritePin` accidentally closed the block comment
  early at the `*/` substring, corrupting everything parsed after it --
  worth remembering as a gotcha for future doc comments in this repo).

**Unresolved, real risk: the full integrated link did not complete.**
`epic_sdcard.c` + `mmc.c`/`crc.c` + the GPIO/SSP/Timer2/IRQ/WDT HAL +
`epic-tick` + a minimal `main()` was attempted three times (default
optimization, then `-O0`, with a trimmed-to-only-what's-needed HAL source
list) and none finished within several minutes each -- no memory-summary
success, no clear "doesn't fit" failure either, just no completion. Given
`crc.c`+`mmc.c` alone already consume 44.9% of RAM, the most plausible
explanation is that adding `epic_sdcard.c`'s own locals plus the HAL's
pushes XC8's auto-variable bank-packing search into a genuinely slow
regime under real memory pressure, rather than any code being wrong (every
piece compiles clean individually). **This is a real, unresolved
question, not glossed over: whether the full wrapper practically fits and
links in reasonable time on a PIC18F4550 is unproven.** Worth
investigating in Phase 3 with real MPLAB X tooling (which may report
progress/diagnostics this command-line invocation didn't surface) --
candidate mitigations if it's genuinely too tight: the linker's own
"pointer ... may have no targets" advisories for `mmc_multiblock_write_start/
data/end` suggest those functions (unused by `epic_sdcard.h`'s wrapped
API, see "Public API design" above) might be excludable from the link to
shrink the call graph, or the caller's own firmware may simply need to be
lean elsewhere on a 2 KB part.

## Open risks to resolve during implementation, not now

- **RAM headroom is tighter than expected even standalone, per Phase 2's
  real measurements** — see "Phase 1/2 findings" above: `crc.c`+`mmc.c`
  alone already measured 919 bytes (44.9% of 2048) on real PIC18F4550
  headers, and the full integrated link (adding `epic_sdcard.c` + the
  HAL) didn't complete in three real attempts. Combining with `epic-usb`
  (454 bytes alone) in one firmware image — e.g. a USB mass-storage
  device exposing the SD card, `apps/msc_test/` in the vendored M-Stack
  tree is exactly that combination — is likely tighter still and
  unproven; not attempted in this plan.
- **SPI clock divisor granularity.** Fosc/4, /16, /64, and TMR2/2 are the
  only options `SSP_ModeTypeDef` exposes; the nearest-not-exceeding helper
  may land meaningfully under a card's actual max supported speed for some
  `Fosc`/target-speed combinations. Not a correctness problem (SD cards
  tolerate being clocked slower than their max), but worth noting in
  `docs/API.md` so a caller chasing throughput knows to check the actual
  achieved rate, not just the requested one.
- **Multi-block write correctness is unverified** until Phase 3 — it's
  real M-Stack code, not something being newly written, but nothing in
  Phase 1/2 exercises it (this plan doesn't wrap it yet, see "Public API
  design" above).

Not committing any of this until the user reviews Phase 1's findings —
implementation proceeds when they say go, git history stays theirs to
curate.
