# epic_* API Naming Convention Implementation Plan

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:subagent-driven-development (recommended) or superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

Status: **approved 2026-08-11, in progress**.

**Goal:** Rename every public symbol in the eight violating modules to the `epic_<module>_*` convention (pure symbol renames, all callers and docs updated, zero old-prefix references), and write the convention into AGENTS.md.

**Architecture:** Prefix substitution per module, applied repo-wide by one implementer per module group (disjoint prefixes, safe in parallel). Completeness is a grep gate. No behavior change; golden vectors regenerate with identical values.

**Tech Stack:** C, grep/ast-level codemod, CMake/ctest, git.

## Global Constraints

- No em-dash characters in added lines or commit messages; Conventional Commits.
- Pure symbol renames: no behavior change, no logic edits. Docstring @param names do not change (only function names), so the doxygen checker stays green.
- Third-party (`epic-usb/third_party/**`) untouched.
- The pre-commit hook runs on every commit (worktree-scoped). Never bypass. Commit with explicit pathspecs; shared-index hygiene.
- The ephemeral-plans convention applies (spec/plan deleted on completion).

## The rename map (per module, all symbols)

- `pic_math_` -> `epic_math_` (epic-math, 21 symbols)
- `task_manager_` -> `epic_taskmgr_`; `task_spawn` -> `epic_taskmgr_spawn` (epic-taskmgr, 8)
- `debounce_` -> `epic_debounce_` (epic-debounce, 3)
- `encoder_` -> `epic_encoder_` (epic-encoder, 7)
- `fsm_` -> `epic_fsm_` (epic-fsm, 3)
- `pid_` -> `epic_pid_` (epic-pid, 6)
- `swuart_` -> `epic_swuart_` (epic-swuart, incl. the swuart_test_* hooks)
- `console_` -> `epic_console_` (epic-console; `epic_console_init` already conforms)

Exclusions: already-`epic_` names stay; `pic_math_` must NOT touch the HAL internals (`pic16f87xa_*`, `pic18_*`, `pic16f193x_*`); `console_` must not touch libc (`printf`, etc. - only the module's own `console_*` symbols); `swuart_` only the module's symbols; third-party untouched.

## The completeness gate (per task and final)

`grep -rnE '\b(pic_math|debounce_|encoder_|fsm_|pid_|swuart_|console_|task_manager|task_spawn)[a-z0-9_]*\(' . --include='*.c' --include='*.h' --include='*.md' | grep -v third_party` -> zero matches.

---

### Task 1: AGENTS.md naming-convention rule

**Files:** `AGENTS.md`

- [ ] **Step 1: Add one bullet** to the conventions section (module anatomy area):

```markdown
- **API naming:** module `epic-X` exports `epic_x_*` (lowercase, e.g.
  `epic_serial_init`); HALs export `EPIC_*` uppercase for the
  cross-family contract (EPIC_GPIO_Init); family-internal helpers use
  the family prefix (`pic16f87xa_*`, `pic18_*`, `pic16f193x_*`);
  epic-common harness glue uses `epic_*`; third-party keeps its own
  names. A module's public symbols never use a bare short prefix
  (`fsm_*`, `task_manager_*`).
```

- [ ] **Step 2: Verify** `PRE_COMMIT_BASE_REF=origin/master scripts/pre-commit-checks.sh` PASS; commit `docs(agents): add the epic_* API naming convention`.

### Task 2: epic-math rename

**Files:** `epic-math/**` + every repo caller of `pic_math_*` (epic-pid, tests, examples, combos, `tests/` dirs), `README.md`, docs.

- [ ] **Step 1: Rename all `pic_math_` symbols** repo-wide (headers, definitions, callers). `gen_golden_vectors.c` calls rename too; **regenerate golden_vectors.h** (build + run the tool; the diff must be zero besides nothing - the values are identical, only the generator source changed).
- [ ] **Step 2: Docs**: epic-math README/API.md/MANUAL.md/ARCHITECTURE.md, root README mentions, any MANUAL cross-references.
- [ ] **Step 3: Gates**: epic-math ctest (9/9) + epic-pid ctest (it calls pic_math_*) + the completeness grep for `pic_math` zero matches.
- [ ] **Step 4: Commit** `refactor(epic-math): rename pic_math_* API to epic_math_*`.

### Task 3: epic-taskmgr rename

**Files:** `epic-taskmgr/**` + every repo caller of `task_manager_*`/`task_spawn` (combos, examples, tests), `README.md` (the scheduler example uses task_manager_init/task_spawn/task_manager_attach_timer0/task_manager_run).

- [ ] **Step 1: Rename** `task_manager_*` -> `epic_taskmgr_*` and `task_spawn` -> `epic_taskmgr_spawn` repo-wide.
- [ ] **Step 2: Docs**: epic-taskmgr README/API docs + the root README example code blocks.
- [ ] **Step 3: Gates**: taskmgr ctest (3/3) + the grep for `task_manager|task_spawn` zero matches.
- [ ] **Step 4: Commit** `refactor(epic-taskmgr): rename task_manager_* API to epic_taskmgr_*`.

### Task 4: epic-debounce + epic-encoder renames

- [ ] Rename `debounce_*` -> `epic_debounce_*` repo-wide (callers: examples, tests, combos, docs). Gate: debounce ctest (2/2) + grep clean.
- [ ] Rename `encoder_*` -> `epic_encoder_*` repo-wide (callers incl. combos, docs). Gate: encoder ctest (12/12) + grep clean.
- [ ] Commits: `refactor(epic-debounce): ...` and `refactor(epic-encoder): ...`.

### Task 5: epic-fsm + epic-pid renames

- [ ] Rename `fsm_*` -> `epic_fsm_*` repo-wide. Gate: fsm ctest (1/1) + grep clean.
- [ ] Rename `pid_*` -> `epic_pid_*` repo-wide (epic-pid calls epic_math_mul_s16 after Task 2). Gate: pid ctest (10/10) + grep clean.
- [ ] Commits: `refactor(epic-fsm): ...` and `refactor(epic-pid): ...`.

### Task 6: epic-swuart + epic-console renames

- [ ] Rename `swuart_*` -> `epic_swuart_*` repo-wide (incl. the swuart_test_* hooks used by tests). Gate: swuart ctest (7/7) + grep clean.
- [ ] Rename `console_*` -> `epic_console_*` repo-wide (console_dispatch/tokenize/write_str; `epic_console_init` already conforms). Gate: console ctest (4/4) + grep clean.
- [ ] Commits: `refactor(epic-swuart): ...` and `refactor(epic-console): ...`.

### Task 7: Final gates and ephemeral cleanup

- [ ] **Step 1**: the completeness grep zero repo-wide (excluding third_party).
- [ ] **Step 2**: full 22-module ctest sweep PASS; doxygen checker full+brief exit 0; `PRE_COMMIT_BASE_REF=origin/master scripts/pre-commit-checks.sh` PASS.
- [ ] **Step 3**: bundle dry-run for one family clean.
- [ ] **Step 4**: delete `docs/superpowers/specs/2026-08-11-api-naming-convention-design.md` and `docs/superpowers/plans/2026-08-11-api-renames.md`; commit `plan(api): mark naming convention enforced, delete ephemeral plan`.

---

### Post-plan (PR-time)

Push `feat/api-renames` and open the PR (user merges, never the agent).
