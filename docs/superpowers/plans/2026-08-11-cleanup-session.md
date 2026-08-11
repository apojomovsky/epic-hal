# Project-wide Cleanup Implementation Plan

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:subagent-driven-development (recommended) or superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

Status: **approved 2026-08-11, in progress**.

**Goal:** Apply the new expression rules (comments + doc lifecycle) across the whole first-party project: rules land in AGENTS.md, toolchain headers shrink, ~430 first-party `.c`/`.h` files get a comment pass, ~25k lines of implemented design docs and bitacores are deleted with operational knowledge distilled to READMEs, and every dangling reference is fixed.

**Architecture:** Phased. Phase 0 establishes the rules in AGENTS.md. Phase 1 trims the toolchain files. Phase 2 runs 11 parallel per-module-group comment passes (each gated by that module's host cmake build + ctest). Phase 3 deletes the bitacores and one-shot migration tooling, triages module ARCHITECTURE/API docs, then fixes every reference. Phase 4 is the final gate. Comments and docs only, zero behavior change; git history is the archive.

**Tech Stack:** C (XC8 + host gcc), CMake/ctest, bash, python3 (scripts), Markdown, git.

## Global Constraints

- No em-dash characters in added lines, commit messages, or code comments (the word "em-dash" is fine).
- Conventional Commits (`feat`/`docs`/`plan`/`fix`/`refactor`/`style`), scope usually the module.
- **Comment-only changes**: no C semantics change. If a comment contradicts the code, fix the comment; never change the code to match a comment.
- Third-party code (`epic-usb/third_party/**`) is hands off, untouched.
- The per-module gate is the host build: `cmake -B build-host/<module> -S <module> && cmake --build build-host/<module> && ctest --test-dir build-host/<module> --output-on-failure` (module root-relative paths; run from the module's parent, i.e. the repo root of the worktree).
- The pre-commit hook runs on every commit (whitespace, no em-dash on added lines, cppcheck); it passes for comment-only changes that follow the rules. Never bypass it.
- The rules below are the single source of truth for the comment pass; every Phase 2 task carries them.

## The rules (verbatim, from the spec; each Phase 2 task repeats a compact form)

1. **Why, not what.** Code says what it does; comments carry what the code cannot: the non-obvious reason, the datasheet fact, the invariant. Comments that restate the line below are deleted.
2. **A comment must earn its lines.** More comment lines than code lines is a smell. Hard cap ~8 lines per block; longer needs a real justification (a hand-trace of non-obvious asm, a race or side-effect proof). Hand-traces survive only where behavior cannot be read from the code, compressed to the essential steps.
3. **No decoration.** No `/* ---- name ---- */` separators, no `@file`/`@brief` boilerplate repeating the filename. A 1-3 line file header is fine when it adds context (which backend, what it rides on).
4. **No narrative.** No "fixed X by doing Y", no iteration/session prose. Git history owns that.
5. **Register maps and datasheet citations stay.** The datasheet-faithful contract is the exception to "why not what": bit-field encodings and SFR facts keep their citations.
6. `TODO`/`FIXME` carry a concrete reason or do not exist.
7. No em-dash characters.

---

### Task 1: Rules land in AGENTS.md (Phase 0)

**Files:**
- Modify: `AGENTS.md`

**Interfaces:**
- Produces: the conventions that every later task's brief cites; the doc-lifecycle rule change that Phase 3's deletions depend on conceptually.

- [ ] **Step 1: Replace the "Update the docs" bullet**

Old:

```markdown
- **Update the docs a change touches before calling it done**: the
  module's `README.md`/`docs/API.md`/`docs/ARCHITECTURE.md` if
  behavior changed, `MANUAL.md` if a register fact changed, the
  `Status:` line of the relevant `docs/<name>-plan.md`. A repo-wide
  audit once found stale `Status: not started` lines on shipped modules
  as the norm, not the exception, because this step kept getting
  skipped.
```

New:

```markdown
- **Update the docs a change touches before calling it done**: the
  module's `README.md`/`docs/API.md`/`docs/ARCHITECTURE.md` if
  behavior changed, `MANUAL.md` if a register fact changed.
```

- [ ] **Step 2: Replace the plan-first bullet**

Old:

```markdown
- **Non-trivial work gets a plan doc first**: `docs/<name>-plan.md`, a
  `Status:` line, explicit solved-vs-pending framing.
```

New:

```markdown
- **Non-trivial work gets a plan doc first** (`docs/superpowers/plans/`),
  and the plan is **ephemeral**: it lives during the work and is
  deleted when the work lands. Git history is the archive. No
  `Status:` line bookkeeping; a design doc for implemented work is a
  bitacore, not documentation.
```

- [ ] **Step 3: Add the Expression conventions section** at the end of the Conventions section (before "## Non-obvious things that will bite you" if that is where Conventions ends; otherwise after the last Conventions bullet):

```markdown
## Expression conventions (comments and docs)

### Comments

1. **Why, not what.** Code says what it does; comments carry what the
   code cannot: the non-obvious reason, the datasheet fact, the
   invariant. A comment that restates the line below it is deleted.
2. **A comment must earn its lines.** More comment lines than code
   lines is a smell. Hard cap ~8 lines per block; longer needs a real
   justification (a hand-trace of non-obvious asm, a race or
   side-effect proof). Hand-traces survive only where behavior cannot
   be read from the code, compressed to the essential steps.
3. **No decoration.** No `/* ---- name ---- */` separators, no
   `@file`/`@brief` boilerplate that repeats the filename. A 1-3 line
   file header is fine when it adds context (which backend, what it
   rides on).
4. **No narrative.** No "fixed X by doing Y", no iteration or session
   prose. Git history owns that.
5. **Register maps and datasheet citations stay.** The
   datasheet-faithful contract is the exception to "why not what":
   bit-field encodings and SFR facts keep their citations.
6. `TODO`/`FIXME` carry a concrete reason or do not exist.

### Docs lifecycle

1. `README.md` = what a human needs to use and maintain the module:
   purpose, usage, build/test commands, links. Living.
2. `MANUAL.md` = the datasheet-cited register/peripheral reference.
   Living.
3. Design docs are ephemeral: written during the work, deleted on
   completion. Git history is the archive.
4. No bitacores: findings narratives and session logs describing
   completed work are deleted. Live gotchas (asm rules, banking,
   debug protocol, tag formulas) live in README/DEVELOPMENT/MANUAL,
   the places a future maintainer actually reads.
5. Third-party code keeps its own style; these rules are first-party
   only.
```

- [ ] **Step 4: Verify**

Run: `PRE_COMMIT_BASE_REF=origin/master scripts/pre-commit-checks.sh`
Expected: PASS. Grep AGENTS.md for a literal em-dash character: no new ones.

- [ ] **Step 5: Commit**

```bash
git add AGENTS.md
git commit -m "docs(agents): add expression conventions, make plans ephemeral"
```

---

### Task 2: Toolchain file headers (Phase 1)

**Files:**
- Modify: `docker/ci-toolchain/Dockerfile` (96-line header)
- Modify: `Makefile` (31-line header)
- Modify: `scripts/bootstrap.sh`, `scripts/install-git-hooks.sh`, `scripts/pre-commit-checks.sh`, `scripts/sim-mdb-run.sh`, `scripts/sim-test-local.sh`, `scripts/ci-target-build.sh`, `scripts/ci-target-sim.sh`, `scripts/ci-target-bundle.sh` (headers)
- Modify: `scripts/*.py` module docstrings: `epic_build.py`, `epicmanifest.py`, `make_bundle.py`, `bundlegen.py`, `ci-discover-affected-modules.py`, `ci_noncode_check.py`, `ci-local-emit.py`, `sfr-map-audit.py`, `config-key-audit.py`, `statics-audit.py`, `hex-identity-audit.py`, `serial-rx-loop.py`
- Do NOT touch: `scripts/rename-*.sh`/`rename-*.awk`, `scripts/hal-epic-exceptions.txt`, `scripts/pic8-epic-modules.txt` (deleted in Task 13).

**Interfaces:**
- Consumes: the rules (Global Constraints).
- Produces: headers that state what the file is for in 3-10 lines, keeping only load-bearing facts (the Dockerfile keeps: version pins are ARGs; installers come from vendor/ by hand because of the bot-challenge; cmake/build-essential are for local make test; mdb is installed for mdb.sh only, 8-bit only, pruned in the same RUN; the tag formula lives in the Makefile/workflows, not the header). The Makefile keeps: docker-first entry point summary + the LOCAL_IMAGE/tag formula pointer + the `--user`/passwd/HOME mount caveat (that one is a real gotcha, keep it compressed to ~6 lines).

- [ ] **Step 1: Read each target file's header**, then rewrite it to 3-10 lines per the rules. Keep genuinely load-bearing facts (the Dockerfile's bot-challenge/vendor/ explanation, the Makefile's user-mount gotcha); delete version-history prose, "two consumers one image" essays, and chunk-by-chunk prune narratives (that history is in git).
- [ ] **Step 2: Trim the python module docstrings** to 2-6 lines stating what the script does and who calls it. The audit scripts' docstrings currently re-explain their whole rationale; keep the purpose line.
- [ ] **Step 3: Verify**

Run: `bash -n scripts/bootstrap.sh scripts/install-git-hooks.sh scripts/pre-commit-checks.sh scripts/sim-mdb-run.sh scripts/sim-test-local.sh scripts/ci-target-build.sh scripts/ci-target-sim.sh scripts/ci-target-bundle.sh`
Expected: no output, exit 0.

Run: `python3 -m py_compile scripts/*.py`
Expected: no output, exit 0.

Run: `PRE_COMMIT_BASE_REF=origin/master scripts/pre-commit-checks.sh`
Expected: PASS.

- [ ] **Step 4: Commit**

```bash
git add docker/ci-toolchain/Dockerfile Makefile scripts/
git commit -m "style: trim toolchain file headers to the expression rules"
```

---

### Task 3: Comment pass, pic16f87xa-hal

**Files:** all first-party `.c`/`.h` under `pic16f87xa-hal/` (~60 files)
**Gate:** `cmake -B build-host/pic16f87xa-hal -S pic16f87xa-hal && cmake --build build-host/pic16f87xa-hal && ctest --test-dir build-host/pic16f87xa-hal --output-on-failure`

- [ ] **Step 1: Apply the rules** to every file: trim file headers to 1-3 context lines (family, peripheral set, MANUAL link stays), delete box separators and `@file`/`@brief` boilerplate, delete changelog/narrative prose, keep every register map, bit-field encoding, and datasheet citation (rule 5), compress any long hand-traces or keep only the essential invariant statement.
- [ ] **Step 2: Run the gate** (command above). Expected: PASS.
- [ ] **Step 3: Commit** with message `style(pic16f87xa-hal): comment pass per expression rules`.

### Task 4: Comment pass, pic18fxx5x-hal

Same as Task 3, module `pic18fxx5x-hal` (~60 files), gate on `pic18fxx5x-hal`, commit `style(pic18fxx5x-hal): comment pass per expression rules`.

### Task 5: Comment pass, pic16f193x-hal

Same as Task 3, module `pic16f193x-hal` (~80 files), gate on `pic16f193x-hal`, commit `style(pic16f193x-hal): comment pass per expression rules`. Note: the file header comments in this HAL reference the 193X architecture doc; keep citations to `docs/ARCHITECTURE.md` where the live BSR convention is cited, drop the "Finding N" narrative.

### Task 6: Comment pass, epic-common + epic-tick + epic-debounce

**Files:** all first-party `.c`/`.h` under `epic-common/` (4), `epic-tick/` (3), `epic-debounce/` (5)
**Gate:** epic-common has no CMakeLists (it is include()'d by the HALs); gate it through the HALs that consume it, i.e. run the pic16f87xa-hal gate from Task 3's command plus the module gates for epic-tick and epic-debounce. If epic-tick or epic-debounce lacks a CMakeLists, gate through `tests/` or skip with a note in the report.
Commit: `style(epic-common): comment pass per expression rules` (one commit; the other two modules get their own commits: `style(epic-tick): comment pass per expression rules`, `style(epic-debounce): comment pass per expression rules`).

### Task 7: Comment pass, epic-math

**Files:** all first-party `.c`/`.h` under `epic-math/` (~32 files)
**Gate:** `cmake -B build-host/epic-math -S epic-math && cmake --build build-host/epic-math && ctest --test-dir build-host/epic-math --output-on-failure`
Commit: `style(epic-math): comment pass per expression rules`.
Note: this is the delicate one. The asm hand-traces in `src/pic16/` and `src/pic18/` prove carry/borrow semantics that cannot be read from the code. Compress each hand-trace to the essential steps (drop the decorative `/* ---- name ---- */` separators and the box art), keep the concrete worked example ONLY where it pins an invariant (e.g. the add-with-carry example), and keep the ARCHITECTURE.md citation for the inline-asm binding rules.

### Task 8: Comment pass, epic-swuart + epic-bus + epic-modbus

**Files:** `epic-swuart/` (11), `epic-bus/` (5), `epic-modbus/` (5) first-party `.c`/`.h`
**Gate:** the three module gates (each `cmake -B build-host/<m> -S <m> && cmake --build build-host/<m> && ctest --test-dir build-host/<m> --output-on-failure`)
Commits: one per module, `style(epic-swuart): ...` / `style(epic-bus): ...` / `style(epic-modbus): ...`.

### Task 9: Comment pass, epic-lcd + epic-sdcard + epic-settings

**Files:** `epic-lcd/` (11), `epic-sdcard/` (8), `epic-settings/` (7)
**Gate:** the three module gates. Note epic-sdcard is PIC18-only (RAM constraint): its host test may be a sim build; run whatever ctest the module defines.
Commits: one per module.

### Task 10: Comment pass, epic-fsm + epic-encoder + epic-taskmgr + epic-pid

**Files:** `epic-fsm/` (7), `epic-encoder/` (7), `epic-taskmgr/` (6), `epic-pid/` (6)
**Gate:** the four module gates.
Commits: one per module. Note epic-taskmgr's `include/task_manager.h` has a 6-line comment run; check whether it is invariant-carrying (the priority/race-free claims) and keep the invariant, drop the rest.

### Task 11: Comment pass, epic-mcp23x17 + epic-adcfilter + tests/epic-combo-rx-loopback

**Files:** `epic-mcp23x17/` (6), `epic-adcfilter/` (6), `tests/epic-combo-rx-loopback/` (14, first-party; this is the only combo with its own CMakeLists)
**Gate:** the three gates (`tests/epic-combo-rx-loopback` has a CMakeLists, gate it with `cmake -B build-host/epic-combo-rx-loopback -S tests/epic-combo-rx-loopback`).
Commits: one per unit.

### Task 12: Comment pass, epic-usb + epic-serial + epic-console

**Files:** first-party `.c`/`.h` under `epic-usb/` (8, EXCLUDING `third_party/`), `epic-serial/` (6), `epic-console/` (7)
**Gate:** the module gates for epic-usb (if its CMakeLists builds the third-party stack too, that is fine, the gate still passes; the third-party files themselves stay untouched), epic-serial, epic-console.
Commits: one per module.

### Task 13: Comment pass, examples/

**Files:** `examples/epicurus-demo-*.X/main.c` (the reference project mains, 3 files) and any other first-party source in `examples/`
**Gate:** none. The demo mains include `<xc.h>` (XC8-only), so no host build exists for them; the "no semantics change" constraint is the gate. Report the files as comment-only in the task report.
Commit: `style(examples): comment pass per expression rules`.

---

### Task 14: Delete the bitacores (Phase 3a)

**Files:**
- Delete: every file under `docs/superpowers/` (plans + specs)
- Delete: `docs/ci-plan.md`, `docs/docker-dev-plan.md`, `docs/multi-family-plan.md`, `docs/mplabx-link-gaps-plan.md`, `docs/pic16f193x-plan.md`, `docs/epic-encoder-plan.md`, `docs/epic-fsm-plan.md`, `docs/epic-math-plan.md`, `docs/epic-modbus-plan.md`, `docs/epic-sdcard-plan.md`, `docs/epic-usb-plan.md`, `docs/hal-epic-rename-plan.md`, `docs/hal-manual-plan.md`, `docs/pic8-epic-rename-plan.md`, `docs/pic8-epic-uppercase-rename-plan.md`, `docs/pic8-vga-plan.md`, `docs/quality-roadmap.md`, `docs/toolchain-coverage.md`
- Delete: `scripts/rename-hal-epic.sh`, `scripts/rename-hal-epic.awk`, `scripts/rename-pic8-epic.sh`, `scripts/rename-pic8-epic.awk`, `scripts/rename-pic8epic-uppercase.sh`, `scripts/rename-pic8epic-uppercase.awk`, `scripts/hal-epic-exceptions.txt`, `scripts/pic8-epic-modules.txt` (one-shot migration tooling from the completed renames)
- Keep: `docs/adding-a-device.md`, `docs/layout-budgets.md`

**Interfaces:**
- Consumes: nothing from earlier tasks.
- Produces: the deleted set that Task 16's reference sweep checks against.

- [ ] **Step 1: Check toolchain-coverage for unresolved work**

Run: `grep -n "SUSPECT\|OPEN" docs/toolchain-coverage.md | head`
If any entry is not SAFE/FIXED, move that specific item as a `TODO` with its reason into the top of `docs/layout-budgets.md` (it is a live bug candidate), then delete the file. If all SAFE/FIXED, delete outright.

- [ ] **Step 2: Check the rename scripts and txt files for live references**

Run: `grep -rn "rename-pic8\|rename-hal\|rename-pic8epic\|hal-epic-exceptions\|pic8-epic-modules" . --include='*.md' --include='*.sh' --include='*.py' --include='Makefile' --include='*.yml' | grep -v docs/superpowers`
Expected: no live references (the plan docs that referenced them are being deleted in this same task). If a live reference exists, fix the wording before deleting.

- [ ] **Step 3: Delete**

Run: `git rm -r docs/superpowers docs/ci-plan.md docs/docker-dev-plan.md docs/multi-family-plan.md docs/mplabx-link-gaps-plan.md docs/pic16f193x-plan.md docs/epic-encoder-plan.md docs/epic-fsm-plan.md docs/epic-math-plan.md docs/epic-modbus-plan.md docs/epic-sdcard-plan.md docs/epic-usb-plan.md docs/hal-epic-rename-plan.md docs/hal-manual-plan.md docs/pic8-epic-rename-plan.md docs/pic8-epic-uppercase-rename-plan.md docs/pic8-vga-plan.md docs/quality-roadmap.md docs/toolchain-coverage.md scripts/rename-hal-epic.sh scripts/rename-hal-epic.awk scripts/rename-pic8-epic.sh scripts/rename-pic8-epic.awk scripts/rename-pic8epic-uppercase.sh scripts/rename-pic8epic-uppercase.awk scripts/hal-epic-exceptions.txt scripts/pic8-epic-modules.txt`

- [ ] **Step 4: Distill any missing operational facts**

Check each surviving README/DEVELOPMENT/AGENTS for these operational facts; add a one-liner to the right home only if the fact is absent:
- docker tag formula and ci-image-push flow: `DEVELOPMENT.md` "Docker" section already covers the targets; add the tag formula line `xc8-v${XC8_VERSION}-dfp...-mplabx${MPLABX_VERSION}` if absent.
- mdb debug protocol (stepi over run+wait, check a known-good control register, high-risk-pattern checklist): AGENTS.md "Non-obvious things" already carries the condensed version; if a fuller procedure is needed, add 3-4 lines to DEVELOPMENT.md. Do not recreate ci-plan.md.
- the CI image-history warnings (EULA/private-GHCR constraint): DEVELOPMENT.md already states "private tag CI pulls"; keep that one line.

- [ ] **Step 5: Verify**

Run: `git status --short | head` and confirm only the intended deletions + any distill edits are staged.
Run: `PRE_COMMIT_BASE_REF=origin/master scripts/pre-commit-checks.sh`
Expected: PASS.

- [ ] **Step 6: Commit**

```bash
git add -A
git commit -m "docs: delete implemented plans, specs, and one-shot rename tooling"
```

---

### Task 15: Module ARCHITECTURE/API doc triage (Phase 3b)

**Files:** every `*/docs/ARCHITECTURE.md` and `*/docs/API.md` (per-module list: epic-adcfilter, epic-bus, epic-console, epic-debounce, epic-encoder, epic-fsm, epic-lcd, epic-math, epic-mcp23x17, epic-modbus, epic-pid, epic-sdcard, epic-serial, epic-settings, epic-swuart, epic-taskmgr, epic-tick, epic-usb, pic16f87xa-hal, pic18fxx5x-hal, pic16f193x-hal, and any other module with a docs/ dir)

**Interfaces:**
- Consumes: the rules' docs-lifecycle section; the AGENTS.md citations (epic-math/docs/ARCHITECTURE.md inline-asm binding, pic16f193x-hal/docs/ARCHITECTURE.md BSR Finding 1).
- Produces: per module, one of: keep (holds a live convention), distill (live facts merged into README.md or MANUAL.md, then the file deleted), or delete (pure narrative).

- [ ] **Step 1: Triage each ARCHITECTURE.md**: keep if it holds a live convention a maintainer must know to touch the code (epic-math asm binding, pic16f193x BSR addressing, epic-swuart if its timing conventions are still live). For pure-findings narrative (session logs, "Finding N" chains that end in a settled rule), move the settled rule into the module's README.md (3-6 lines) and delete the file.
- [ ] **Step 2: Triage each API.md**: keep if it documents something the README does not (function contracts beyond the README's usage examples). If the README already covers the API surface, delete the API.md and add a one-line pointer in the README if needed. Do not duplicate content: the README wins.
- [ ] **Step 3: Update the module README.md** for every distilled fact; keep READMEs at their current length plus at most 6 lines.
- [ ] **Step 4: Verify** with a per-module gate for any module whose README edit is the only change (no gate needed: docs only) and a repo-wide grep for links to any deleted per-module doc:

Run: `grep -rn "docs/ARCHITECTURE\|docs/API" --include='*.md' --include='*.c' --include='*.h' . | grep -v third_party | grep -v superpowers`
Every remaining reference must point at a surviving file; fix or delete as needed.

- [ ] **Step 5: Commit**

```bash
git add -A
git commit -m "docs: triage module architecture and API docs per expression rules"
```

---

### Task 16: Reference sweep (Phase 3c)

**Files:** any file referencing a deleted doc: `AGENTS.md`, `README.md`, `DEVELOPMENT.md`, `scripts/README.md`, module READMEs, `.github/workflows/*.yml`, `docker/ci-toolchain/Dockerfile`, first-party `.c`/`.h` comments, `docs/layout-budgets.md`, `docs/adding-a-device.md`

**Interfaces:**
- Consumes: the deletion sets from Tasks 14-15.
- Produces: zero references to any deleted path.

- [ ] **Step 1: Find every dangling reference**

Run: `grep -rn "superpowers\|ci-plan\|docker-dev-plan\|multi-family-plan\|mplabx-link-gaps\|pic16f193x-plan\|epic-encoder-plan\|epic-fsm-plan\|epic-math-plan\|epic-modbus-plan\|epic-sdcard-plan\|epic-usb-plan\|hal-epic-rename\|hal-manual-plan\|pic8-epic-rename\|pic8-epic-uppercase\|pic8-vga-plan\|quality-roadmap\|toolchain-coverage" --include='*.md' --include='*.c' --include='*.h' --include='*.sh' --include='*.py' --include='*.yml' . | grep -v third_party`
Expected: only the task's own worktree scratch files remain; fix every hit by rewording or deleting the reference. For AGENTS.md/README.md/DEVELOPMENT.md, replace doc links with the surviving equivalents (e.g. docker-dev-plan -> DEVELOPMENT.md "Docker" section; multi-family-plan -> epic-common/README.md + MANUAL.md).
- [ ] **Step 2: Fix the AGENTS.md "Non-obvious things" pointers** that cite `docs/` paths (lines referencing multi-family-plan and pic16f193x-plan): reword to the surviving homes (epic-common/README.md + MANUAL.md for the contract; nothing for the 193X plan, the content is settled).
- [ ] **Step 3: Verify with a link scan**

Run: `git grep -oE 'docs/[A-Za-z0-9_./-]+\.md' -- '*.md' | cut -d: -f3 | sort -u | while read p; do [ -e "$p" ] || echo "DANGLING: $p"; done`
Expected: no `DANGLING` lines (ignore `docs/superpowers` output before Task 14 lands; after Task 14 there must be none).

- [ ] **Step 4: Commit**

```bash
git add -A
git commit -m "docs: sweep references to deleted docs"
```

---

### Task 17: Final gate (Phase 4)

**Files:** none (verification only; fix anything that fails).

- [ ] **Step 1: Full host sweep**

Run: `fail=0; for m in $(git ls-files -- '*/CMakeLists.txt' | sed 's#/CMakeLists.txt$##' | sort); do echo "== $m"; cmake -B "build-host/$m" -S "$m" >/dev/null && cmake --build "build-host/$m" >/dev/null && ctest --test-dir "build-host/$m" --output-on-failure || fail=1; done; exit $fail`
Expected: all modules PASS.
- [ ] **Step 2: Script tests**

Run: `python3 scripts/tests/test_ci_noncode.py && python3 scripts/tests/test_epic_build.py && python3 scripts/tests/test_epicmanifest.py && python3 scripts/tests/test_bundlegen.py`
Expected: all pass.
- [ ] **Step 3: Pre-commit + dangling refs**

Run: `PRE_COMMIT_BASE_REF=origin/master scripts/pre-commit-checks.sh` (PASS) and the Task 16 dangling-link scan again (no DANGLING).
- [ ] **Step 4: Sanity that the cleanup is comment-only**

Run: `git diff origin/master --stat | tail -20` and confirm the diff is docs + comments (the `.c`/`.h` diffs must be comment-only by construction; spot check one HAL diff).
- [ ] **Step 5: Commit any fix-ups** with `style`/`docs` messages; the branch must end clean.

---

### Post-plan (PR-time, not a task)

Before opening the PR: `git rm docs/superpowers/specs/2026-08-11-cleanup-session-design.md docs/superpowers/plans/2026-08-11-cleanup-session.md` and commit, so the PR carries only the cleanup (standing preference). Push and open the PR; note in the body that the cleanup is comment/doc-only, gated by the full host sweep + script tests, and that plans are now ephemeral per AGENTS.md.
