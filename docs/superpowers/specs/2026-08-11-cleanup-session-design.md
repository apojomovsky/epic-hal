# Project-wide cleanup: expression rules, comment pass, doc triage

Status: **approved 2026-08-11, not started**.

## Problem

The repo carries ~25k lines of design docs and bitacores (session
logs, findings narratives, completed plans) that no longer serve
anyone: ~20 files under `docs/superpowers/plans`+`specs`, ~18
top-level `docs/*-plan.md`, and audit/status docs. First-party code
comments range from genuinely useful (datasheet citations, asm
hand-traces) to decorative and narrative (box separators, changelog
prose, `@file` boilerplate). The toolchain files carry 96-line
(Dockerfile) and 31-line (Makefile) header blocks. There is no
written rule set for what a comment or a doc is for, so the pile grows
back after every session.

## Goal

1. A pragmatic, human-approachable ruleset for expression in this
   project (comments + doc lifecycle), living in `AGENTS.md`.
2. A full first-party comment pass applying those rules (~430
   `.c`/`.h` files, the toolchain files, module docs), zero behavior
   change, gated by the host build/test per module.
3. A docs triage: delete implemented plans/specs/bitacores, distill
   anything a future maintainer actually needs into README/MANUAL/
   DEVELOPMENT, fix every dangling reference.
4. The plan-first convention stays but plans become ephemeral:
   deleted when the work lands; git history is the archive.

## Decisions (user-approved)

