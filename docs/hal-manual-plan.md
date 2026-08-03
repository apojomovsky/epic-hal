# HAL manual, multi-family split — plan

Status: **done.** The split described below has been fully executed:
`pic8-common/MANUAL.md` and `pic18fxx5x-hal/MANUAL.md` both exist, in the
shape this document specifies. Kept as the historical design record
(`pic8-common/MANUAL.md`'s own intro cites this document for exactly that
reason); nothing below should be read as pending work. Written after
inspecting the actual state of the docs at the time (not assumed):
`pic16f87xa-hal/MANUAL.md` was then 1416 lines and the *only* manual in the
repo; `pic18fxx5x-hal` had no `MANUAL.md` at all, only a 219-line README
that already contained fragments of the same kind of content (layout,
build, an XC8-interrupt-syntax note, API conventions) but nowhere near the
same depth or a per-peripheral reference.

## Motivation

`docs/multi-family-plan.md` already solved this exact problem for *code*:
extract what's architecture-blind into `pic8-common/` once, keep what's
genuinely register-specific per family under a fixed contract (same names/
signatures, different bodies). The manual has the identical shape problem
but hasn't had the same split applied — today the only manual is 100%
PIC16F87XA content, some of which (naming conventions, the harness, the
handle pattern, host/target build philosophy) is actually already
family-agnostic in substance, just not in location. As a 3rd/4th family
gets added (per `multi-family-plan.md`'s own "How to add family #3"
section), writing each one's manual from scratch means either re-deriving
the agnostic chapters every time (drift risk: a convention fix made in one
family's copy and forgotten in another) or skipping documentation for new
families entirely, which is already what happened to PIC18.

## Decision: split into a shared core-concepts doc + one per-family manual

Mirrors `multi-family-plan.md`'s Option A (shared core, per-family
backend), applied to documentation instead of code:

