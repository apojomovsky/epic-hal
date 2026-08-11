# Doxygen-style Docstrings Implementation Plan

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:subagent-driven-development (recommended) or superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

Status: **approved 2026-08-11, in progress**.

**Goal:** Every first-party function gets a Doxygen-style docstring (`@brief` always, `@param` per argument without `[in]`/`[out]`, `@return` for non-void), enforced by a custom checker script with no doxygen dependency. Library code gets full docs; tests/examples get `@brief`-only.

**Architecture:** A Python checker (`scripts/doxygen_doc_check.py`) is the compliance definition: it tokenizes C function signatures, matches each to its preceding `/** */` block, and fails on missing docs, missing `@param`, missing `@return`, or `[in]`/`[out]` tags. Batch agents instrument each module group against the contract, gated by checker-clean + module ctest. AGENTS.md gains the contract.

**Tech Stack:** Python 3 (stdlib), C, CMake/ctest, git.

## Global Constraints

- No em-dash characters in added lines or commit messages; Conventional Commits.
- **No comment reduction**: docstrings are added or existing comments are converted to the Doxygen form and extended with missing `@param`/`@return`. No existing comment content is deleted (beyond comment-marker conversion).
- No C semantics change: docstrings only.
- Third-party (`epic-usb/third_party/**`) untouched.
- The checker is the compliance definition; the module gate is checker-clean + the module's host ctest (`cmake -B build-host/<m> -S <m> && cmake --build build-host/<m> && ctest --test-dir build-host/<m> --output-on-failure`).
- The pre-commit hook runs on every commit (worktree-scoped, cppcheck 2.19-robust). Never bypass.
- The ephemeral-plans convention applies: this spec and plan are deleted when the work lands (final task).

## The docstring contract (verbatim, from the spec)

```c
/**
 * @brief One-line summary.
 *
 * Longer explanation only when the behavior is not obvious.
 *
 * @param name what it is
 * @param out_buf where results are written
 * @return what the caller gets
 */
```

1. `@brief` required on every function doc block.
2. `@param name` per named parameter, order-insensitive, names must match the signature; no `@param[in]`/`@param[out]`/`[in,out]`.
3. `@return` required for non-void; none for void.
4. The block must be Doxygen-style (`/** ... */`), not `/*` or `//`.
5. No `@file`/`@author`/`@date`; `@details` only when needed.
6. Placement: headers for public API; `.c` for `static` functions (non-static `.c` definitions are covered by the header). Tests/examples: `@brief`-only.

---

### Task 1: The checker and its unit tests

**Files:**
- Create: `scripts/doxygen_doc_check.py`
- Create: `scripts/tests/test_doxygen_doc_check.py`

**Interfaces:**
- Produces: `python3 scripts/doxygen_doc_check.py [--brief-only] FILE...` exiting 0 (compliant) or 1 (violations listed to stderr). Consumed by every later task as the gate.

- [ ] **Step 1: Write `scripts/doxygen_doc_check.py`**

Contract:
- Tokenize C: find all `/** ... */` doc blocks and all function signatures. A function signature is a declaration/definition starting with optional qualifiers (`static`, `inline`, `const`, `volatile`, XC8 `__interrupt`) and a type, then `name(`, with balanced-paren parameter parsing (handles nested parens, function-pointer params like `void (*cb)(int)`, multi-line signatures, `...`). XC8 `__at(addr)` attributes on functions or in signatures must be tolerated (strip `\b__at\s*\([^)]*\)` tokens before matching).
- Return type: the type text before the name; non-void unless it is exactly `void` (so `void *` and `const void *` are non-void).
- Doc association: the nearest preceding `/** */` block with only whitespace between block end and the signature line.
- Violations per function: missing doc block; doc not `/**`-style; `@brief` missing; `@param <name>` missing for any named parameter (match by word boundary; `void` and `...` params are not named); `[in]`/`[out]`/`[in,out]` present anywhere in the block; `@return` missing for non-void; `@return` present for void.
- `--brief-only`: only requires the doc block, `/**` style, and `@brief`.
- Fail-closed: anything that looks like a function but cannot be parsed (unbalanced parens, attribute forms not handled) is reported as `UNPARSEABLE` and makes the run fail, so a human looks.
- Output: one line per violation, `file:line: kind: function: detail`; summary line; exit 1 if any violation or unparseable.