1. **Third-party code is hands off**: `epic-usb/third_party/m-stack`
   (Microchip's USB stack) and vendor code keep their own style. The
   rules apply to first-party code only.
2. **Docs die outright**: implemented plans/specs/bitacores are
   deleted, not archived. Git history preserves them. Operational
   knowledge that a future maintainer needs moves to the relevant
   README/MANUAL/DEVELOPMENT if not already there.
3. **Plans become ephemeral**: AGENTS.md's convention changes from
   "plan doc with a Status: line, kept forever" to "plan doc written
   during the work, deleted on completion". No Status-line
   bookkeeping. Plan-first itself is unchanged and load-bearing.
4. **Full first-party pass this session**: all first-party `.c`/`.h`,
   the Makefile/Dockerfile/scripts headers, and the module docs, via
   parallel subagents, per-module host ctest as the gate.
5. **AGENTS.md is the rules home**: one conventions file, read first
   by every agent.

## The rules (the centerpiece, lands in AGENTS.md)

### Comments

1. **Why, not what.** Code says what it does; comments carry what the
   code cannot: the non-obvious reason, the datasheet fact, the
   invariant. Comments that restate the line below are deleted.
2. **A comment must earn its lines.** More comment lines than code
   lines is a smell. Hard cap ~8 lines per block; longer needs a real
   justification (a hand-trace of non-obvious asm, a race or
   side-effect proof). Hand-traces survive only where behavior cannot
   be read from the code, compressed to the essential steps.
3. **No decoration.** No `/* ---- name ---- */` separators, no
   `@file`/`@brief` boilerplate repeating the filename. A 1-3 line
   file header is fine when it adds context (which backend, what it
   rides on).
4. **No narrative.** No "fixed X by doing Y", no iteration/session
   prose. Git history owns that.
5. **Register maps and datasheet citations stay.** The datasheet-faithful
   contract is the exception to "why not what": bit-field encodings
   and SFR facts keep their citations.
6. `TODO`/`FIXME` carry a concrete reason or do not exist.
7. No em-dash characters (existing rule).

### Docs lifecycle

1. `README.md` = what a human needs to use and maintain the module:
   purpose, usage, build/test commands, links. Living.
2. `MANUAL.md` = the datasheet-cited register/peripheral reference.
   Living.
3. Design docs are ephemeral: written during the work, deleted on
   completion. Git history is the archive.
4. No bitacores: findings narratives and session logs describing
   completed work are deleted. Live gotchas (asm rules, banking,
   debug protocol, tag formulas) live in README/DEVELOPMENT/MANUAL.
5. Third-party code keeps its own style; rules are first-party only.

## Dispositions (resolved during design)

**Keep (living):**
- All module `README.md` and `MANUAL.md` (datasheet-cited contract).
- `docs/adding-a-device.md`: the verification-gated playbook for new
  devices, referenced by AGENTS.md.
- `docs/layout-budgets.md`: the distilled XC8 layout-constraint
  knowledge (the repo's single most expensive bug class). Already the
  distilled form; optionally linked from AGENTS.md.
- `docs/ARCHITECTURE.md` files that hold live conventions, including
  the two AGENTS.md cites: `epic-math/docs/ARCHITECTURE.md` (XC8
  inline-asm binding) and `pic16f193x-hal/docs/ARCHITECTURE.md` (BSR
  addressing, Finding 1). Pure-narrative ARCHITECTURE files (findings
  logs with no live rule) are triaged per file in the plan: distill
  the live rule, delete the rest.
- `docs/API.md` files: API reference. Kept where they add what the
  README does not; merged into README or deleted where duplicative
  (per-file judgment in the plan).

**Delete:**
- `docs/superpowers/plans/*` and `docs/superpowers/specs/*` (all
  implemented; git history is the archive).
- Top-level `docs/*-plan.md` (multi-family-plan, pic16f193x-plan,
  ci-plan, docker-dev-plan, mplabx-link-gaps-plan, epic-*-plan,
  hal-*-plan, pic8-*-rename-plan, pic8-vga-plan, quality-roadmap,
  etc.). Operational facts they hold that are not already in a
  README/DEVELOPMENT/AGENTS get distilled first (e.g. the docker tag
  formula, the mdb debug protocol, the ci image-history warnings).
- `docs/toolchain-coverage.md`: a completed audit report (all
  SUSPECT sites SAFE or FIXED per its own header). Plan step greps
  for any unresolved SUSPECT before deleting; if any remain open,
  they move to a TODO list, not a report.

**Rewritten:**
- `AGENTS.md`: new Expression conventions section (the rules above);
  the plan-doc/Status-line convention replaced by the ephemeral
  lifecycle; doc references updated for deletions.
- `DEVELOPMENT.md`/`README.md`: any distilled operational knowledge
  that has no current home.
- `scripts/README.md` and script headers: trimmed to the rules.

## Phases

- **Phase 0** : Rules land in `AGENTS.md` (Expression conventions +
  ephemeral-plan lifecycle). One commit.
- **Phase 1** : Toolchain files: Dockerfile header (96 lines to ~10),
  Makefile header (31 to ~10), every `scripts/*.sh` header and
  `*.py` docstring block to the same standard.
- **Phase 2** : Full first-party comment pass: ~30 module groups,
  parallel subagents, each carrying the rules verbatim, per-module
  gate = host `cmake` configure+build+`ctest` + pre-commit checks.
- **Phase 3** : Docs triage: delete per dispositions, distill missing
  operational knowledge, update all references (AGENTS.md, READMEs,
  module docs, workflow comments, script headers).
- **Phase 4** : Reference sweep + final gate: grep for dangling
  references to deleted docs, link scan, full host ctest, script
  tests, pre-commit checks.

## Verification

- Every module still passes its host `cmake` build + `ctest` (the
  repo's own gate; no C semantics change, so this is the proof).
- `python3 scripts/tests/*.py` pass.
- Pre-commit checks (whitespace, no em-dash, cppcheck) pass.
- A repo-wide grep finds no reference to any deleted doc path.
- Markdown links resolve (scan over the surviving docs).

## Out of scope

- Third-party/vendor code (`epic-usb/third_party/**`).
- Any behavior change: comments and docs only, no code edits that
  alter semantics (comment-only diffs; a rename of a documented
  concept is allowed only where the comment itself demanded it).
- The CI classifier and workflows: untouched except where a comment
  references a deleted doc (wording fix only).
- `docs/adding-a-device.md` and `docs/layout-budgets.md` content:
  kept as-is (adding-a-device may get its references updated).