- **`pic8-common/MANUAL.md`** (new) — the conceptual chapters that are true
  regardless of which family's SFRs are underneath: naming conventions, the
  handle pattern, status codes, the (four-function) harness, the host/
  target build-time split philosophy, the host-sim-backend *pattern*
  (not each family's specific register models), the general "writing an
  ISR-driven peripheral" recipe, and the retargeting/porting guidance.
  Lives next to the shared *code* it describes, same reasoning
  `multi-family-plan.md` used for putting shared code in `pic8-common/`.
- **`<family>-hal/MANUAL.md`** (one per family, same section-numbering
  shape every time) — genuinely per-family content: the device-variant
  table, SFR-specific gotchas, the peripheral chapters (GPIO, timers, CCP,
  USART, MSSP, ADC, …) with real register names/bit behavior, the
  interrupt-vector-count/priority model (PIC16: one vector, no priority;
  PIC18: two vectors + priority, per `multi-family-plan.md`), the
  datasheet-section appendix, and that family's own known-gaps list.
  Each opens with one line: "Family-agnostic conventions, harness, and
  build philosophy: see `pic8-common/MANUAL.md`."

Keeping the same chapter *shape* (e.g., "chapter 10 is always GPIO") across
every family's manual is deliberate — it's what makes hopping from one
family's manual to another's predictable, the documentation equivalent of
the fixed code contract.

## Section-by-section classification of the current `pic16f87xa-hal/MANUAL.md`

| # | Section | Classification | Disposition |
|---|---|---|---|
| — | Title/intro paragraph | Mixed | Template reusable per-family; the specific "DS39582B" citation stays per-family |
| 1 | What this is | Mixed | The device-variant table is per-family; the "why a HAL" framing is agnostic |
| 2 | The big picture | Mixed | The `pic8-common`/per-family split concept (2.1, 2.2) is agnostic; 2.3 "one interrupt vector" is **PIC16-specific** (PIC18 has two + priority) |
| 3 | Quick start | Per-family | Exact commands/paths differ per family |
| 4 | Build systems | Mixed | XC8 v3.x general behavior (`.p1`, `-mdfp`) is agnostic; CMake/Makefile variable names and config-word bits are per-family |
| 5.1 | Naming | **Agnostic** | Move as-is |
| 5.2 | Status codes | **Agnostic** | Move as-is (shared `HAL_StatusTypeDef` already lives in `pic8-common`) |
| 5.3 | Handle pattern | **Agnostic** | Move as-is |
| 5.4 | Interrupts and callbacks | **Agnostic** | The weak-handler/callback pattern is identical across families |
| 5.5 | Datasheet citations | Mixed | The *practice* is agnostic; the "DS39582B" string is per-family |
| 6.1 | What the sim models | Per-family | Register-level behavior is genuinely per-family |
| 6.2-6.4 | Sim API shape, dispatch, writing a sim test | **Agnostic** | Pattern reusable; function name prefixes (`pic16f87xa_sim_*`) differ per family but the shape doesn't |
| 7 | The harness | **Agnostic** | Already lives in `pic8-common/pic8_harness.h` — the doc should too |
| 8.1 | IRQ identity enum | Per-family | Contents differ per family |
| 8.2, 8.3 | Enable/disable API, ISR-driven-peripheral recipe | **Agnostic** | Same shape both families already share (`HAL_IRQ_*`, `HAL_IRQ_SetPriority` per `multi-family-plan.md`) |
| 9-21 | Core (WDT/Sleep) + every peripheral chapter | Per-family | Register-level reference, stays in the family manual |
| 22 | The SFR layer | Per-family | Raw addresses/bit names specific to that datasheet |
| 23 | Device selection | Per-family | Macro table specific to that family's variants |
| 24 | The examples | Mixed | "Examples live under `tests/`, first two build for both host and target" is an agnostic convention; the actual example list is per-family |
| 25 | Retargeting and porting | **Agnostic** | Already anticipates this exact split ("To another PIC16 family... the *structure* is reusable as-is") — promote it, maybe merge with `multi-family-plan.md` |
| 26 | Known gaps and gotchas | Mixed | Most entries are register-specific; two ("`IRQ_Enable` doesn't set GIE", "no `HAL_PPP_MspInit`") describe HAL-wide design decisions — verify against the PIC18 HAL before moving, they may be agnostic too |
| 27 | Datasheet section index | Per-family | Maps HAL modules to that family's datasheet sections |

## What this means concretely

- `pic8-common/MANUAL.md` (new): naming, status codes, handle pattern,
  interrupt callback pattern, the harness, the sim-API shape/dispatch
  pattern, the ISR-driven-peripheral recipe, retargeting guidance. Written
  once, linked from every family manual.
- `pic16f87xa-hal/MANUAL.md` (trimmed): keeps quick start, build-system
  specifics, the device-variant table, one-vector interrupt model, every
  peripheral chapter, the SFR layer, device selection, examples list,
  PIC16-specific gotchas, and the datasheet appendix — each of the moved
  sections replaced by a one-line pointer to `pic8-common/MANUAL.md`.
- `pic18fxx5x-hal/MANUAL.md` (new): same chapter shape as the PIC16 one,
  seeded from what the existing README already has (Layout, Build,
  the resolved XC8 dual-priority-vector interrupt syntax, API conventions)
  plus the peripheral coverage the HAL already implements (GPIO, Timer0-3,
  ECCP1/CCP2, MSSP, EUSART, Comparator, EEPROM, ADC, SPP per the top-level
  README's component table) written up to the same per-peripheral depth as
  the PIC16 manual.

## Also update `docs/multi-family-plan.md`

**Done, though not the way originally planned.** `multi-family-plan.md`'s
old "How to add family #3 (and #4)" checklist was later replaced wholesale
by `docs/adding-a-device.md`, which covers writing the new family's
`MANUAL.md` (against this document's shape) as one of its own steps, so the
documentation step this section asked for exists, just relocated.

## Phased execution

1. Write `pic8-common/MANUAL.md` by extracting and generalizing the
   agnostic sections above out of the current `pic16f87xa-hal/MANUAL.md`
   (de-genericizing anything that still says "PIC16F87XA" where the point
   is family-agnostic).
2. Trim `pic16f87xa-hal/MANUAL.md` to per-family content only, replace
   extracted sections with pointers, renumber the table of contents.
3. Write `pic18fxx5x-hal/MANUAL.md` in the same shape, mining the existing
   README plus the HAL headers/source for the actual per-peripheral
   content (it doesn't exist anywhere yet, so this is new writing, not
   extraction).
4. Add the manual-doc step to `multi-family-plan.md`'s "add a new family"
   checklist.

All four steps above have been completed; this document only records how
the split was designed and done.