- [ ] **Step 2: Write `scripts/tests/test_doxygen_doc_check.py`** (unittest, same pattern as the other script tests: `sys.path.insert(0, ...parents[1])`, then import). Cases:
  - a fully documented function passes;
  - missing doc fails; missing `@brief` fails; missing one `@param` fails; extra `@param` for an unnamed param fails; `@param[in]` fails; non-void without `@return` fails; void with `@return` fails; `//`-style doc fails;
  - multi-line signature, function-pointer param, `...` param, `static`, `const char *` return, `void *` return (non-void), `__at(0x100)` attribute on a function, `__interrupt` ISR;
  - `--brief-only` passes a `@brief`-only block and still fails a missing `@brief`;
  - a `void`-parameter function (`f(void)`) needs no `@param`.

- [ ] **Step 3: Run the unit tests** until green.
- [ ] **Step 4: Run the checker on the current tree** (`python3 scripts/doxygen_doc_check.py $(git ls-files '*.c' '*.h' | grep -v third_party)`) and inspect the output. Acceptance: every reported violation is a REAL function lacking docs (the work inventory), and there are no false positives or UNPARSEABLE noise from attributes/function pointers/macros. Fix parser gaps until true; pin each gap in the unit tests.
- [ ] **Step 5: Commit** `feat(ci): add doxygen-style docstring compliance checker`.

---

### Task 2: AGENTS.md conventions update

**Files:**
- Modify: `AGENTS.md` (Expression conventions section)

- [ ] **Step 1: Add a "Function documentation" subsection** under Expression conventions > Comments:

```markdown
### Function docstrings (Doxygen style)

Every first-party function carries a Doxygen-style docstring:

- `@brief` on every function; a longer `@details` only when the
  behavior is not obvious.
- `@param name` per named argument, names matching the signature, with
  in/out semantics in the prose; never `@param[in]`/`@param[out]`.
- `@return` for non-void functions, nothing for void.
- The block is `/** ... */`, never `/*` or `//`.
- Placement: headers for public API; `.c` for `static` functions.
  Tests and examples: `@brief`-only.
- `scripts/doxygen_doc_check.py` is the compliance checker; run it
  before finishing work that touches functions.
```

- [ ] **Step 2: Verify** `PRE_COMMIT_BASE_REF=origin/master scripts/pre-commit-checks.sh` PASS; commit `docs(agents): add function docstring contract to expression conventions`.

---

### Docstring batch recipe (shared by Tasks 3-13)

Each batch applies the same steps to its module group:

- [ ] **Step 1: Instrument.** For every function declared in the group's headers and every `static` function in its `.c` files, add or convert the docstring per the contract (convert existing `/* */` or `//` comments above functions to `/** */` and extend; keep all existing comment content). For the group's tests/examples, ensure `@brief` on every function (brief style).
- [ ] **Step 2: Check.** `python3 scripts/doxygen_doc_check.py $(git ls-files '<group paths>' | grep -v third_party)` exit 0, and `python3 scripts/doxygen_doc_check.py --brief-only $(git ls-files '<group tests/examples>' | grep -v third_party)` exit 0.
- [ ] **Step 3: Gate.** The module ctest gate(s) for the group (command in Global Constraints).
- [ ] **Step 4: No-reduction check.** `git diff` for the group shows only added doc lines and comment-marker conversions; no existing comment content deleted.
- [ ] **Step 5: Commit** `docs(<scope>): add doxygen docstrings to <module>` (one commit per module in the group).

### Task 3: pic16f87xa-hal

Group: all `.c`/`.h` under `pic16f87xa-hal/` (lib full mode, tests/examples brief mode). Gates: `cmake -B build-host/pic16f87xa-hal -S pic16f87xa-hal && cmake --build build-host/pic16f87xa-hal && ctest --test-dir build-host/pic16f87xa-hal --output-on-failure` (this module registers no ctest tests; run the example smoke binaries as the behavioral gate, same as the cleanup task). Commit: `docs(pic16f87xa-hal): add doxygen docstrings`.

### Task 4: pic18fxx5x-hal

