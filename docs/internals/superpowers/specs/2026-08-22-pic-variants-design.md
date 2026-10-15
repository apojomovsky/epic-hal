# PIC Variant Support (HAL) -- device registry + SFR generation + canonical-per-core CI -- Design

**Status:** draft (pending user approval)<br>
**Date:** 2026-08-22<br>
**Parent:** `epic-cc/docs/superpowers/specs/2026-08-22-pic-variants-design.md` (compiler contract), `epic-hal/docs/adding-a-device.md` (existing playbook), `epic-common/MANUAL.md`<br>
**Scope:** HAL only. `epic-cc#83` is the compiler-side contract this spec consumes.<br>
**Tickets:** `epic-hal#70` (registry + generation + CI), `#71` (p16f887 exemplar).

---

## 1. Goal and non-goals

**Goal:** adding a same-core PIC to the HAL is a generated header + a board entry, and HAL CI does not grow as `devices × peripherals × cores`. Same "file per device" promise as the compiler, on the HAL's terms.

**Non-goals (v1):**

* `pic14e` family (`pic16f193x-hal` -- foundation only, GPIO+Timer0, per doc 31 D-1) -- explicitly out of scope until `epic-cc` has `isel-pic14e`.
* Per-device full peripheral matrix. By the same core-vs-data argument as the compiler, a peripheral bug is family-wide or data-wide; one canonical per family finds family bugs, one smoke finds data bugs.
* `epic-math` assembly conformance (HAL-3, #59) and `epic-serial` `printf` retarget beyond what `epic-cc --target` already covers.

---

## 2. Current state

* `docs/adding-a-device.md` already describes the manual path: transcribe SFRs, bitfields, config bits from DS + DFP, add `mcu/<part>/` or `mcu/<family>/` entry, wire `epic_build.py`, add a board JSON, run host + `mdb` gates.
* Each HAL (`pic16f87xa-hal`, `pic18fxx5x-hal`, `pic16f88x-hal`) carries hand-written `target/<part>.h` or `proc/<part>.h`-derived headers. Adding a device edits multiple files.
* CI today: `ci.yml` → `host` job + one `family-check.yml` per family, each cross-compiles every MCU variant of that family and runs `mdb`/bundle audits. Growth is already `families × variants` and will be linear in devices if not stratified.

---

## 3. Decisions

### 3.1 SFR headers become generated, source stays TOML/ATDF

* Source of truth is the same ATDF that `epic-cc` uses to generate `devices/<name>.toml` -- Microchip DFP `*.atdf` (authoritative, free download), `gputils` `.inc` as byte-for-byte oracle, XC8 headers black-box only (AGENTS.md GPL boundary).
* Generator is `scripts/gen-device.py` (shared with `epic-cc`, or a HAL-side wrapper) -- input ATDF, output:
  * `epic-cc` side (already): `crates/device/devices/<name>.toml` (memory map + config + optional `sfrs` table);
  * HAL side: `pic16f87xa-hal/include/generated/<name>.h` (or `pic16f88x-hal/...`, or shared `include/epic-cc/devices/<name>.h` -- decision in §4).
* Hand-written headers remain for already-supported parts until they are re-generated and diffed; `gen-device --check` + `git diff --exit-code` in CI gates drift for new parts.

### 3.2 Build selection via `-mcu <name>`

HAL-2 (#58) already teaches `epic_build.py`/`epic-hal.mk` to drive `epic-cc --target <name>`. This spec adds the header side: `-mcu p16f887` selects `generated/p16f887.h` via an include path, no hand-edited `EQU`s. `epic_build.py` forwards `-mcu` to both the compiler flag and the include resolution.

### 3.3 CI stratification (mirrors `epic-cc#84`)

| Gate | When | What |
|------|------|------|
| **Canonical per family** | Every PR, always | e.g. `pic16f87xa` on `p16f877a`, `pic18fxx5x` on `p18f4550`. Full per-module gate per `docs/adding-a-device.md` §4 (host + `mdb` register read) + bundle audit. Fixed `families` jobs, not `variants`. |
| **Per-device lightweight** | PR iff `generated/<name>.h` or `mcu/<name>/` or `boards/<name>.json` touched; nightly always | Single smoke (`blinky` + `host` sim) on that `mcu` via `epic-cc --target <name>` → `sim` or `mdb` read-back of one register. ~1 min/device. |
| **Nightly** | `schedule: cron` + `workflow_dispatch` | Lightweight for *all* `mcu/*` boards. |
| **Never** | -- | Full `variants × peripherals` on every PR. A peripheral bug is family-wide by the HAL's fixed-contract design (`epic-common` pan). |

`docs/adding-a-device.md` §4's "mandatory verification gate" remains for the canonical; per-device lightweight still runs the same gate but on one peripheral (blinky) to prove data wiring, not peripheral coverage.

---

## 4. Header layout

Option A (recommended): `pic16f87xa-hal/include/generated/<name>.h` and `pic16f88x-hal/include/generated/<name>.h` per family, generated files only, `.gitignore` the ATDF itself. `generated/` is the clear "do not hand-edit" signal.

Option B: shared `include/epic-cc/devices/<name>.h` consumed by both HALs -- fewer files but cross-family sharing of truly per-family headers is rarely correct (different `ANSEL` vs `ANSELA` etc.). Rejected for v1; can be layered if a family later proves identical.

Each header exposes `volatile` SFR defs + bitfield masks exactly as the DS names them, with `DS+section` comments. Size is not a concern; `gputils` `p16f887.inc` is ~2K lines, ATDF is larger but the header is a projection.

---

## 5. `mcu/` board wiring

`mcu/<name>/` or `mcu/<family>/<name>/` holds the minimal `Makefile` fragment / JSON that maps `name` to:

* compiler flag `mcu = p16f887` (forwarded as `--target p16f887`);
* `generated/<name>.h` include path;
* `boards/<name>.json` for PlatformIO (PIO-1 contract: `"mcu": "<name>"`).

Adding a device is one header + one `mcu/` entry + one board JSON -- same "file per device" claim as `cc`.

---

## 6. `epic-platformio` linkage

`epic-platformio/boards/<name>.json` already maps `"mcu"` to the `epic-cc --target` string (spec §11). The HAL's `framework-epichal` package bundles the `generated/` headers, so `pio run --environment p16f887` compiles without a Microchip pack -- same "no download" promise as the compiler side.

---

## 7. Sequencing

1. HAL-2 (#58) lands `epic-cc` toolchain branch in `epic_build.py` (claimed). No change here.
2. `#70` -- add generator wiring + CI split (canonical vs changed-device). No new device yet.
3. `#71` -- `p16f887` exemplar: `generated/p16f887.h` + `mcu/p16f887` + `boards/p16f887.json`, one smoke (`blinky` on `p16f887` via `sim`/`mdb`). Proves "file per device" on HAL side.
4. Follow-up: re-generate existing parts (`p16f877a`, `p18f4550`) and diff.

No parallel crates needed; keep PRs slice-shaped (#70, then #71).

---

## 8. Testing

* Per-family canonical: full host `ctest` + `mdb` register read for every peripheral in that family (existing `adding-a-device.md` §4).
* Per-device lightweight: `blinky` on that `mcu` → `sim` (HAL-4, #60) or `mdb` single register read; proves header + memory map + vector wiring.
* Negative: unknown `-mcu` lists available `mcu/*` stems (mirrors `cc`).

---

## 9. Risks

* **Bad SFR bit** -- caught by `mdb` single-register read, not by running every peripheral. Family-wide bugs remain on the canonical.
* **ATDF licence** -- `.atdf` itself never committed, only generated headers (same posture as `cc`'s TOML).
* **`pic14e` mistaken for `pic14`** -- `epic-cc` firewall (`core = "pic14e"` panic) propagates to HAL build failure; HAL build should surface "no backend for core pic14e" rather than silently compiling wrong bank code.

---

## 10. Not in v1

* `pic14e` family bring-up, `epic-math` assembly, `epic-serial` retarget beyond toolchain flag pass-through, full `variants × peripherals` matrix.