Same as Task 3 for `pic18fxx5x-hal/`. Commit: `docs(pic18fxx5x-hal): add doxygen docstrings`.

### Task 5: pic16f193x-hal

Same as Task 3 for `pic16f193x-hal/` (largest HAL, ~80 files). Commit: `docs(pic16f193x-hal): add doxygen docstrings`.

### Task 6: epic-common + epic-tick + epic-debounce

Group: the three modules. epic-common has no CMakeLists: gate through pic16f87xa-hal with a DEDICATED build dir (`cmake -B build-host-epic-common -S pic16f87xa-hal && cmake --build build-host-epic-common && ctest --test-dir build-host-epic-common --output-on-failure`), plus the epic-tick and epic-debounce gates. Commits: one per module (`docs(epic-common): add doxygen docstrings`, etc.).

### Task 7: epic-math

Group: `epic-math/` (asm backends included: the hand-trace comments stay, the docstring blocks go above each function). Gate: the epic-math gate. Commit: `docs(epic-math): add doxygen docstrings`.

### Task 8: epic-swuart + epic-bus + epic-modbus

Group: the three modules; three gates, three commits.

### Task 9: epic-lcd + epic-sdcard + epic-settings

Group: the three modules; three gates (epic-sdcard: whatever ctest the module defines), three commits.

### Task 10: epic-fsm + epic-encoder + epic-taskmgr + epic-pid

Group: the four modules; four gates, four commits.

### Task 11: epic-mcp23x17 + epic-adcfilter + tests/epic-combo-rx-loopback

Group: the two modules (full) + the combo (brief-only; it is a test unit). Gates: mcp23x17, adcfilter, and `cmake -B build-host/epic-combo-rx-loopback -S tests/epic-combo-rx-loopback && cmake --build build-host/epic-combo-rx-loopback && ctest --test-dir build-host/epic-combo-rx-loopback --output-on-failure`. Commits: three.

### Task 12: epic-usb (first-party) + epic-serial + epic-console

Group: first-party files under the three modules (third_party untouched). Three gates, three commits.

### Task 13: examples/ and remaining tests sweep (brief mode)

Group: `examples/**/*.c`, `tests/**/*.c` not covered by a module batch. Instrument `@brief` on every function (brief style; these are demo/test mains and helpers). No gate (XC8-only mains have none); the checker brief-mode run is the gate. Commit: `docs(examples): add doxygen briefs`.

---

### Task 14: Final gate and ephemeral cleanup

**Files:** none (verification only) plus the spec/plan deletion.

- [ ] **Step 1: Full-tree checker.** `python3 scripts/doxygen_doc_check.py $(git ls-files '*.c' '*.h' | grep -v third_party)` exit 0; and the brief-mode run over `tests/` + `examples/` paths exit 0.
- [ ] **Step 2: Script tests.** `python3 scripts/tests/test_doxygen_doc_check.py` (and the other script tests) PASS.
- [ ] **Step 3: Full ctest sweep.** `for m in $(git ls-files -- '*/CMakeLists.txt' | sed 's#/CMakeLists.txt$##' | sort); do cmake -B build-host/$m -S $m && cmake --build build-host/$m && ctest --test-dir build-host/$m || exit 1; done` all PASS.
- [ ] **Step 4: Pre-commit** `PRE_COMMIT_BASE_REF=origin/master scripts/pre-commit-checks.sh` PASS.
- [ ] **Step 5: Ephemeral cleanup.** Delete `docs/superpowers/specs/2026-08-11-doxygen-docstrings-design.md` and `docs/superpowers/plans/2026-08-11-doxygen-docstrings.md` (the ephemeral-plans convention; the work is done, git history is the archive). Commit `plan(doxygen): mark docstring contract implemented, delete ephemeral plan`.
- [ ] **Step 6: Sanity.** `git status --short` clean; `git diff origin/master --stat` shows only docstring/comment additions and the checker files.

---

### Post-plan (PR-time, not a task)

Push `feat/doxygen-docs` and open the PR against master (user merges, never the agent). The PR body notes: the checker is the compliance definition (no doxygen dependency), the contract lives in AGENTS.md, coverage split (lib full, tests/examples brief), and that the final gate (full-tree checker + 22-module ctest + pre-commit) passed.
